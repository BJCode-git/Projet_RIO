#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdnoreturn.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define CHK(op) do{ if((op) == -1) raler(#op);} while (0)
#define PCHK(op) do{ if((op) == NULL) raler(#op);} while (0)
#define TCHK(op) do{ if ((errno = (op)) > 0) raler(#op);} while (0)

void raler(const char* msg){
  fprintf(stderr,"%s",msg);
  if(errno!=0)
    perror("Error : ");
  exit(EXIT_FAILURE);
}

#define lock(p_mutex) TCHK(pthread_mutex_lock(p_mutex))
#define unlock(p_mutex) TCHK(pthread_mutex_unlock(p_mutex))
#define Mutex pthread_mutex_t

#define Thread pthread_t

#define Condition pthread_cond_t
#define wait(p_cond,p_mutex) TCHK(pthread_cond_wait(p_cond,p_mutex))
#define timedwait(p_cond,p_mutex,p_time) TCHK(pthread_cond_timedwait(p_cond,p_mutex,p_time))
#define signal(p_cond) TCHK(pthread_cond_signal(p_cond))

#define DEFAULT_LISTEN_PORT_PROXY 52000
#define DEFAULT_EXCHANGE_PORT_SERVER 52001
#define DEFAULT_CLIENT_PORT 48000
#define BUFLEN 1024
#define DATALEN 2
#define MAX_PSEUDO_LENGTH 30
#define MAX_FD 1024
#define MAX_ATTEMPTS 3
#define TIMEOUT_MS 133 

typedef struct sockaddr_in Sockaddr_in;

typedef enum {
  CON = 0, // connect to the proxy
  GET, // get ip from name 
  BOC, // Beginning of Chat
  CEX, // Chat EXchange
  EOC, // End of Chat

  BOF, // Beginning of File
  FEX, // File EXchange
  EOF, // End of File
  
  EOJ, // End Of Job
  //ACK = 7, // ACKnowledge
  NAK, // Negative AcKnowledge
  INE, // Invalid Name Error
  AEU // Already Existing User

}Data_type;


typedef struct{
  Sockaddr_in origin_addr;
  Sockaddr_in dest_addr;
  Data_type type;
  uint8_t id;
  uint8_t data;
  uint8_t crc;
}Data;

