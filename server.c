#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>


#define MAX_CLIENTS 200
#define BUFFER_SIZE 2048
#define NAME_LENGTH 100


static _Atomic unsigned int cli_count;
static int uid = 0;

const char *ip = "127.0.0.1";

void print_ip_addr(struct sockaddr_in* client_addr){
    printf("%d.%d.%d.%d",
      client_addr->sin_addr.s_addr& 0xff, 
      (client_addr->sin_addr.s_addr& 0xff00) >> 8,
      (client_addr->sin_addr.s_addr& 0xff0000) >> 16,
      (client_addr->sin_addr.s_addr& 0xff000000) >> 24
    );
}


// client structure
typedef struct{
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char name[NAME_LENGTH];
}client_t;

client_t* clients[MAX_CLIENTS];

pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;






void str_overwrite_stdout(){
    printf("\r%s" ,"> ");
    fflush(stdout);
}

void str_trim_lf(char* arr, int length){
    for(int i = 0; i < length; i++){
        if(arr[i] == '\n'){
            arr[i] = '\0';
            break;
        }
    }
}

void queue_add(client_t* client){
    pthread_mutex_lock(&client_mutex);

    for(int i = 0; i < MAX_CLIENTS; i++){
        if(!clients[i]){
            clients[i] = client;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

void queue_remove(int uid){
    pthread_mutex_lock(&client_mutex);
    for(int i = 0; i < MAX_CLIENTS; i++){
        if(clients[i]->uid == uid){
            clients[i] == NULL;
            break;
        }
    }
    pthread_mutex_unlock(&client_mutex);
}

void send_message(char *s, int uid){
    pthread_mutex_lock(&client_mutex);

    for(int i = 0; i < MAX_CLIENTS; i++){
        if(clients[i]){
            if(clients[i]->uid != uid){
                if(write(clients[i]->sockfd, s, strlen(s)) < 0){
                    printf("Error: write to descriptor\n");
                    break;
                }
            }
        }
    }
     pthread_mutex_unlock(&client_mutex);
}

void* handle_client(void* arg){
    char buffer[BUFFER_SIZE];
    char name[NAME_LENGTH];
    int leave_flag = 0;
    cli_count++;

    client_t* client = (client_t*)arg;

    
    
    if(recv(client->sockfd, name, NAME_LENGTH,0) <= 0 || strlen(name) < 2 || strlen(name) >= NAME_LENGTH - 1){
        printf("Enter the name correctly\n");
        leave_flag = 1;
    }else{
        strcpy(client->name, name);
        sprintf(buffer, "%s has joined\n", client->name);
        printf("%s", buffer);
        send_message(buffer, client->uid);
    }
   
    bzero(buffer, BUFFER_SIZE);

    while(1){
        if(leave_flag){
            break;
        }
    

        int receive = recv(client->sockfd, buffer, BUFFER_SIZE, 0);

        if(receive > 0){
            if(strlen(buffer) > 0){
                send_message(buffer, client->uid);
                str_trim_lf(buffer,strlen(buffer));
                printf("%s -> %s", buffer, client->name);
            }
        }else if(receive == 0 || strcmp(buffer, "exit") == 0){
            sprintf(buffer, "%s has left\n", client->name);
            printf("%s",buffer);
            send_message(buffer, client->uid);
            leave_flag = 1;
        }else{
            printf("ERROR: -1\n");
            leave_flag = 1;
        }
        bzero(buffer, BUFFER_SIZE);
        }

    close(client->sockfd);
    queue_remove(client->uid);
    free(client);
    pthread_detach(pthread_self());

    return NULL;
}



int main(int argc, char** argv){

    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

   
    int port = atoi(argv[1]);
    

    int listenfd = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    pthread_t tid;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);


    //Singals
    signal(SIGPIPE, SIG_IGN);

    
    if(bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        printf("ERROR: bind\n");
        return EXIT_FAILURE;
    };

    if(listen(listenfd, 10) < 0){
        printf("ERROR: listen\n");
        return EXIT_FAILURE;
    }

    printf("CHATROOM\n");

    int connfd = 1;
    while(1){
        socklen_t clien = sizeof(client_addr);
        connfd = accept(listenfd, (struct sockaddr*)& client_addr, &clien);

        // Check for max client
        if(cli_count + 1 == MAX_CLIENTS){
            printf("MAX amount of clients. Connection Rejected");
            print_ip_addr(&client_addr);
            close(connfd);
            continue;
        }

        client_t* new_client = (client_t*)malloc(sizeof(client_t));
        new_client->address = client_addr;
        new_client->sockfd = connfd;
        new_client->uid = uid++;

        // Add client to queue
        queue_add(new_client);
        pthread_create(&tid, NULL, &handle_client, (void*)new_client);

        sleep(1);
    }
    close(listenfd);

    return EXIT_SUCCESS;
}