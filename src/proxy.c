#include "../include/proxy.h"

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

void *thread_server_send(void *arg){

	Serveur_args *args = (Serveur_args *)arg;
	int sock = args->sock;
	struct sockaddr_in addr = args->addr;
	char buf[BUFLEN];
	int len = sizeof(addr);

	while(1){
		CHK(recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *)&addr, &len));
		printf("Received packet from %s:%d\nData: %s\n\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), buf);
		CHK(sendto(sock, buf, BUFLEN, 0, (struct sockaddr *)&addr, len));
	}

	return pthread_exit(NULL);
}

void *thread_server_read(void *arg){

	Serveur_args *args = (Serveur_args *)arg;
	int sock = args->sock;
	struct sockaddr_in addr = args->addr;
	char buf[BUFLEN];
	int len = sizeof(addr);

	while(1){
		CHK(recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *)&addr, &len));
		printf("Received packet from %s:%d\nData: %s\n\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), buf);
		CHK(sendto(sock, buf, BUFLEN, 0, (struct sockaddr *)&addr, len));
	}

	return pthread_exit(NULL);
}

void initialize_server(Server *s, int port){

    port = port < 0 ? DEFAULT_PORT : port;

     /// Configure server address structure
    s->server_addr.sin_family      = AF_INET;
    s->server_addr.sin_port        = port;
    s->server_addr.sin_addr.s_addr = htonl (INADDR_ANY);


    CHK(s->listening_sock_fd = socket(AF_INET, SOCK_STREAM, 0));
    CHK(bind(s->listening_sock_fd, (struct sockaddr *)& s->server_addr, sizeof(s->server_addr)));
  
    s->shm.users = NULL;
    s->shm.users = malloc(10*sizeof(s->shm.users));
    s->shm.nb_users_allocated = s->shm.users == NULL ? 0 : 10;
    s->shm.nb_users = 0;

    s->shm.buffers = NULL;
    s->shm.buffers = malloc(10*sizeof(s->shm.buffers));
    s->shm.nb_buffers_allocated = s->shm.buffers == NULL ? 0 : 10;
    s->shm.nb_buffers = 0;

    s->shm.mutex = NULL;
    s->shm.mutex = malloc(10*sizeof(s->shm.mutex));
    s->shm.nb_mutex_allocated = s->shm.mutex == NULL ? 0 : 10;
    s->shm.nb_mutex = 0;

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