#pragma once
#include "base.h"

typedef struct Shared_memory Shared_memory;

typedef struct{
  int server_fd;
  Sockaddr_in server_addr;
  int load;
  Shared_memory *shm;
}Server;

typedef struct{
  int sock_fd;
  Server *server;
  Sockaddr_in addr;
  char name[MAX_PSEUDO_LENGTH];
  Shared_memory *shm;
}Client;

struct Shared_memory{
  Client clients[BUFLEN];
  int nb_clients;
  Mutex mutex_clients;

  Server servers[BUFLEN];
  int nb_serveurs;
  Mutex mutex_servers;
}Shared_memory;

typedef struct{
  int sock_listen_fd;
  Sockaddr_in proxy_addr;
  Thread thread_client[BUFLEN];
  Thread thread_server[BUFLEN];
  Shared_memory shm;
}Proxy;


Client* find_client(Shared_memory *shm, char *pseudo);

Server* get_server_with_min_load(Shared_memory *shm);
int get_client_fd(Shared_memory *shm,Sockaddr_in addr);


void *thread_client(void *arg);
void *thread_server(void *arg);

void initialize_proxy(Server *s, int argc, char **argv);

void accept_new_client(int listen_fd);

//void *server(int argc, char **argv);
