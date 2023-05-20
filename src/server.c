#include "../include/server.h"

void test_data_integrity(Data *data){
    
    // test if the data is corrupted, correct it if possible
    if(test_and_correct_crc(&data->data,&data->crc) == -1 ){
        // if the data is corrupted and can't be corrected, we send a NAK
        // proxy should return the packet to the sender
        data->type = NAK;
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

	pthread_exit(NULL);
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
	while(shm->continue_job){

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

	pthread_exit(NULL);
}

void initialize_server(Server *s, int port){

    port = port < 0 ? DEFAULT_EXCHANGE_PORT_SERVER : port;

     /// Configure server address structure
    s->server_addr.sin_family      = AF_INET;
    s->server_addr.sin_port        = port;
    s->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /// Initialize Shared memory
    s->shm.continue_job = 1;
    s->shm.sender_ready = 0;
    s->shm.nb_data_in_buffer = 0;
    memset(s->shm.buffer, 0, BUFLEN);

    pthread_mutex_init(&s->shm.mutex, NULL);
    pthread_cond_init(&s->shm.cond, NULL);
}

void wait_for_proxy(Server *s){
    int sock; // Socket d'écoute
    CHK(sock = socket(AF_INET, SOCK_STREAM, 0));
    CHK(bind(sock, (struct sockaddr *) &s->server_addr, sizeof(s->server_addr)));
    CHK(listen(sock,SOMAXCONN));

    int sinsize =  sizeof(s->proxy_addr);
    CHK(s->shm.proxy_sock_fd = accept(sock, (struct sockaddr *) &s->proxy_addr, &sinsize));
    close(sock);
}

int main(int argc, char **argv){

    int port = argc > 1 ? atoi(argv[1]) : -1;
    
    Server s;
    initialize_server(&s,port);
    wait_for_proxy(&s);

    pthread_t thread_send;
    pthread_t thread_read;

    pthread_create(&thread_send, NULL, &thread_server_send, &s.shm);
    pthread_create(&thread_read, NULL, &thread_server_read, &s.shm);

    pthread_join(thread_send, NULL);
    pthread_join(thread_read, NULL);

    close(s.shm.proxy_sock_fd);
    return 0;
}