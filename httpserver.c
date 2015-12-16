#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORTNO 3333
#define BUFSIZE 4096

void handle_sockfd(int fd) {
    int ret;
    char buf[BUFSIZE];

    ret = read(fd, buf, BUFSIZE - 1);

    if (ret == 0 || ret == -1) {
        exit(1);
    }

    if (ret > 0 && ret < BUFSIZE) {
        buf[ret] = '\0';    
    } else {
        buf[0] = '\0';
    }

    /* printf("ret = %s\n", buf); */
    /* fflush(stdout); */

    for (int i = 0; i < ret; i++) {
        if (buf[i] == '\r' || buf[i] == '\n') {
            buf[i] = '\0';
        }    
    }

    memset(buf, 0, BUFSIZE);
    sprintf(buf, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", "text/html");
    write(fd, buf, strlen(buf) + 1);

    int file_fd = open("form_get.html", O_RDONLY);
    
    while ((ret = read(file_fd, buf, BUFSIZE)) > 0) {
        write(fd, buf, ret);
    }

    exit(0);
}

int main(int argc, const char *argv[])
{
    int sockfd, newsockfd, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int pid;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("ERROR on opening socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero((char*)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORTNO);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        printf("newsockfd = %d\n", newsockfd);
        if (newsockfd < 0) {
            perror("ERROR on accepting");
            exit(1);
        }

        pid = fork();

        if (pid < 0) {
            perror("ERROR on fork");
            exit(1);
        }

        if (pid == 0) {
            close(sockfd);
            dup2(newsockfd, 0);
            dup2(newsockfd, 1);
            dup2(newsockfd, 2);
            handle_sockfd(newsockfd);
        } else {
            close(newsockfd);
        }
    }
    return 0;
}
