#include "../include/server.h"

void test_data_integrity(Data *data){
    
    // test if the data is corrupted, correct it if possible
    uint8_t m = data->data;
    uint8_t crc = data->crc;

    if(test_and_correct_crc(&m,&crc) == -1 ){
        printf("\t Correction impossible\n");
        // if the data is corrupted and can't be corrected, we send a DT_NAK
        // proxy should return the packet to the sender

        data->type = DT_NAK;
        memcpy(&data->dest_addr, &data->origin_addr, sizeof(Sockaddr_in));
    }

    data->data = m;
    data->crc = crc;
}

void server_send(Server *s){

    if(s->continue_job == 0) return;

    // send the data to the proxy
    printf("Paquet à envoyer au proxy\n");
    print_data(&s->buffer);
    send(s->proxy_sock_fd,&s->buffer, sizeof(Data), 0);
    

    // if the data  is not corrupted and is an DT_CEX or DT_FEX, 
    // we also send an DT_ACK
    // to the sender
    if(s->buffer.type == DT_CEX || s->buffer.type == DT_FEX){

        s->buffer.type = DT_ACK;
        memcpy(&s->buffer.dest_addr, &s->buffer.origin_addr, sizeof(Sockaddr_in));
        send(s->proxy_sock_fd,&s->buffer, sizeof(Data), 0);
        printf("Paquet ACK envoyé au proxy\n");
        print_data(&s->buffer);
    }

}

void server_read(Server *s){
    
    memset(&s->buffer, 0, sizeof(Data));

    CHK(recv(s->proxy_sock_fd, &s->buffer, sizeof(Data), 0));
    printf("Paquet reçu du proxy\n");
    print_data(&s->buffer);
    Data_type type = s->buffer.type;

    // test if the proxy is sending an DT_EOJ
    if(type == DT_EOJ){
        s->continue_job = 0;
        printf("Le proxy a fermé la connexion\n");
        return;
    }

    // test integrity of the data
    if(type == DT_CEX || s->buffer.type == DT_FEX)
        test_data_integrity(&s->buffer);

 
}

void initialize_server(Server *s, int port){
   
    s->continue_job = 1;
    s->proxy_sock_fd = -1;
    memset(&s->buffer, 0, sizeof(Data));
    memset(&s->proxy_addr, 0, sizeof(Sockaddr_in));
    memset(&s->server_addr, 0, sizeof(Sockaddr_in));

    s->proxy_addr.sin_family      = AF_INET;

    port = port < 0 ? DEFAULT_EXCHANGE_PORT_SERVER : htons(port);

     /// Configure server address structure
    s->server_addr.sin_family      = AF_INET;
    s->server_addr.sin_port        = port;
    s->server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    printf("Addresse d'écoute serveur : %s\n", inet_ntoa(s->server_addr.sin_addr));
    printf("Port d'écoute du serveur : %d\n", ntohs(s->server_addr.sin_port));

}

void wait_for_proxy(Server *s){
    int sock; 

    CHK(sock = socket(AF_INET, SOCK_STREAM, 0));
    
    CHK(bind(sock, (struct sockaddr *) &s->server_addr, sizeof(s->server_addr)));
    CHK(listen(sock,1));

    printf("En attente de connexion du proxy...\n");

    socklen_t sinsize =  sizeof(s->proxy_addr);
    CHK(s->proxy_sock_fd = accept(sock, (struct sockaddr *) &s->proxy_addr, &sinsize));
    close(sock);

    printf("Connexion du proxy réussie\n");
    printf("Proxy connecté %s:%d\n", inet_ntoa(s->proxy_addr.sin_addr),ntohs(s->proxy_addr.sin_port));
    fflush(stdout);
}

int main(int argc, char **argv){

    int port = argc > 1 ? atoi(argv[1]) : -1;
    
    Server s;

    printf("Initialisation du serveur ...\n");
    initialize_server(&s,port);
    printf("Initialisation du serveur réussie\n");

    wait_for_proxy(&s);

  
    printf("Communication du serveur sur %s:%d \n", inet_ntoa(s.server_addr.sin_addr),ntohs(s.server_addr.sin_port));

    while(s.continue_job == 1){
        server_read(&s);
        server_send(&s);
    }


    close(s.proxy_sock_fd);
    return 0;
}