#include "../include/proxy.h"
/*
int dynamic_realloc(void **ptr, size_t *allocated_size, size_t required_size, size_t *sizeofelement){

    if (required_size <= *allocated_size) return 0;

	void *new_ptr = realloc(*ptr, (*allocated_size+100)*sizeofelement);
	size_t new_size = *allocated_size+100;

	if(new_ptr == NULL){

		new_ptr = realloc(*ptr, (*allocated_size + 10)* sizeofelement );
		new_size = *allocated_size + 10;

		if(new_ptr == NULL){

			new_ptr = realloc(*ptr, (*allocated_size + 1)*sizeofelement);
			new_size = *allocated_size + 1;

			if(new_ptr == NULL){
				new_ptr = ptr;
				new_size = *allocated_size ;
				return -1;
			}
		}
	}

	*allocated_size = new_size;
	*ptr = new_ptr;

	return 0;
}
*/

Client* get_client(Shared_memory *shm, char *name){
    printf("Recherche d'un client \n");

    int cmp;

    for(int i = 0 ; i< shm->nb_clients;i++ ){
        lock(&shm->clients[i].client_mutex);
            cmp = strncmp(shm->clients[i].name,name, MAX_PSEUDO_LENGTH-1);
        unlock(&shm->clients[i].client_mutex);
        
        if(cmp==0){
            return shm->clients + i;
        }
    }

    return NULL;
}

int get_client_fd(Shared_memory *shm,Sockaddr_in *addr){

    int cmp,borne;
    borne = shm->nb_clients;

    for(int i = 0 ; i< borne;i++ ){

        lock(&shm->clients[i].client_mutex);
            cmp = memcmp(&shm->clients[i].addr,addr, sizeof(Sockaddr_in));
        unlock(&shm->clients[i].client_mutex);

        if(cmp==0){
            return shm->clients[i].sock_fd;
        }
    }

    return -1;
}

Server* get_server_with_min_load(Shared_memory *shm){
    int min = INT_MAX,borne;
    Server *s = NULL;

    borne = shm->nb_servers;

    for(int i = 0 ; i< shm->nb_servers;i++ ){

        lock(&shm->servers[i].server_mutex);
            if(shm->servers[i].load < min){
                min = shm->servers[i].load;
                s = shm->servers+i;
            }
        unlock(&shm->servers[i].server_mutex);
    }

    if(s != NULL){
        lock(&s->server_mutex);
            s->load++;
        unlock(&s->server_mutex);
    }

    return s;
}

void *handle_client_stream(void *arg){
    
    Client *c = (Client*) arg;
    memset(&c->data, 0, sizeof(c->data));
    int continuer =1;
    Data_type type;

    // receive nam

    printf("\t Thread s\'occupe du client %s:%d connecté \n",inet_ntoa(c->addr.sin_addr),ntohs(c->addr.sin_port));
    //fflush(stdout);

    while(continuer){
        
        lock(&c->client_mutex);
            if(recv(c->sock_fd, &c->data, sizeof(Data), 0)< 0){
                perror("client recv");
                unlock(&c->client_mutex);
                pthread_exit(NULL);
            };
            memcpy(&c->data.origin_addr, &c->addr, sizeof(c->addr));
            type = c->data.type;
        unlock(&c->client_mutex);

        printf("\t Thread Client reception de : \n",c->name);
        print_data(&c->data);
        fflush(stdout);

        // if client close the connection
        if(type == DT_CLO){
            printf("Client %s:%d ->deconnexion \n",inet_ntoa(c->addr.sin_addr),ntohs(c->addr.sin_port));
            fflush(stdout);
            continuer = 0;
            
            // remove client from list
            int fd;
            Server *s;

            lock(&c->client_mutex);
                fd = c->sock_fd;
                s = c->server;
                c->sock_fd = -1;
            unlock(&c->client_mutex);

            close(fd);

            // decrement server load
            lock(&s->server_mutex);
                s->load = s->load > 0 ? s->load - 1 : 0;
            unlock(&s->server_mutex);
        }

        // if client want to connect
        else if(type == DT_CON){;
            
            lock(&c->client_mutex);

                recv(c->sock_fd, c->name, MAX_PSEUDO_LENGTH, 0);
        
            unlock(&c->client_mutex);

            printf("Client %s %s:%d ->connexion  \n",c->name,inet_ntoa(c->addr.sin_addr),ntohs(c->addr.sin_port));
            fflush(stdout);
        }

        // if client want to get address of another client
        else if(type == DT_GET){

            char name[MAX_PSEUDO_LENGTH];
            memset(name, 0, MAX_PSEUDO_LENGTH);

            // receive name
            recv(c->sock_fd, name, MAX_PSEUDO_LENGTH, 0);
            Client *c2 = get_client(c->shm,name);
            memset(&c->data, 0, sizeof(c->data));

            if(c2 == NULL) 
                c->data.type = DT_INE;
            else{
                c->data.type = DT_GET;
                lock(&c2->client_mutex);
                    memcpy(&c->data.origin_addr, &c2->addr, sizeof(Sockaddr_in)); // on envoie l'adresse du client demandé
                unlock(&c2->client_mutex);
            }
            
            send(c->sock_fd, &c->data, sizeof(c->data), 0);
        }

        else if(type == DT_CEX || type == DT_FEX || type == DT_BOC || type == DT_BOF){
            bruiter(&c->data.data);
            send(c->server->server_fd, &c->data, sizeof(c->data), 0);
        }

        type = DT_CON;
        lock(&c->client_mutex);
            memset(&c->data, 0, sizeof(c->data));
        unlock(&c->client_mutex);
    }

    pthread_exit(NULL);
}

void *handle_server_stream(void *arg){
    Server *s = (Server*) arg;
    
        printf("Serveur %s:%d connecté \n",inet_ntoa(s->addr.sin_addr),ntohs(s->addr.sin_port));
    

    while(s->shm->nb_servers >0){
        // receive data from server
        memset(&s->data, 0, sizeof(Data));
        if(recv(s->server_fd, &s->data, sizeof(Data), 0) <0){
            perror("server recv");
            pthread_exit(NULL);
        }
 
        printf("\t Serveur %s:%d a envoyé \n",inet_ntoa(s->addr.sin_addr),ntohs(s->addr.sin_port));
        print_data(&s->data);
        // send data to client
        int fd = get_client_fd(s->shm, &s->data.dest_addr);

        // if the dest client has close the connection,
        // we send the message to sender client
        if(fd != -1){
            s->data.type = s->data.type == DT_CEX ? DT_EOC : EOF;
            fd = get_client_fd(s->shm, &s->data.origin_addr);
        }

        send(fd, &s->data, sizeof(s->data), 0) ;
        lock(&s->server_mutex);
            memset(&s->data, 0, sizeof(Data));
        lock(&s->server_mutex);
    }
    pthread_exit(NULL);
}

void accept_new_client(Proxy *p){

   Client *c = &p->shm.clients[p->shm.nb_clients];

    // init client
   lock(&c->client_mutex);
        memset(c, 0, sizeof(Client));
        c->sock_fd = -1;
        memset(&c->addr, 0, sizeof(c->addr));
        c->server = NULL;
        memset(c->name,'\0',MAX_PSEUDO_LENGTH);
        c->shm = &p->shm;
    unlock(&c->client_mutex);


    int len = sizeof(struct sockaddr);
    lock(&c->client_mutex);
        c->sock_fd =  accept( p->sock_listen_fd,( struct sockaddr *) &c->addr, &len);
    unlock(&c->client_mutex);
    if(c->sock_fd <= 0){
        fprintf(stderr, "Error accept client %s: \n", inet_ntoa(c->addr.sin_addr), ntohs(c->addr.sin_port));
        perror("accept");
        fflush(stderr);
        return;
    }

    lock(&c->client_mutex);

        c->server = get_server_with_min_load(&p->shm);
        if (c->server == NULL){
            close(c->sock_fd);
            unlock(&c->client_mutex);
            return;
        }

        lock(&c->server->server_mutex);
            c->server->load++;
        unlock(&c->server->server_mutex);

    unlock(&c->client_mutex);
   
    
    if(pthread_create(p->thread_client+p->shm.nb_clients, NULL, handle_client_stream, c)==-1) 
        return;

    printf("\t Client %d connecté %s:%d \n", p->shm.nb_clients,inet_ntoa(c->addr.sin_addr), ntohs(c->addr.sin_port));
    fflush(stdout);

    p->shm.nb_clients++;
}

void open_new_server(Proxy *p){
    
    Server *s = &p->shm.servers[p->shm.nb_servers];

    lock(&s->server_mutex);
        s->server_fd  = socket(AF_INET, SOCK_STREAM, 0);
    
    if( s->server_fd  == -1){
        unlock(&s->server_mutex);
        perror("socket");
        return;
    }

    unlock(&s->server_mutex);

    printf(" Tentative de connexion au Serveur %d:%s : %d \n",p->shm.nb_servers, inet_ntoa(s->addr.sin_addr), ntohs(s->addr.sin_port));
    
    lock(&s->server_mutex);
        if(connect(s->server_fd ,(struct sockaddr *) &s->addr, sizeof(struct sockaddr)) == -1){
                close(s->server_fd );
                s->server_fd = -1;
            unlock(&s->server_mutex);
            perror("connect");
            return;
        }
    unlock(&s->server_mutex);

    if(pthread_create(p->thread_server+p->shm.nb_servers, NULL, handle_server_stream,s)==-1){
        lock(&s->server_mutex);
            close(s->server_fd);
            s->server_fd = -1;
        unlock(&s->server_mutex);
        perror("pthread_create");
        return;
    }

    printf(" Connecté au Serveur %d : %s:%d \n",p->shm.nb_servers, inet_ntoa(s->addr.sin_addr), ntohs(s->addr.sin_port));

    p->shm.nb_servers++;
}

void initialize_proxy(Proxy *p, int argc,char **argv){

    memset(p, 0, sizeof(Proxy));
    printf("initialisation du proxy :\n");

    int port=0;
    if(argc > 2)
        port = atoi(argv[2]);
    
    
    
    // init all mutex
    printf("initialisation des mutex :\n");
    for(int i=0; i<BUFLEN; i++){
        pthread_mutex_init(&p->shm.clients[i].client_mutex ,NULL);
        pthread_mutex_init(&p->shm.servers[i].server_mutex ,NULL);
    }

    p->shm.nb_clients = 0;
    p->shm.nb_servers = 0;


    printf("Lecture de la liste des serveurs :\n");

    FILE * f = NULL;
    PCHK( f = fopen( argv[1], "r" ) ); 

    char line[128];
    char *ip = NULL,*pt = NULL;
    Server *s = NULL;
    int sport = -1;
    
    while ( !feof(f) ){
        
        // format : ip:port
        PCHK(fgets(line, 128, f ));

        ip = strtok(line, ":");
            if(ip == NULL) continue;
        pt = strtok(NULL, "");
        sport = pt ==NULL ? DEFAULT_EXCHANGE_PORT_SERVER : htons(atoi(pt));

        s = &p->shm.servers[p->shm.nb_servers];

        //init server
        lock(&s->server_mutex);
            memset(s, 0, sizeof(Server));
            s->shm = &p->shm;
            s->addr.sin_family = AF_INET;
            s->addr.sin_addr.s_addr = inet_addr(ip);
            s->addr.sin_port = sport;
        unlock(&s->server_mutex);
        
        printf("Serveur %d : %s:%d\n", p->shm.nb_servers, inet_ntoa(s->addr.sin_addr), ntohs(s->addr.sin_port));
        open_new_server(p);
    }

    fclose(f);

    if(p->shm.nb_servers == 0){
        raler("Pas de serveur trouvé\n");
    }

    p->proxy_addr.sin_family = AF_INET;
    p->proxy_addr.sin_port = port > 0 ? htons(port) : DEFAULT_LISTEN_PORT_PROXY;
    p->proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    printf("Démarrage de l'écoute ...\n");
    CHK( p->sock_listen_fd = socket(AF_INET, SOCK_STREAM, 0) );
    CHK( bind(p->sock_listen_fd, (struct sockaddr *) &p->proxy_addr, sizeof(struct sockaddr)) );
    CHK( listen(p->sock_listen_fd, SOMAXCONN) );
    printf("Ecoute : %s:%d\n", inet_ntoa(p->proxy_addr.sin_addr),ntohs(p->proxy_addr.sin_port));
    fflush(stdout);
    
}

int main(int argc, char **argv){

    Proxy p;

    printf("Proxy démarré\n");

    initialize_proxy(&p, argc, argv);
    printf("Proxy initialisé\n");

    printf("En attente de clients\n");
    fflush(stdout);

    while (p.shm.nb_clients < BUFLEN) {

        printf("\t Attente d'une nouvelle connexion\n");
        accept_new_client(&p);

    }
    close(p.sock_listen_fd);

    for(int i = 0; i < p.shm.nb_clients; i++) 
        pthread_join(p.thread_client[i], NULL);

    // when all clients are disconnected, we send a message to all servers
    Data data;
    data.type = DT_EOJ;
    for(int i = 0; i < p.shm.nb_servers; i++){
        // send a message to the server to stop
        send(p.shm.servers[i].server_fd, &data, sizeof(data), 0);
        pthread_cancel(p.thread_server[i]);
        pthread_join(p.thread_server[i], NULL);
    }


    return EXIT_SUCCESS;

}