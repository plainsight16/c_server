#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define PORT "8080"
#define BACKLOG 20
int main(void){
    //getaddrinfo
    //socket
    //bind
    //listen
    //accept
    struct addrinfo hints, *res;
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

    if ((status = getaddrinfo(NULL, PORT,  &hints, &res)) != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }
    
    sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock_fd == -1){
        perror("Socket error");
        exit(1);
    }
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
        sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    if (bind(sock_fd, res->ai_addr, res->ai_addrlen) == -1){
        close(sock_fd);
        perror("Bind error");
        exit(1);
    }
    listen(sock_fd, BACKLOG);


    
    addr_size = sizeof their_addr;
    while(1){
        printf("Waiting for request...\n");
        new_fd = accept(sock_fd, (struct sockaddr *)&their_addr, &addr_size);
        if(new_fd == -1){
            close(new_fd);
            perror("Accept");
            continue;
        }
        // Receive intial GET MESSAGE
        int max_len = 1000;
        char buff[max_len];
        memset(buff, 0, max_len);
   
        
        int message_size = recv(new_fd, buff, max_len, 0);
        if (message_size == -1){
            perror("receive error");
        }
        FILE *file_d = fopen("index.html", "r");
        char *response;
        int sent_bytes;
        if (strncmp("GET", buff, 3) == 0){
            printf("Received a get request\n");
            response = "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "\r\n";
            int first_send_status, sec_send_status;
            if ((first_send_status = send(new_fd, response, strlen(response), 0)) == -1){
                perror("Send error");
                close(new_fd);
            };

            char c;
            while ((c = getc(file_d)) != EOF){
                if ((sec_send_status = send(new_fd, &c, 1, 0)) == -1){
                    perror("Send error");
                    close(new_fd);
                };
            }
        }
        else{
            printf("ignored, come back with a get request\n");
        }
        close(new_fd);
        
    }
    close(sock_fd);

}