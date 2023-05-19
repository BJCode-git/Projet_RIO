#pragma once
#include "base.h"
#include "correcteur.h"

typedef struct Thread_server_args;


typedef struct {
  
  int proxy_sock_fd;
  unsigned int continue_job;
  unsigned int nb_data_in_buffer;
  unsigned int sender_ready;

  Mutex mutex;
  Condition cond;
  Data buffer[BUFLEN];
}Shared_memory;

typedef struct{
  Sockaddr_in proxy_addr;
  Sockaddr_in server_addr;
  Shared_memory shm;
}Server;


void test_data_integrity(Data *data);

void *thread_server_send(void *arg);

void *thread_server_read(void *arg);

void initialize_server(Server *s, int port);

void wait_for_proxy(Server *s);



//void *server(int argc, char **argv);
