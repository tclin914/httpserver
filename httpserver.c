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

#define PORTNO 3344
#define BUFSIZE 4096

struct {
    char* ext;
    char* filetype;
} extensions[] = {
    {"gif", "image/gif"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"zip", "image/zip"},
    {"gz", "image/gz"},
    {"tar", "image/tar"},
    {"htm", "text/html"},
    {"html", "text/html"},
    {"exe", "text/plain"},
    {"cgi", "text/html"},
    {0, 0}
};

typedef enum {
    GIF,
    JPG,
    JPEG,
    PNG,
    ZIP,
    GZ,
    TAR,
    HTM,
    HTML,
    EXT,
    CGI,
    NONE
} FileType;

void handle_sockfd(int fd) {
    int ret;
    char buf[BUFSIZE];

    ret = read(fd, buf, BUFSIZE - 1);

    if (ret == 0 || ret == -1) {
        exit(1);
    }

    fprintf(stdout, buf);
    fflush(stdout);

    /* insert '\0' in the end of string */
    if (ret > 0 && ret < BUFSIZE) {
        buf[ret] = '\0';    
    } else {
        buf[0] = '\0';
    }

    /* replace '\r' and '\n' with '\0' */
    for (int i = 0; i < ret; i++) {
        if (buf[i] == '\r' || buf[i] == '\n') {
            buf[i] = '\0';
        }    
    }

    /* only accept GET request */
    if (strncmp(buf, "GET ", 4) != 0 && strncmp(buf, "get ", 4) != 0) {
        exit(1);   
    } 

    /* remove the last of GET /xxx.html?xxx */
    int i;
    for (i = 4; i < ret; i++) {
        if (buf[i] == ' ') {
            buf[i] = '\0';
            break;
        }
    }

    char* filename;
    filename = strtok(&buf[5], buf);

    /* avoid .. command */
    for (int j = 0; j < i - 1; j++) {
        if (buf[j] == '.' && buf[j + 1] == '.') {
            exit(1);
        }
    }

    FileType filetype; 
    int buflen = strlen(buf);
    int len;
    /* char* filetype = (char*)0; */
    for (int i = 0; extensions[i].ext != 0; i++) {
        len = strlen(extensions[i].ext);
        if (strncmp(&buf[buflen - len], extensions[i].ext, len) == 0) {
            /* filetype = extensions[i].filetype; */
            filetype = i;        
            break;
        }
    }

    char msg_buf[BUFSIZE];
    memset(msg_buf, 0, BUFSIZE);
    sprintf(msg_buf, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", extensions[filetype].filetype);
    write(fd, msg_buf, strlen(msg_buf) + 1);

    memset(msg_buf, 0, BUFSIZE);

    int file_fd;
    switch (filetype) {
        case HTML:
            file_fd = open(&buf[5], O_RDONLY);    
            while ((ret = read(file_fd, msg_buf, BUFSIZE)) > 0) {
                write(fd, msg_buf, ret);
            }
            
            break;
        default:
            break;
    }

    fprintf(stdout, "buf = %s\n", buf);
    fprintf(stdout, "filetype = %s\n", extensions[filetype].filetype);
    fflush(stdout);
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
