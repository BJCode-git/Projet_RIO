#include "../include/server.h"

void test_data_integrity(Data *data){
    uint16_t check = concat(data->data, data->crc);
    if(crcVerif(check) == -1 ){
        data->type = NAK;
        // proxy should return the packet to the sender
        memcpy(data->dest_addr, data->origin_addr, sizeof(data->dest_addr));
    }
}

void *thread_server_send(void *arg){

	Shared_memory *shm = (Shared_memory*) arg;

    lock(&shm->mutex);
    shm->sender_ready = 1;

	while(shm->continue_job){

		wait(&shm->cond,&shm->mutex);

        if(shm->continue_job == 0) break;
        if(shm->nb_data_in_buffer == 0) continue;

        CHK(send(shm->proxy_sock_fd, shm->buffer, shm->nb_data_in_buffer*sizeof(Data), 0));

        // send ACK to the sender
        for(int i = 0; i < shm->nb_data_in_buffer; i++){
            shm->buffer[i]->type = ACK;
            memcpy(data->dest_addr, data->origin_addr, sizeof(data->dest_addr));
        }
        CHK(send(shm->proxy_sock_fd, shm->buffer, shm->nb_data_in_buffer*sizeof(Data), 0));

        shm->nb_data_in_buffer = 0;

        // tell that the data has been sent
        csignal(&shm->cond);
    }

    unlock(&shm->mutex);

	return pthread_exit(NULL);
}

void *thread_server_read(void *arg){
    Shared_memory *shm = (Shared_memory*) arg;
    Data received;

    // wait for the sender to be ready
    lock(&shm->mutex);
    while(shm->sender_ready!=1){
        cwait(&shm->cond, &shm->mutex);
    }
    unlock(&shm->mutex);

    clock_t last_send_time = clock();
	while(s->shm.continue_job){

        memset(&received, 0, sizeof(received));
        recv(shm->proxy_sock_fd, &received, sizeof(&received), 0);

        // test integrity of the data
        test_data_integrity(&received);

        // test if the proxy is sending an EOJ
        if(received.type == EOJ){
            lock(&shm->mutex);
            shm->continue_job = 0;
            csignal(&shm->cond);
            unlock(&shm->mutex);
            break;
        }

        lock(&shm->mutex);

        memcpy(shm->buffer+shm->nb_data_in_buffer, &received, sizeof(received));
        shm->nb_data_in_buffer = shm->nb_data_in_buffer == BUFLEN ? BUFLEN : shm->nb_data_in_buffer + 1;

        // if the buffer is full or the timeout is reached, send data to the proxy
        if(shm->nb_data_in_buffer == BUFLEN || last_send_time + 10*TIMEOUT_MS < clock()  ){
            last_send_time = clock();
            csignal(&shm->cond);
            // wait for the data to be sent
            cwait(&shm->cond, &shm->mutex);
        }

        unlock(&shm->mutex);
	}

	return pthread_exit(NULL);
}

void initialize_server(Server *s, int port){

    port = port < 0 ? DEFAULT_PORT : port;

     /// Configure server address structure
    s->server_addr.sin_family      = AF_INET;
    s->server_addr.sin_port        = port;
    s->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /// Initialize Shared memory
    s->shm.continue_job = 1;
    s->shm.sender_ready = 0;
    s->shm.nb_data_in_buffer = 0;
    memset(s->shm.buffer, 0, BUFLEN);

    s->shm.mutex = MUTEX_INITIALIZER;
    s->shm.cond = COND_INITIALIZER;
}

void wait_for_proxy(Server *s){
    int sock; // Socket d'écoute
    CHK(sock = socket(AF_INET, SOCK_STREAM, 0));
    CHK(bind(sock, &s->server_addr, sizeof(s->server_addr)));
    CHK(listen(sock,SO_MAXCONN));

    int sinsize =  sizeof(s->proxy_addr);
    CHK(s->shm.proxy_sock_fd = accept(sock, &s->proxy_addr, &sinsize));
    close(sock);
}

void main(int argc, char **argv){
    
    Server s;
    initialize_server(&s);
    wait_for_proxy(&s);

    pthread_t thread_server_send;
    pthread_t thread_server_read;

    pthread_create(&thread_server_send, NULL, thread_server_send, &s->shm);
    pthread_create(&thread_server_read, NULL, thread_server_read, &s->shm);

    pthread_join(thread_server_send, NULL);
    pthread_join(thread_server_read, NULL);

    close(s->proxy_sock_fd);
    return 0;
}