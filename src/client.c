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

	while(shm->state != QUIT){

		memset(&received, 0, sizeof(received));
		recv(shm->sock_fd, &received, sizeof(&received), 0);
		print_data(&received);
		
		lock(&shm->mutex);
		switch(received.type){

			case DT_BOC: // Beginning of Chat
				if(shm->state != UNDEFINED) break;
				shm->out_fd = STDOUT_FILENO;
				shm->state = CHAT;
			break;

			case DT_EOC: // End of Chat
				if(shm->state != CHAT) break;
				shm->out_fd = -1;
				shm->state = UNDEFINED;
			break;

			// normally, the client should not receive these packets
			// they are here for security
			case DT_EOJ: // End Of Job
			break;

			case DT_BOF: // Beginning of File
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
			
			case DT_ACK: // ACKnowledge
				shm->ack_until = received.id > shm->ack_until ? received.id : shm->ack_until;
				// If we received all the ACK, we can write again
				if(shm->ack_until+1 == shm->size){
					shm->ack_until = 0;
					shm->size = 0;
					shm->can_write = 1;
				}
				
			break;
			case DT_NAK: // Negative AcKnowledge
				shm->ack_until = received.id > 0 ? received.id - 1 : 0;
				// tell that the data has been sent
				csignal(&shm->ecrire);
			break;

			default:
				if(shm->out_fd >=0 && (received.type == DT_CEX || received.type == DT_FEX  ) ){
					write(shm->out_fd, &received.data, sizeof(received.data));
					csignal(&shm->ecrire);
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

		cwait(&shm->ecrire, &shm->mutex);
		
		// send data
		shm->can_write = 0;
		for(int i=shm->ack_until; i<shm->size; i++){
			shm->data.id = i;
			shm->data.type = DT_CEX;
			shm->data.data = (uint8_t) shm->buffer[i];
			CHK(send(shm->sock_fd, &shm->data, sizeof(Data), 0));
		}
		printf("Message envoyé\n");
		
    }

    unlock(&shm->mutex);

	pthread_exit(NULL);
}

void *thread_client_resend(void *arg){
	
	Shared_memory *shm = (Shared_memory*) arg;
	uint8_t  ack;
	int attempts = 0;

	lock(&shm->mutex);
	while(shm->state == CHAT){
	
		ack = shm->ack_until;
		unlock(&shm->mutex);

		// wait for an ACK
		usleep(1000*TIMEOUT_MS*2);

		lock(&shm->mutex);

		// if we have not received an ACK, 
		// for a certain time, we resend the data
		if(shm->ack_until == ack && shm->size > 0){
			// if we tried to resend data too many times,
			// we give up
			if(attempts >= MAX_ATTEMPTS){
				shm->ack_until = 0;
				shm->size = 0;
				shm->can_write = 1;
				attempts = 0;
				if(shm->state == FTP){
					shm->state = UNDEFINED;
					close(shm->out_fd);
					shm->out_fd = -1;
					fprintf(stderr, "Erreur lors de l'envoi du fichier\n");
				}
				printf("Erreur lors de l'envoi des données\n");
			}
			else{
				attempts++;
				csignal(&shm->ecrire);
			}
		}
	}

	unlock(&shm->mutex);

	pthread_exit(NULL);
}

void chat(Client *c){

	c->shm.state = CHAT;
	Thread thread_send, thread_receive, thread_resend;
	c->shm.ack_until = 0;
	c->shm.size = 0;
	c->shm.can_write = 1;

	TCHK(pthread_create(&thread_send, NULL, thread_client_send, &c->shm));
	TCHK(pthread_create(&thread_receive, NULL, thread_client_receive, &c->shm));
	TCHK(pthread_create(&thread_resend, NULL, thread_client_resend, &c->shm));

	
	
	// we use a buffer to for the interaction with the user
	// which allow him to write his message while the other is writing
	// and quit at anytime without having to wait that the data
	// has been correctly sent
	int taille = strlen(c->pseudo);
	int size = taille + 2;
	char buffer[256];
	memcpy(buffer, c->pseudo, taille);

	buffer[taille+1] = '>';
	taille+=2;
	
	// send the beginning of chat
	c->shm.state = CHAT;
	c->shm.data.type = DT_BOC;
	CHK(send(c->shm.sock_fd, &c->shm.data, sizeof(c->shm.data), 0));


	lock(&c->shm.mutex);
	while(c->shm.state == CHAT){
		unlock(&c->shm.mutex);
		printf("\n%s > ", c->pseudo);
		fflush(stdout);

		memset(buffer + taille, 0, 256 - taille);
		fgets(buffer + taille, 256 -1 - taille, stdin);

		if(strncmp(buffer + taille, QUIT_CMD, sizeof(QUIT_CMD)) == 0){ 
			c->shm.state = QUIT;
			csignal(&c->shm.ecrire);
			printf(" -> Quitting...\n");
			break;
		}
		if(strncmp(buffer + taille, CLOSE_CMD, sizeof(CLOSE_CMD)) == 0){
			c->shm.state = QUIT;
			csignal(&c->shm.ecrire);
			printf(" -> Closing...\n");
			break;
		}

		size = strlen(buffer);
		
		lock(&c->shm.mutex);
		if(c->shm.can_write == 1){
			memcpy(c->shm.buffer, buffer, size);
			c->shm.size = size;
			csignal(&c->shm.ecrire);
			printf(" -> Envoi...\n");
			fflush(stdout);
		}
	}

	c->shm.data.type = DT_EOC;
	CHK(send(c->shm.sock_fd, &c->shm.data, sizeof(c->shm.data), 0));
	unlock(&c->shm.mutex);

	pthread_join(thread_send, NULL);
	pthread_join(thread_receive, NULL);
}

void ftp(Client *c){
	c->shm.state = FTP;
	printf("Implementation incomplete\n");
}

void initialize_client(Client *c, char *ip_addr, int port){


	memset(c, 0, sizeof(Client));

	c->shm.state = UNDEFINED;
	c->shm.out_fd = -1;
	c->shm.sock_fd = -1;

	pthread_mutex_init(&c->shm.mutex, NULL);
	pthread_cond_init(&c->shm.ecrire, NULL);

	c->proxy_addr.sin_family = AF_INET;
	c->proxy_addr.sin_port = port > 0 ? htons(port) : DEFAULT_LISTEN_PORT_PROXY;

	if( inet_aton(ip_addr, &c->proxy_addr.sin_addr) ==0 ){
		fprintf(stderr, "Erreur lors de la conversion de l'adresse IP\n");
		exit(EXIT_FAILURE);
	}
	

	c->dest_addr.sin_family = AF_INET;
	c->dest_addr.sin_port = DEFAULT_CLIENT_PORT;
}

void connect_to_proxy(Client *c){

	// create the socket
	CHK(c->shm.sock_fd = socket(AF_INET, SOCK_STREAM, 0));
	
	// connect to the proxy
	int i;
	for(i = 0; i < MAX_ATTEMPTS; i++){
		printf("Tentative de connexion au proxy...\n");

		if(connect(c->shm.sock_fd, (struct sockaddr *) &c->proxy_addr, sizeof(struct sockaddr))==-1){
			perror("Erreur de connexion au proxy, nouvelle tentative dans 1 seconde...\n");
			sleep(1);
		}
		else
			break;
		
	}

	if(i == MAX_ATTEMPTS){
		CHK(close(c->shm.sock_fd));
		raler("Impossible de se connecter au proxy\n");
	}

	printf("Connexion au proxy reussie\n");
	
	// send the pseudo
	read_name(c);
	CHK(send(c->shm.sock_fd, c->pseudo, MAX_PSEUDO_LENGTH, 0));
}

void get_receiver(Client *c){

	int continuer = 1;

	while(continuer){
		printf("Entrez le pseudo ou l'adresse ip du destinataire : \n");
		memset(c->shm.buffer, 0, 128);
		PCHK(fgets(c->shm.buffer, 128, stdin));

		if( inet_aton(c->shm.buffer, &c->dest_addr.sin_addr)!=0 ){

			memset(c->shm.buffer, 0, 8);
			printf("Entrez le port du destinataire : \n");
			PCHK(fgets(c->shm.buffer, 7, stdin));

			c->dest_addr.sin_port = htons(atoi(c->shm.buffer));
			if(c->dest_addr.sin_port < 0 || c->dest_addr.sin_port > 65535){
				c->dest_addr.sin_port = DEFAULT_CLIENT_PORT;
			}

			c->dest_addr.sin_family = AF_INET;
			continuer = 0;
		}
		else{
			// send the pseudo
			
			memset(&c->shm.data, 0, sizeof(c->shm.data));
			c->shm.data.type = DT_GET;

			// send query to the proxy
			CHK(send(c->shm.sock_fd,&c->shm.data,sizeof(c->shm.data), 0));

			// send the pseudo
			CHK(send(c->shm.sock_fd,c->shm.buffer, MAX_PSEUDO_LENGTH, 0));

			// receive the answer
			CHK(recv(c->shm.sock_fd, &c->shm.data, sizeof(&c->shm.data) , 0));

			if(c->shm.data.type == DT_INE)
				printf("Utilisateur non trouvé \n");
			else{
				printf("Utilisateur trouvé \n");
				memcpy(&c->dest_addr, &c->shm.data.origin_addr, sizeof(c->dest_addr));
				continuer = 0;
			}
		}

	}
}

void configure_data(Client *c){
	memset(&c->shm.data, 0, sizeof(c->shm.data));
	// don't care, the proxy will fill it with the right value
	memset(&c->shm.data.origin_addr, 0, sizeof(c->shm.data.origin_addr));
	memcpy(&c->shm.data.dest_addr, &c->dest_addr, sizeof(c->dest_addr));
}

int main(int argc, char **argv){

	if(argc < 2){
		printf("Usage : %s <adresse proxy> <port proxy>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	Client c;
	initialize_client(&c, argv[1], atoi(argv[2]));
	connect_to_proxy(&c);

	int continuer = 1;
	while(continuer){
		get_receiver(&c);
		configure_data(&c);
		printf("Que voulez-vous faire ?\n");
		printf("1 - Chat\n");
		printf("2 - FTP\n");
		printf("3 - Quitter\n");
		fgets(c.shm.buffer, 255, stdin);
		switch(atoi(c.shm.buffer)){
			case 1:
				chat(&c);
			break;
			case 2:
				ftp(&c);
			break;
			case 3:
				continuer = 0;
			break;
			default:
				printf("Choix invalide\n");
		}
	}
	
	return 0;
}