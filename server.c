#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>

#define PORT "8080"
#define BACKLOG 20

//------NETWORKING UTILITY-----
int send_all(int fd, const void *data, size_t len){
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

//-------SERVER SETUP -------
int create_listener(void){
    //does the following
    //getaddrinfo
    //socket
    //bind
    //listen

    struct addrinfo hints, *servinfo, *p; // hints gives getaddrinfo what specs I would like in my servinfo struct
    // which begs the question, why not just have one then?

    int listener = -1;
    int yes = 1;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE; // use local host
    
    int status = getaddrinfo(NULL, PORT,  &hints, &servinfo);
    

    if (status != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }
    for (p=servinfo; p != NULL; p = p->ai_next){
        int candidate = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (candidate == -1){
            continue;
        }
        if (setsockopt(candidate, SOL_SOCKET, SO_REUSEADDR, &yes,
            sizeof(int)) == -1) {
            perror("setsockopt");
            close(candidate);
            continue;
        }
        
        if (bind(candidate, p->ai_addr, p->ai_addrlen) == -1){
            close(candidate);
            continue;
        }
        listener = candidate;
        break;
    }
    freeaddrinfo(servinfo);
    if (listener == -1){
        fprintf(stderr, "Failed to bind to any available addresses");
        return -1;
    }
    
    if(listen(listener, BACKLOG) == - 1){
        perror("listen");
        close(listener);
        return -1;
    }
    return listener;
}

//-------HTTP RESPONSE---------
int serve_index(int client_fd){
    FILE *file_d = fopen("index.html", "r");
    if (file_d == NULL){
        perror("fopen");
        return -1;
    }

    if (fseek(file_d, 0, SEEK_END) != 0) {
        perror("fseek");
        fclose(file_d);
        return -1;
    }

    long file_size = ftell(file_d);
    if (file_size < 0) {
        perror("ftell");
        fclose(file_d);
        return -1;
    }

    rewind(file_d); // to return the pointer file_d to the start of the file

    char response[256];
    int header_len = snprintf(
        response,
        sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %ld\r\n"
        "\r\n",
        file_size
    );
    if (header_len < 0 ||
        (size_t)header_len >= sizeof response) {
        fprintf(stderr, "Failed to construct HTTP headers\n");
        fclose(file_d);
        return -1;
    }
    if(send_all(client_fd, response, strlen(response)) == -1){
        perror("Send headers");
        fclose(file_d);
        return -1;
    }
    char file_buffer[4096];
    size_t bytes_read;
    int result = 0;
    while ((bytes_read = fread(file_buffer, 1, sizeof(file_buffer), file_d)) > 0){
        if(send_all(client_fd, file_buffer, bytes_read) == -1){
            perror("Send files");
            result = -1;
            break;
        }
    }
    if (ferror(file_d)){
        perror("fread");
        result = -1;
    }
    fclose(file_d);
    return result;
}

// ---------REQUEST HANDLING -------
void handle_client(int client_fd){
    char buff[1000]; // where did I get 1000 from? fix this.
    int message_size = recv(client_fd, buff, sizeof(buff) - 1, 0);

    if (message_size < 0){
        perror("recv");
        return;
    }
    if (message_size == 0){
        perror("recv");
        return;
    }
    buff[message_size] = '\0';
    if (strncmp("GET ", buff, 4) == 0){
        printf("Received a get request\n");
        if(serve_index(client_fd) == 0){
            printf("Responded to GET request\n");
        }
    }
    else{
        printf("Unsupported request method\n");
    }
}





int main(void){
    int listener = create_listener();
    if (listener == -1){
        return EXIT_FAILURE;
    }

    while(1){
        struct sockaddr_storage client_addr;
        socklen_t addr_size = sizeof client_addr;
        printf("Waiting for request...\n");

        int client_fd = accept(listener, (struct sockaddr *)&client_addr, &addr_size);
        if(client_fd == -1){
            if(errno == EINTR){
                continue;
            }
            perror("Accept");
            continue;
        }
        handle_client(client_fd);
        close(client_fd);      
    }
    close(listener);

}