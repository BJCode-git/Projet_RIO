#ifndef PROXY_H
#define PROXY_H

#include "base.h"
#include "correcteur.h"

typedef struct Shared_memory Shared_memory;

typedef struct{

  int server_fd;
  Sockaddr_in addr;
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
  int nb_servers;
  Mutex mutex_servers;

};

typedef struct{
  int sock_listen_fd;
  Sockaddr_in proxy_addr;
  Thread thread_client[BUFLEN];
  Thread thread_server[BUFLEN];
  Shared_memory shm;
}Proxy;


Client* find_client(Shared_memory *shm, char *pseudo);

Server* get_server_with_min_load(Shared_memory *shm);
int get_client_fd(Shared_memory *shm,Sockaddr_in *addr);


void * handle_client_stream(void *arg);
void * handle_server_stream(void *arg);

void initialize_proxy(Proxy *p, int argc, char **argv);

void accept_new_client(Proxy *p);
void open_new_server(Proxy *p);

#endif