#ifndef BASE_H
#define BASE_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <stdnoreturn.h>
#include <string.h>

#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define CHK(op) do{ if((op) == -1) {fprintf(stderr,"Erreur ligne %d, dans %s\n",__LINE__,__FILE__);raler(#op);}} while (0)
#define PCHK(op) do{ if((op) == NULL) {fprintf(stderr,"Erreur ligne %d, dans %s\n",__LINE__,__FILE__);raler(#op);}} while (0)
#define TCHK(op) do{ if ((errno = (op)) > 0) {fprintf(stderr,"Erreur ligne %d, dans %s\n",__LINE__,__FILE__);raler(#op);}} while (0)

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
#define cwait(p_cond,p_mutex) TCHK(pthread_cond_wait(p_cond,p_mutex))
#define ctimedwait(p_cond,p_mutex,p_time) TCHK(pthread_cond_timedwait(p_cond,p_mutex,p_time))
#define csignal(p_cond) TCHK(pthread_cond_signal(p_cond))

#define DEFAULT_LISTEN_PORT_PROXY htons(48020)
#define DEFAULT_EXCHANGE_PORT_SERVER htons(48010)
#define DEFAULT_CLIENT_PORT htons(48000)
#define BUFLEN 1024
#define DATALEN 2
#define MAX_PSEUDO_LENGTH 30
#define MAX_FD 1024
#define MAX_ATTEMPTS 3
#define TIMEOUT_MS 133 

typedef struct sockaddr_in Sockaddr_in;

typedef enum {
  DT_CON, // Connection
  DT_CLO, // CLOse connection
  DT_GET, // GET information about a user

  DT_BOC, // Beginning of Chat
  DT_CEX, // Chat EXchange
  DT_EOC, // End Of Chat

  DT_BOF, // Beginning of File
  DT_FEX, // File EXchange
  DT_EOF, // End Of File

  DT_EOJ, // End Of Job

  DT_ACK, // ACKnowledge
  DT_NAK, // Negative AcKnowledge

  DT_INE, // Invalid Name Error
  DT_AEU // Already Existing User
}Data_type;


typedef struct{
  Sockaddr_in origin_addr;
  Sockaddr_in dest_addr;
  Data_type type;
  uint8_t id;
  uint8_t data;
  uint8_t crc;
}Data;

#endif