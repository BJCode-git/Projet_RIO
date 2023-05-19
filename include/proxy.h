#pragma once
#include "base.h"

typedef struct{
  int sock_fd;
  Sockaddr_in server_addr;
  int load;
}Server;

typedef struct{
  int sock_fd;
  Sockaddr_in Addr;
  char name[MAX_PSEUDO_LENGTH];
}Client;

typedef struct {

  Server servers[BUFLEN];
  Client clients[BUFLEN];

  Data receive_buffer[BUFLEN];
  Data send_buffer[BUFLEN];

  Mutex *receive_mutex;
  Mutex *send_mutex;

  int nb_clients;
  int nb_serveurs;
  int nb_mutex;

}Shared_memory;



User *get_user(Shared_memory *shm, char *pseudo);

void *thread_server_TCP(void *arg);

void initialize_server(Server *s, int port);

//void *server(int argc, char **argv);
