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
    lock(&shm->mutex_clients);
    for(int i = 0 ; i< shm->nb_clients;i++ ){
        if(strncmp(shm->clients[i].name,name, MAX_PSEUDO_LENGTH)==0){
            unlock(&shm->mutex_clients);
            return shm->clients + i;
        }
    }
    unlock(&shm->mutex_clients);
    return NULL;
}

int get_client_fd(Shared_memory *shm,Sockaddr_in *addr){
    lock(&shm->mutex_clients);
    for(int i = 0 ; i< shm->nb_clients;i++ ){
        if(memcmp(&shm->clients[i].addr,addr, sizeof(Sockaddr_in))==0){
            unlock(&shm->mutex_clients);
            return shm->clients[i].sock_fd;
        }
    }
    unlock(&shm->mutex_clients);
    return -1;
}

Server* get_server_with_min_load(Shared_memory *shm){
    int min = INT_MAX;
    Server *s = NULL;

    lock(&shm->mutex_servers);

    for(int i = 0 ; i< shm->nb_servers;i++ ){
        if(shm->servers[i].load < min){
            min = shm->servers[i].load;
            s = shm->servers+i;
        }
    }
    if(s != NULL)
        s->load++;

    unlock(&shm->mutex_servers);

    return s;
}

void *handle_client_stream(void *arg){
    
    Client *c = (Client*) arg;
    Data data;
    int continuer =1;
    // receive name
    recv(c->sock_fd, c->name, MAX_PSEUDO_LENGTH, 0);

    while(continuer){
        memset(&data, 0, sizeof(data));
        memcpy(&data.origin_addr, &c->addr, sizeof(Sockaddr_in));
        recv(c->sock_fd, &data, sizeof(data), 0);

        print_data(&data);

        // if client close the connection
        if(data.type == DT_CLO){
            continuer = 0;
            
            // remove client from list
            lock(&c->shm->mutex_clients);
            memset(c, 0, sizeof(Client));
            close(c->sock_fd);
            c->sock_fd = -1;
            unlock(&c->shm->mutex_clients);

            // decrement load
            lock(&c->shm->mutex_servers);
            c->server->load--;
            unlock(&c->shm->mutex_servers);
        
        }

        // if client want to connect
        else if(data.type == DT_CON){
            memset(c->name, 0, MAX_PSEUDO_LENGTH);
            recv(c->sock_fd, c->name, MAX_PSEUDO_LENGTH-1, 0);
            continue;
        }

        // if client want to get address of another client
        else if(data.type == DT_GET){

            char name[MAX_PSEUDO_LENGTH];
            memset(name, 0, MAX_PSEUDO_LENGTH);
            // receive name
            recv(c->sock_fd, name, MAX_PSEUDO_LENGTH-1, 0);
            Client *c2 = get_client(c->shm,name);
            memset(&data, 0, sizeof(data));

            if(c2 == NULL) 
                data.type = DT_INE;
            else{
                data.type = DT_GET;
                memcpy(&data.origin_addr, &c2->addr, sizeof(Sockaddr_in)); // on envoie l'adresse du client demandé
            }

            send(c->sock_fd, &data, sizeof(data), 0);
        }

        else{
            bruiter(&data.data);
            send(c->server->server_fd, &data, sizeof(data), 0);
        }
    }

    pthread_exit(NULL);
}

void *handle_server_stream(void *arg){
    Server *s = (Server*) arg;
    Data data;
    while(1){
        // receive data from server
        memset(&data, 0, sizeof(data));
        recv(s->server_fd, &data, sizeof(data), 0);
        // send data to client
        int fd = get_client_fd(s->shm, &data.dest_addr);

        // if the dest client is end the connection,
        // we send the message to sender client
        if(fd != -1){
            data.type = data.type == DT_CEX ? DT_EOC : EOF;
            fd = get_client_fd(s->shm, &data.origin_addr);
        }
        send(fd, &data, sizeof(data), 0) ;
    }
    pthread_exit(NULL);
}

void accept_new_client(Proxy *p){
    if(p->shm.nb_clients >= BUFLEN) return;

    memset(&p->shm.clients[p->shm.nb_clients] , 0, sizeof(Client));


    int len = sizeof(struct sockaddr);
    int fd =  accept( p->sock_listen_fd,( struct sockaddr *) &p->shm.clients[p->shm.nb_clients].addr, &len);
    if(fd <= 0){
        fprintf(stderr, "Error accept client ip %s \n", inet_ntoa(p->shm.clients[p->shm.nb_clients].addr.sin_addr));
        perror("accept");
        fflush(stderr);
        return;
    }


    p->shm.clients[p->shm.nb_clients].sock_fd = fd;

    p->shm.clients[p->shm.nb_clients].server = get_server_with_min_load(&p->shm);

    if(p->shm.clients[p->shm.nb_clients].server == NULL) return;
    
    if(pthread_create(p->thread_client+p->shm.nb_clients, NULL, handle_client_stream, p->shm.clients+p->shm.nb_clients)==-1) 
        return;

    p->shm.nb_clients++;
}

void open_new_server(Proxy *p){
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if( fd == -1) return;
    
    printf(" Tentative de connexion au Serveur %d:%s : %d \n",p->shm.nb_servers, inet_ntoa(p->shm.servers[p->shm.nb_servers].addr.sin_addr), ntohs(p->shm.servers[p->shm.nb_servers].addr.sin_port));
    
    if(connect(fd,(struct sockaddr *) &p->shm.servers[p->shm.nb_servers].addr, sizeof(struct sockaddr)) == -1) return;

   p->shm.servers[p->shm.nb_servers].server_fd = fd;

    if(pthread_create(p->thread_server+p->shm.nb_servers, NULL, handle_server_stream,p->shm.servers+p->shm.nb_servers)==-1){
        close(fd);
        return;
    }

    printf(" Connecté un Serveur %d : %s : %d \n",p->shm.nb_servers, inet_ntoa(p->shm.servers[p->shm.nb_servers].addr.sin_addr), ntohs(p->shm.servers[p->shm.nb_servers].addr.sin_port));

    p->shm.nb_servers++;
}

void initialize_proxy(Proxy *p, int argc,char **argv){

    memset(p, 0, sizeof(Proxy));
    printf("initialize_proxy :\n");

    int port;
    if(argc > 2)
        port = atoi(argv[2]);
    if(port <= 0)
        port = DEFAULT_LISTEN_PORT_PROXY;
    else
        port = htons(port);
    

    p->proxy_addr.sin_family = AF_INET;
    p->proxy_addr.sin_port = port;
    p->proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    printf("Configuration du proxy :\n");
    printf("Port d'écoute : %d\n", ntohs(p->proxy_addr.sin_port));
    printf("Adresse d'écoute : %s\n", inet_ntoa(p->proxy_addr.sin_addr));
    fflush(stdout);
    
    CHK(p->sock_listen_fd = socket(AF_INET, SOCK_STREAM, 0));

    pthread_mutex_init(&p->shm.mutex_clients ,NULL);
    pthread_mutex_init(&p->shm.mutex_servers ,NULL);
    p->shm.nb_clients = 0;
    p->shm.nb_servers = 0;


    printf("Lecture de la liste des serveurs :\n");

    FILE * f = NULL;
    PCHK( f = fopen( argv[1], "r" ) ); 

    char line[129];
    char *token = NULL;
    
    while ( !feof(f) ) {

        PCHK(fgets(line, 129, f ));

        lock(&p->shm.mutex_servers);
        memset(&p->shm.servers[p->shm.nb_servers], 0, sizeof(Server));
        p->shm.servers[p->shm.nb_servers].shm = &p->shm;
        p->shm.servers[p->shm.nb_servers].addr.sin_family = AF_INET;
        // format : ip:port
        token = strtok(line, ":");
        if(token == NULL) continue;

        p->shm.servers[p->shm.nb_servers].addr.sin_addr.s_addr = inet_addr(token);

        token = strtok(NULL, "");
        if(token == NULL)
           p->shm.servers[p->shm.nb_servers].addr.sin_port = DEFAULT_EXCHANGE_PORT_SERVER;
        else
           p->shm.servers[p->shm.nb_servers].addr.sin_port = htons(atoi(token));
        
        printf("Serveur %d : %s:%d\n", p->shm.nb_servers, inet_ntoa(p->shm.servers[p->shm.nb_servers].addr.sin_addr), ntohs(p->shm.servers[p->shm.nb_servers].addr.sin_port));
        open_new_server(p);
        unlock(&p->shm.mutex_servers);
    }

    fclose(f);

    if(p->shm.nb_servers == 0){
        raler("Pas de serveur trouvé\n");
    }
    
}

int main(int argc, char **argv){

    Proxy p;

    printf("Proxy démarré\n");

    initialize_proxy(&p, argc, argv);
    printf("Proxy initialisé\n");

    printf("Démarrage de l'écoute ...\n");
    CHK( bind(p.sock_listen_fd, (struct sockaddr *) &p.proxy_addr, sizeof(struct sockaddr)) );
    CHK( listen(p.sock_listen_fd, SOMAXCONN) );

    printf("En attente de clients\n");
    fflush(stdout);

    lock(&p.shm.mutex_clients);
    while (p.shm.nb_clients < BUFLEN) {
        unlock(&p.shm.mutex_clients);

        printf("\t Attente d'une nouvelle connexion\n");
        accept_new_client(&p);
        printf("\t Client %d connecté %s:%d \n", p.shm.nb_clients,inet_ntoa(p.shm.clients[p.shm.nb_clients-1].addr.sin_addr), ntohs(p.shm.clients[p.shm.nb_clients-1].addr.sin_port));
        fflush(stdout);

        lock(&p.shm.mutex_clients);
    }
    unlock(&p.shm.mutex_clients);

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