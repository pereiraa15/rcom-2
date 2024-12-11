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

#define MAX_LENGTH 500
#define FTP_PORT 21
#define BUFFER_SIZE 1024
#define DEFAULT_PORT 21
#define PASV_PORT_PATTERN "%*[^0-9]%d,%d,%d,%d,%d,%d"

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

/* Default login for case 'ftp://<host>/<url-path>' */
#define DEFAULT_USER "rcom"
#define DEFAULT_PASSWORD "rcom"

/* Parser output */
struct URL
{
    char host[MAX_LENGTH];     // 'ftp.up.pt'
    char resource[MAX_LENGTH]; // 'parrot/misc/canary/warrant-canary-0.txt'
    char file[MAX_LENGTH];     // 'warrant-canary-0.txt'
    char user[MAX_LENGTH];     // 'username'
    char password[MAX_LENGTH]; // 'password'
    char ip[MAX_LENGTH];       // 193.137.29.15
};

typedef enum
{
    START,
    SINGLE,
    MULTIPLE,
    END
} ResponseState;

int parse(char *input, struct URL *url)
{

    regex_t regex;
    regcomp(&regex, BAR, 0);
    if (regexec(&regex, input, 0, NULL, 0))
        return -1;

    regcomp(&regex, AT, 0);
    if (regexec(&regex, input, 0, NULL, 0) != 0)
    { // ftp://<host>/<url-path>

        sscanf(input, HOST_REGEX, url->host);
        strcpy(url->user, DEFAULT_USER);
        strcpy(url->password, DEFAULT_PASSWORD);
    }
    else
    { // ftp://[<user>:<password>@]<host>/<url-path>

        sscanf(input, HOST_AT_REGEX, url->host);
        sscanf(input, USER_REGEX, url->user);
        sscanf(input, PASS_REGEX, url->password);
    }

    sscanf(input, RESOURCE_REGEX, url->resource);
    strcpy(url->file, strrchr(input, '/') + 1);

    struct hostent *h;
    if (strlen(url->host) == 0)
        return -1;
    if ((h = gethostbyname(url->host)) == NULL)
    {
        printf("Invalid hostname '%s'\n", url->host);
        exit(-1);
    }
    strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));

    return !(strlen(url->host) && strlen(url->user) &&
             strlen(url->password) && strlen(url->resource) && strlen(url->file));
}

int createSocket(char *ip, int port)
{
    int sockfd;
    struct sockaddr_in server_addr;

    printf("Debug: Creating socket for %s:%d\n", ip, port);

    if (inet_addr(ip) == INADDR_NONE)
    {
        printf("Error: Invalid IP address format: %s\n", ip);
        return -1;
    }

    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        return -1;
    }

    printf("Debug: Socket created successfully\n");
    printf("Debug: Attempting to connect to %s:%d...\n", ip, port);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect()");
        printf("Debug: Connection failed. Please verify:\n");
        printf("1. You are connected to FEUP's VPN or network\n");
        printf("2. The host is reachable (try 'ping %s')\n", ip);
        printf("3. No firewall is blocking the connection\n");
        return -1;
    }

    printf("Debug: Connection established\n");
    return sockfd;
}

int authenticate(int sock, const char *user, const char *pass)
{
    char cmd[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    sprintf(cmd, "USER %s\r\n", user);
    write(sock, cmd, strlen(cmd));
    read(sock, response, BUFFER_SIZE);

    sprintf(cmd, "PASS %s\r\n", pass);
    write(sock, cmd, strlen(cmd));
    read(sock, response, BUFFER_SIZE);

    return 0;
}

int enterPassiveMode(int sock, char *addr, int *port)
{
    char cmd[] = "PASV\r\n";
    char response[BUFFER_SIZE];
    int ip[4], p[2];

    write(sock, cmd, strlen(cmd));
    read(sock, response, BUFFER_SIZE);

    sscanf(response, PASV_PORT_PATTERN, &ip[0], &ip[1], &ip[2], &ip[3], &p[0], &p[1]);
    sprintf(addr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    *port = p[0] * 256 + p[1];

    return 0;
}

int getServerResponse(int sock, char *buffer)
{
    char byte;
    char code[4];
    int state = 0;
    int index = 0;

    memset(buffer, 0, BUFFER_SIZE);

    while (state != 3)
    {

        read(sock, &byte, 1);
        buffer[index] = byte;
        index++;

        switch (state)
        {
        case 0:
            if (index == 3)
            {
                memcpy(code, buffer, 3);
                code[3] = '\0';
                state = 1;
            }
            break;
        case 1:
            if (byte == '\n')
            {
                if (buffer[index - 2] == '\r')
                {
                    if (buffer[0] == code[0] &&
                        buffer[1] == code[1] &&
                        buffer[2] == code[2] &&
                        buffer[3] == ' ')
                    {
                        state = 3;
                    }
                    else
                        state = 2;
                }
            }
            break;
        case 2:
            if (byte == '\n')
            {
                if (buffer[index - 2] == '\r')
                {
                    if (index >= 4 &&
                        buffer[index - 5] == code[0] &&
                        buffer[index - 4] == code[1] &&
                        buffer[index - 3] == code[2] &&
                        buffer[index - 2] == ' ')
                    {
                        state = 3;
                    }
                }
            }
            break;
        }
    }

    return atoi(code);
}

int downloadFile(int ctrlSock, int dataSock, char *filename)
{
    FILE *file;
    char buffer[BUFFER_SIZE];
    int bytes;

    if (!(file = fopen(filename, "wb")))
    {
        printf("Cannot create file\n");
        return -1;
    }

    while ((bytes = read(dataSock, buffer, BUFFER_SIZE)) > 0)
    {
        fwrite(buffer, bytes, 1, file);
    }

    fclose(file);
    return 0;
}

int requestFile(int sock, char *path)
{
    char cmd[BUFFER_SIZE];
    sprintf(cmd, "RETR %s\r\n", path);
    write(sock, cmd, strlen(cmd));
    return 0;
}

int closeConnection(int sock)
{
    char cmd[] = "QUIT\r\n";
    char answer[BUFFER_SIZE];

    write(sock, cmd, strlen(cmd));
    getServerResponse(sock, answer);

    return close(sock);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        return 1;
    }

    struct URL url;
    memset(&url, 0, sizeof(url));
    if (parse(argv[1], &url) != 0)
    {
        printf("Parse error. Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        return 1;
    }

    printf("\n=== Connection Details ===\n");
    printf("Host: %s\n", url.host);
    printf("Resource: %s\n", url.resource);
    printf("File: %s\n", url.file);
    printf("User: %s\n", url.user);
    printf("Password: %s\n", url.password);
    printf("IP Address: %s\n", url.ip);
    printf("Port: %d\n", FTP_PORT);
    printf("=======================\n\n");

    int ctrlSock = createSocket(url.ip, FTP_PORT);
    if (ctrlSock < 0 || getServerResponse(ctrlSock, NULL) != SV_READY4AUTH)
    {
        printf("Socket to '%s' and port %d failed\n", url.ip, FTP_PORT);
        return 1;
    }

    if (authenticate(ctrlSock, url.user, url.password) != 0)
    {
        printf("Authentication failed with username = '%s' and password = '%s'.\n", url.user, url.password);
        return 1;
    }

    char dataAddr[BUFFER_SIZE];
    int dataPort;
    if (enterPassiveMode(ctrlSock, dataAddr, &dataPort) != 0)
    {
        printf("Passive mode failed\n");
        return 1;
    }

    int dataSock = createSocket(dataAddr, dataPort);
    if (dataSock < 0)
    {
        printf("Socket to '%s:%d' failed\n", dataAddr, dataPort);
        return 1;
    }

    if (requestFile(ctrlSock, url.resource) != 0)
    {
        printf("Unknown resouce '%s' in '%s:%d'\n", url.resource, dataAddr, dataPort);
        return 1;
    }

    if (downloadFile(ctrlSock, dataSock, url.file) != 0)
    {
        printf("Error transfering file '%s' from '%s:%d'\n", url.file, dataAddr, dataPort);
        return 1;
    }

    if (closeConnection(ctrlSock) != 0)
    {
        printf("Sockets close error\n");
        return 1;
    }

    close(dataSock);

    return 0;
}