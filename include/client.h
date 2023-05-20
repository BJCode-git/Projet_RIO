#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <fcntl.h>
#include "base.h"
#include "correcteur.h"

#define SEND_TO_CMD "/sd2"
#define P2P_CMD "/p2p"
#define CHAT_CMD "/c"
#define FILE_CMD "/f"
#define CLOSE_CMD "/end"
#define QUIT_CMD "/q"
#define HELP_CMD "/h"

typedef enum {
	UNDEFINED,
	CHAT,
	FTP,
	QUIT
}State;


typedef struct {
	State state;
	int sock_fd;
	int out_fd;
	
	uint8_t size;
	uint8_t ack_until;
	int can_write;

	Sockaddr_in dest_addr;
	Sockaddr_in src_addr;
	Sockaddr_in proxy_addr;

	Mutex mutex;
	Condition ecrire;
	Condition peut_ecrire;
	Data data;
	char buffer[256];
}Shared_memory;

typedef struct {
	char pseudo[MAX_PSEUDO_LENGTH];
	Shared_memory shm;
}Client;


void *thread_client_receive(void *arg);
void *thread_client_send(void *arg);
void *thread_client_resend(void *arg);

void chat(Client *c);
void ftp(Client *c);

void initialize_client(Client *c);

void get_dest_by_name(Client *c);
void read_name(Client *c);
void save_file(Shared_memory *shm);

#endif