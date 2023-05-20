#include "../include/client.h"

void read_name(Client *c){
	// read the name from stdin
	printf("Entrez votre pseudo : \n");
	memset(c->pseudo, 0, MAX_PSEUDO_LENGTH);
	fgets(c->pseudo, MAX_PSEUDO_LENGTH-1, stdin);
}

void save_file(Shared_memory *shm){
	// read the filepath from stdin
	// need to be protected by a mutex
	
	memset(shm->buffer, 0, 256);
	fgets(shm->buffer, 256, stdin);
	
	if( (shm->out_fd = open(shm->buffer,  O_CREAT | O_WRONLY | O_TRUNC , 0666)) == -1 ){
		perror("Erreur lors de l'ouverture du fichier, sauvegarde vers stdout\n");
		shm->out_fd = STDOUT_FILENO;
	}

}

void *thread_client_receive(void *arg){

	Shared_memory *shm = (Shared_memory*) arg;
	Data received;
	struct timespec start = clo, stop;
	clock_gettime (CLOCK_MONOTONIC, &start);

	while(shm->state != QUIT){

		memset(&received, 0, sizeof(received));
		recv(shm->sock_fd, &received, sizeof(&received), 0);
		
		lock(&shm->mutex);
		switch(received.type){

			case BOC: // Beginning of Chat
				if(shm->state != UNDEFINED) break;
				shm->out_fd = STDOUT_FILENO;
				shm->state = CHAT;
			break;
			case EOC: // End of Chat
				if(shm->state != CHAT) break;
				shm->out_fd = -1;
				shm->state = UNDEFINED;
			break;

			// normally, the client should not receive these packets
			// they are here for security
			case EOJ: // End Of Job
			break;

			case BOF: // Beginning of File
				if(shm->state != UNDEFINED) break;
				save_file(shm);
				shm->state = FTP;
			break;

			case EOF: // End of File
				if(shm->state != FTP) break;
				close(shm->out_fd);
				shm->out_fd = -1;
				shm->state = UNDEFINED;
			break;

			case ACK: // ACKnowledge
				shm->ack_until = received.id > shm->ack_until ? received.id : shm->ack_until;
				if(shm->ack_until+1 == shm->size){
					shm->resend = 0;
					shm->ack_until = 0;
					csignal(&shm->peut_ecrire);
				}
				
				if( clock_gettime( CLOCK_MONOTONIC, &stop) != -1 ){
					if( (stop.tv_sec - start.tv_sec) * 1000 + (stop.tv_nsec - start.tv_nsec) / 1000000 > 1000 ){
						shm->resend = 1;
						shm->ack_until = received.id > 0 ? received.id - 1 : 0;
					}
					csignal(&shm->cond);
				}
				
			break;
			case NAK: // Negative AcKnowledge
				shm->resend = 1;
				shm->ack_until = received.id > 0 ? received.id - 1 : 0;
				// tell that the data has been sent
				csignal(&shm->cond);
			break;

			default:
				if(shm->out_fd >=0){
					write(shm->out_fd, received.data, sizeof(received.data));
					csignal(&shm->cond);
				}
		}
		unlock(&shm->mutex);
	}

	pthread_exit(NULL);
}

void *thread_client_send(void *arg){

	Shared_memory *shm = (Shared_memory*) arg;
	clock_t send_time;

	lock(&shm->mutex);
	while(shm->state == CHAT){

		wait(&shm->cond);

		// send data
		if(shm->state == CHAT && shm->size > 0){
			int offset = shm->resend == 1 ? 0 : shm->ack_until + 1;
        	CHK(send(shm->sock_fd, shm->buffer+ offset, shm->size*sizeof(Data), 0));
		}

    }

    unlock(&shm->mutex);

	return pthread_exit(NULL);
}

void chat(Client *c){

	Thread *thread_send, Thread *thread_receive;
	TCHK(pthread_create(&thread_send, NULL, thread_client_send, (void*)c));
	TCHK(pthread_create(&thread_receive, NULL, thread_client_receive, (void*)c));

	int taille = strlen(shm.buffer);
	for(int i=0; i<taille+1; i++){
			c->shm.buffer[i].data = (uint8_t )c->pseudo[i];
	}
	c->shm.buffer[taille+1] = '>';
	c->shm.buffer[taille+2] = ' ';
	c->shm.size = taille+2;
	c->shm.state = CHAT;

	while(c->shm.state == CHAT ){
		printf("%s > ", c->pseudo);
		fflush(stdout);

		lock(&c->shm.mutex);
		fgets(c->shm.buffer+c->shm.size, BUFLEN - c->shm.size, stdin);
		c->shm.size += strlen(c->shm.buffer);
		cwait(&shm->peut_ecrire)

		unlock(&c->shm.mutex);
	}

	pthread_join(thread_send, NULL);
	pthread_join(thread_receive, NULL);
}

void ftp(Client *c){}

void initialize_client(Client *c){

	read_name(&c);
	
	c->proxy_addr.sin_addr.s_addr = INADDR_NONE;
	while(INADDR_NONE == c->shm.server.sin_addr.s_addr ){
		printf("Entrez l'adresse du proxy : \n");
		fgets(c->shm.buffer, 255, stdin);
		c->proxy_addr.sin_addr.s_addr = inet_addr(c->shm.buffer);
	}
	
	c->proxy_addr.sin_port = -1;
	while(c->proxy_addr.sin_port < 0 || c->proxy_addr.sin_port > 65535){
		printf("Entrez le port du proxy : \n");
		fgets(c->shm.buffer, 7, stdin);
		c->proxy_addr.sin_port = htons(atoi(c->shm.buffer));
		read_port(&c);
	}
	// initialize the proxy address
	c->proxy_addr.sin_family = AF_INET;

	//remplit la structure sockadd pour moi
	memset(&c->client_addr, 0, sizeof(c->client_addr));
	c->client_addr.sin_family = AF_INET;
	c->client_addr.sin_port = htons(DEFAULT_CLIENT_PORT);
	c->client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	printf("Entrez le port du serveur : \n");
	read_port(&c);

}

void connect_to_proxy(Client *c){
	
	// create the socket
	CHK(c->shm.sock_fd = socket(AF_INET, SOCK_STREAM, 0));

	// connect to the proxy
	CHK(connect(c->shm.sock_fd, &c->proxy_addr, sizeof(c->server)));

	// send the pseudo
	//CHK(send(c->shm.sock_fd, c->pseudo, MAX_PSEUDO_LENGTH, 0));
}

int main(int argc, char **argv){

	Client c;
	initialize_client(&c);
	connect_to_proxy(&c);

	chat(&c);

	return 0;
}