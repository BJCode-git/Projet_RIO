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

typedef struct{
  int sock_fd;
  Sockaddr_in server_addr;
  int load;
}Server;

typedef struct{
  int sock_fd;
  Sockaddr_in addr;
  char name[MAX_PSEUDO_LENGTH];
}Client;

typedef struct{
  int sock_listen_fd;
  Sockaddr_in proxy_addr;
  Thread thread_client[BUFLEN];
  Thread thread_server[BUFLEN];

  Client clients[BUFLEN];
  int nb_clients;
  Mutex mutex_clients;

  Server servers[BUFLEN];
  int nb_serveurs;
  Mutex mutex_servers;
  
}Proxy;


Client* get_client(Proxy *p, char *pseudo){
    lock(&p->mutex_clients);
    for(int i = 0 ; i< p->nb_clients;i++ ){
        if(strncmp(p->clients[i].pseudo,pseudo, MAX_PSEUDO_LENGTH)==0){
            unlock(&p->mutex_clients);
            return clients + i;
        }
    }
    unlock(&shm->mutex);
    return NULL;
}

void *thread_client(void *arg){
    
    Client *c = (Client*) arg;
    Data data;
    int continuer =1;
    // receive pseudo
    recv(c->sock_fd, c->name, MAX_PSEUDO_LENGTH, 0);

    while(continuer){
        memset(&data, 0, sizeof(data));
        recv(c->sock_fd, &data, sizeof(data), 0);

        if(data.type == CON){
            memset(c->pseudo, 0, MAX_PSEUDO_LENGTH);
            recv(c->sock_fd, c->pseudo, MAX_MESSAGE_LENGTH-1, 0);
            continue;
        }

        else if(data.type == GET){

            char name[MAX_PSEUDO_LENGTH];
            memset(name, 0, MAX_PSEUDO_LENGTH);
            recv(c->sock_fd, name, MAX_PSEUDO_LENGTH-1, 0);

            Client *c2 = get_client(name);
            memset(&data, 0, sizeof(data));

            if(c2 == NULL) 
                data.type = INE;
            else{
                data.type = GET;
                memcpy(&data.origin_addr, &c2->addr, sizeof(Sockaddr_in)); // on envoie l'adresse du client demandé
            }

            send(c->sock_fd, &data, sizeof(data), 0);

        }
        
        else
            send(c->server_fd, &data, sizeof(data), 0);
    }

    return pthread_exit(NULL);
}

void *thread_server(void *arg){

}

void initialize_proxy(Proxy *p, int argc, char **argv){

    shm->mutex_clients = MUTEX_INITIALIZER;
    shm->receive_mutex = MUTEX_INITIALIZER;
    shm->send_mutex = MUTEX_INITIALIZER;
    shm->nb_clients;
    shm->nb_serveurs;
    shm->nb_mutex;
}

void accept_new_client(Proxy *p){

    memset(p->clients[p->nb_clients] , 0, sizeof(Client));

    int fd =  accept( p->sock_listen_fd, &p->clients[p->nb_clients].addr, sizeof(struct sockaddr));
    if(fd == -1) return;
    p->clients[p->nb_clients].sock_fd = fd;
    
    if(pthread_create(p->thread_client+p->nb_clients, NULL, thread_client, p->clients+p->nb_clients)==-1) 
        return;

    p->nb_clients++;
}

void main(int argc, char **argv){
    
    Server s;
    initialize_server(&s);

    // must connect to the proxy server

    while (1) {

        // Prepare a thread to serve the connection 
        CHK(dynamic_realloc(&threads, &nb_threads_allocated, nb_threads + 1, sizeof(pthread_t))) ;

        /* Accept connection to client. */
        Sockaddr_in client_address;
        int Sockaddr_len = sizeof(client_address);
        new_socket_fd = accept(socket_fd, (struct sockaddr *)&pthread_arg->client_address, &Sockaddr_len);
        if (new_socket_fd == -1) {
            perror("accept");
            break;
        }

        

        int res = -1;

        /* try to allocate a new buffers */
        res = dynamic_realloc(&shm.buffers, &shm.nb_buffers_allocated, shm.nb_buffers + 1, sizeof(char [BUFLEN]));

        /* try to allocate as many mutex as buffers*/
        if(res =!= -1){

            res = dynamic_realloc(&shm.mutex, &shm.nb_mutex_allocated, shm.nb_mutex + 1, sizeof(pthread_mutex_t));
            if( res == -1){
                // si on ne peut pas allouer de mutex, on rend le buffer indisponible
                shm.nb_buffers--;
            }
        }

        
    
    }

    for(int i = 0; i < shm.nb_connections; i++) pthread_join(shm.threads[i], NULL);


}