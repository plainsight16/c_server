#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>

#define PORT "8080"
#define BACKLOG 20
int main(void){
    //getaddrinfo
    //socket
    //bind
    //listen
    //accept
    struct addrinfo hints, *servinfo; // hints gives getaddrinfo what specs I would like in my servinfo struct
    // which begs the question, why not just have one then?
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE; // use local host
    
    int status;
    int sock_fd, new_fd;
    int yes = 1;

    if ((status = getaddrinfo(NULL, PORT,  &hints, &servinfo)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }
    
    sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if (sock_fd == -1){
        perror("Socket error");
        exit(1);
    }
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
        sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    if (bind(sock_fd, servinfo->ai_addr, servinfo->ai_addrlen) == -1){
        close(sock_fd);
        perror("Bind error");
        exit(1);
    }
    freeaddrinfo(servinfo);

    if(listen(sock_fd, BACKLOG) == - 1){
        perror("listen");
        close(sock_fd);
        exit(1);
    }


    
    addr_size = sizeof their_addr;
    while(1){
        printf("Waiting for request...\n");
        new_fd = accept(sock_fd, (struct sockaddr *)&their_addr, &addr_size);
        if(new_fd == -1){
            perror("Accept");
            continue;
        }
        // ------------ Receive intial GET MESSAGE ------------- //

        char buff[1000]; // where did I get 1000 from? fix this.
        int message_size = recv(new_fd, buff, sizeof(buff) - 1, 0);

        if (message_size < 0){
           perror("recv");
           continue; 
        }
        if (message_size == 0){
            perror("recv");
            close(new_fd);
            continue;
        }
        buff[message_size] = '\0';


        // Dealing with File opening and using
        // TODO: convert this whole file handling into a function later on
        FILE *file_d = fopen("index.html", "r");
        if (file_d == NULL){
            perror("file error");
            close(new_fd);
            continue;
        }

        fseek(file_d, 0, SEEK_END);
        long file_size = ftell(file_d);
        rewind(file_d); // to return the pointer file_d to the start of the file

        if (strncmp("GET", buff, 3) == 0){
            printf("Received a get request\n");
            int sec_send_status;
            char response[256];
            snprintf(
                response,
                sizeof(response),
                "HTTP/1.1 200 OK \r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: %ld\r\n"
                "\r\n",
                file_size
            );
            if(send(new_fd, response, strlen(response), 0) == -1){
                perror("Send error: Intital response failed");
                close(new_fd);
            }
            char file_buffer[file_size + 1];
            size_t bytes_read;
            while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file_d)) > 0){
                if(send(new_fd, file_buffer, bytes_read, 0) == -1){
                    perror("Send errror: Error while sending the file over");
                    close(new_fd);
                }
            }
            printf("Responded to GET request\n");
        }
        else{
            printf("ignored, come back with a get request\n");
        }
        close(new_fd);
        
    }
    close(sock_fd);

}