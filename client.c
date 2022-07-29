#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>

#define NAME_LENGTH 100
#define BUFFER_SIZE 2048

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[NAME_LENGTH];

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

void catch_ctrl_c(){
    flag = 1;
}


void recv_msg_handler(){
    char message[BUFFER_SIZE] = {};
    while(1){
        int receive = recv(sockfd, message, BUFFER_SIZE, 0);

        if(receive > 0){
            printf("%s ", message);
            str_overwrite_stdout();
        }else if( receive == 0){
            break;
        }
        bzero(message, BUFFER_SIZE);
    }
}

void send_msg_handler(){
    char buffer[BUFFER_SIZE] = {};
    char message[BUFFER_SIZE + NAME_LENGTH] = {};

    while(1){
        str_overwrite_stdout();
        fgets(buffer, BUFFER_SIZE, stdin);
        str_trim_lf(buffer, BUFFER_SIZE);
         
        if(strcmp(buffer, "exit") == 0)
            break;
        else{
            sprintf(message, "%s: %s\n", name, buffer);
            send(sockfd, message, strlen(message), NULL);
        }
        bzero(buffer, BUFFER_SIZE);
        bzero(message, BUFFER_SIZE + NAME_LENGTH);
    }

    catch_ctrl_c(2);
}

int main(int argc, char** argv){

    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* ip = "127.0.0.1";
    int port = atoi(argv[1]);

    
    signal(SIGINT, catch_ctrl_c);

    printf("Enter your name: ");
    fgets(name, NAME_LENGTH, stdin);
    str_trim_lf(name, strlen(name));

    if(strlen(name) > NAME_LENGTH - 1 || strlen(name) < 2){
        printf("Enter name correctly\n");
        return EXIT_FAILURE;
    }
   
   struct sockaddr_in server_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);


    // connection
    if(connect(sockfd, (struct sockaddr*)& server_addr, sizeof(server_addr)) < 0){
        printf("ERROR: connection to the server\n");
        return EXIT_FAILURE;
    }

    //s end the name
    send(sockfd, name, NAME_LENGTH, 0);

    printf("CHATROOM\n");

    pthread_t send_msg_thread;
    if(pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, NULL) != 0){
        printf("Error thread\n>");
        return EXIT_FAILURE;
    };

    pthread_t recv_msg_thread;
    if(pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0){
        printf("Error thread\n>");
        return EXIT_FAILURE;
    };

    while(1){
        if(flag){
            printf("\nBYE\n");
            break;
        }
    }

    close(sockfd);
    return EXIT_SUCCESS;
}