#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <termios.h>
#include <errno.h>
#include <pwd.h>
#include <time.h>

#define MAX_LENGTH 500
#define FTP_PORT 21
#define BUFFER_SIZE 1024
#define DEFAULT_PORT 21
#define PASV_PORT_PATTERN "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)"

/* Server responses */
#define SV_READY4AUTH 220
#define SV_READY4PASS 331
#define SV_LOGINSUCCESS 230
#define SV_PASSIVE 227
#define SV_READY4TRANSFER 150
#define SV_TRANSFER_COMPLETE 226
#define SV_GOODBYE 221

/* Parser regular expressions */
#define AT "@"
#define BAR "/"
#define HOST_REGEX "%*[^/]//%[^/]"
#define HOST_AT_REGEX "%*[^/]//%*[^@]@%[^/]"
#define RESOURCE_REGEX "%*[^/]//%*[^/]/%s"
#define USER_REGEX "%*[^/]//%[^:/]"
#define PASS_REGEX "%*[^/]//%*[^:]:%[^@\n$]"
#define RESPCODE_REGEX "%d"
#define PASSIVE_REGEX "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]"

/* Default login */
#define DEFAULT_USER "rcom"
#define DEFAULT_PASSWORD "rcom"

/* URL structure */
struct URL {
    char host[MAX_LENGTH];     // 'ftp.up.pt'
    char resource[MAX_LENGTH]; // 'parrot/misc/canary/warrant-canary-0.txt'
    char file[MAX_LENGTH];     // 'warrant-canary-0.txt'
    char user[MAX_LENGTH];     // 'username'
    char password[MAX_LENGTH]; // 'password'
    char ip[MAX_LENGTH];       // 193.137.29.15
};

/* Function prototypes */
// URL Parser
int parse(char *input, struct URL *url);

// Socket operations
int createSocket(char *ip, int port);
int closeConnection(int sock);

// FTP Protocol
int authenticate(int sock, const char *user, const char *pass);
int enterPassiveMode(int sock, char *addr, int *port);
int getServerResponse(int sock, char *buffer);
int downloadFile(int ctrlSock, int dataSock, char *filename);
int requestFile(int sock, char *path);

#endif /* FTP_CLIENT_H */ 