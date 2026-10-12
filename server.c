#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>

#define PORT "8080"
#define BACKLOG 20
#include <errno.h>
#include <sys/socket.h>

int send_all(int fd, const void *data, size_t len)
{
    const char *ptr = data;
    size_t total_sent = 0;

    while (total_sent < len) {
        ssize_t n = send(fd, ptr + total_sent,
                         len - total_sent, 0);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }

        if (n == 0) {
            return -1;
        }

        total_sent += (size_t)n;
    }

    return 0;
}
int main(void){
    //getaddrinfo
    //socket
    //bind
    //listen
    //accept
    struct addrinfo hints, *servinfo, *p; // hints gives getaddrinfo what specs I would like in my servinfo struct
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
    for (p=servinfo; p != NULL; p = p->ai_next){
        sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock_fd == -1){
            continue;
        }
        if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes,
            sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        
        if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1){
            close(sock_fd);
            continue;
        }
        break;
    }
    if (p == NULL){
        fprintf(stderr, "Failed to bind to any available addresses");
        freeaddrinfo(servinfo);
        exit(EXIT_FAILURE);
    }
    
    
    freeaddrinfo(servinfo);

    if(listen(sock_fd, BACKLOG) == - 1){
        perror("listen");
        close(sock_fd);
        exit(1);
    }


    
    
    while(1){
        printf("Waiting for request...\n");
        addr_size = sizeof their_addr;
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
           close(new_fd);
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


        if (strncmp("GET", buff, 3) == 0){
            printf("Received a get request\n");


            FILE *file_d = fopen("index.html", "r");
            if (file_d == NULL){
                perror("file error");
                close(new_fd);
                continue;
            }

            fseek(file_d, 0, SEEK_END);
            if (fseek(file_d, 0, SEEK_END) != 0) {
                perror("fseek");
                fclose(file_d);
                close(new_fd);
                continue;
            }

            long file_size = ftell(file_d);
            if (file_size < 0) {
                perror("ftell");
                fclose(file_d);
                close(new_fd);
                continue;
            }

            rewind(file_d); // to return the pointer file_d to the start of the file


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
            if(send_all(new_fd, response, strlen(response)) == -1){
                perror("Send headers");
                close(new_fd);
                fclose(file_d);
                continue;
            }
            char file_buffer[4096];
            size_t bytes_read;
            while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file_d)) > 0){
                if(send_all(new_fd, file_buffer, bytes_read) == -1){
                    perror("Send files");
                    close(new_fd);
                    continue;
                }
            }
            printf("Responded to GET request\n");
            fclose(file_d);
        }
        else{
            printf("ignored, come back with a get request\n");
        }
        close(new_fd);
        
        
    }
    close(sock_fd);

}