#ifndef SERVER_H

#include "base.h"
#include "correcteur.h"


typedef struct{

  int proxy_sock_fd;
  unsigned int continue_job;
  Data buffer;
  Sockaddr_in proxy_addr;
  Sockaddr_in server_addr;
}Server;


void test_data_integrity(Data *data);

void server_send(Server *s);

void server_read(Server *sm);

void initialize_server(Server *s, int port);

void wait_for_proxy(Server *s);

#endif
