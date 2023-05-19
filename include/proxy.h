#pragma once
#include "base.h"

typedef struct{
  char pseudo[MAX_PSEUDO_LENGTH];
  Sockaddr_in Addr;
  int sock;
}Client;

typedef char buff[BUFLEN] Buffer;
typedef struct Thread_server_args;

typedef struct {

  Thread_server_args *users;
  Buffer *buffers;
  Mutex *mutex;

  int nb_users;
  int nb_users_allocated;

  int nb_buffers;
  int nb_buffers_allocated;

  int nb_mutex;
  int nb_mutex_allocated;

}Shared_memory;

struct Thread_server_args{

  Shared_memory *shm;
  int user_sock;
  int thread;
  int buffer_attributed;
  Sockaddr_in user_addr;
  Sockaddr_in server_addr;
};

typedef struct{
  int listening_sock_fd;
  Sockaddr_in server_addr;
  Shared_memory shm;
}Server;


typedef struct{
  int sock_fd;
  Sockaddr_in Addr;
}Client;

User *get_user(Shared_memory *shm, char *pseudo);

void *thread_server_TCP(void *arg);

void initialize_server(Server *s, int port);

//void *server(int argc, char **argv);
