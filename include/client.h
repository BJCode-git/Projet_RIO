#pragma once
#include "base.h"
#include "correcteur.h"

#define PSEUDO_CMD "/pseudo"
#define SEND_TO_CMD "/sendto"
#define P2P_CMD "/p2p"
#define HELP_CMD "/help"
#define CHAT_CMD "/chat"
#define FILE_CMD "/file"
#define CLOSE_CMD "/close"
#define QUIT_CMD "/quit"

typedef enum {
	UNDEFINED,
	CHAT,
	FTP
	//SENDING_FILE,
	//RECEIVING_FILE,
	QUIT
}State;


typedef struct {
	State state;
	int sock_fd;
	int out_fd;
	
	uint8_t size;
	uint8_t ack_until;
	int resend;
	int block_write;
	Data_type data_type;
	Sockaddr_in dest_addr;

	Mutex mutex;
	Condition cond;
	Condition peut_ecrire;
	Data buffer[256]; 
}Shared_memory;

typedef struct {
	Sockaddr_in dest_addr;
	Sockaddr_in client_addr;
	Sockaddr_in proxy_addr;
	char pseudo[MAX_PSEUDO_LENGTH];
	Shared_memory shm;
}Client;


void *thread_client_receive(void *arg);
void *thread_client_send(void *arg);

void chat(Client *c);
void ftp(Client *c);

void initialize_client(Client *c);

void get_dest_by_name(Client *c);
void read_name(Client *c);
void save_file(Shared_memory *shm);
