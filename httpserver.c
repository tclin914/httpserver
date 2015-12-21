#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define STDIN  0
#define STDOUT 1
#define STDERR 2

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
    EXE,
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

    /* fprintf(stdout, buf); */
    /* fflush(stdout); */

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

    /* separate filename and get infomation */
    char* filename;
    filename = strtok(&buf[5], "?");
    char* get;
    get = strtok(NULL, "?");

    /* avoid .. command */
    for (int j = 1; j < strlen(filename); j++) {
        if (filename[j - 1] == '.' && filename[j] == '.') {
            exit(1);
        }
    }

    FileType filetype; 
    int fnlen = strlen(filename);
    int len;
    /* char* filetype = (char*)0; */
    for (int i = 0; extensions[i].ext != 0; i++) {
        len = strlen(extensions[i].ext);
        if (strncmp(&filename[fnlen - len], extensions[i].ext, len) == 0) {
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
    char *arguments[2];
    int pid, status;
    switch (filetype) {
        case GIF:
        case JPG:
        case PNG:
        case ZIP:
        case GZ:
        case TAR:
        case HTM:
        case HTML:
            file_fd = open(filename, O_RDONLY);    
            while ((ret = read(file_fd, msg_buf, BUFSIZE)) > 0) {
                write(fd, msg_buf, ret);
            }
            
            break;
        case EXE:
            break;
        case CGI:

            pid = fork();

            if (pid < 0) {
                perror("ERROR on fork");
                exit(1);
            }
            if (pid == 0) {
                dup2(fd, STDOUT);
                dup2(fd, STDERR);
                
                /* set environment variable */
                setenv("QUERY_STRING", get, 1);
                setenv("CONTENT_LENGTH", "1024", 1);
                setenv("REQUEST_METHOD", "GET", 1);
                setenv("SCRIPT_NAME", filename, 1);
                setenv("REMOTE_HOST", "nplinux1.cs.nctu.edu.tw", 1);
                setenv("REMOTE_ADDR", "140.113.168.191", 1);
                setenv("AUTH_TYPE", "NONE", 1);
                setenv("REMOTE_USER", "tclin", 1);
                setenv("REMOTE_IDENT", "NONE", 1);

                execl(filename, filename, NULL);
                fprintf(stderr, "Unknown command.\n");
                fflush(stdout);
                exit(1);
            } else {
                waitpid((pid_t)pid, &status, 0);

            }
            break;
        default:
            break;
    }

    /* fprintf(stdout, "buf = %s\n", buf); */
    /* fprintf(stdout, "filetype = %s\n", extensions[filetype].filetype); */
    /* fprintf(stdout, "filename = %s\n", filename); */
    /* fprintf(stdout, "get = %s\n", get); */
    /* fflush(stdout); */
    exit(0);
}

int main(int argc, const char *argv[])
{
    int sockfd, newsockfd, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int pid;

    /* chdir("/u/gcs/103/0356100/public_html"); */

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
