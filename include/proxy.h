#pragma once
#include "base.h"

typedef struct{
  int sock_fd;
  Sockaddr_in server_addr;
  int load;
}Server;

typedef struct{
  int sock_fd;
  int server_fd;
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


Client* find_client(Shared_memory *shm, char *pseudo);
int get_server_fd(Shared_memory *shm);

void *thread_client(void *arg);
void *thread_server(void *arg);

void initialize_proxy(Server *s, int argc, char **argv);

void accept_new_client(int listen_fd);

//void *server(int argc, char **argv);
