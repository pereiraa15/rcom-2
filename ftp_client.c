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

/* Function prototypes */
int parse(char *input, struct URL *url);
int createSocket(char *ip, int port);
int authenticate(int sock, const char *user, const char *pass);
int enterPassiveMode(int sock, char *addr, int *port);
int getServerResponse(int sock, char *buffer);
int downloadFile(int ctrlSock, int dataSock, char *filename);
int requestFile(int sock, char *path);
int closeConnection(int sock);

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
        printf("Debug: Connection failed to %s:%d\n", ip, port);
        printf("Error code: %d\n", errno);
        close(sockfd);
        return -1;
    }

    printf("Debug: Connection established\n");
    return sockfd;
}

int authenticate(int sock, const char *user, const char *pass)
{
    char cmd[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int responseCode;

    // Keep reading responses until we get the final 220
    do
    {
        responseCode = getServerResponse(sock, response);
        if (responseCode < 0)
            return -1;
    } while (response[3] == '-'); // Continue if it's a multi-line response

    if (responseCode != SV_READY4AUTH)
    {
        printf("Server not ready. Response code: %d\n", responseCode);
        return -1;
    }

    // Now send username
    printf("Sending USER command...\n");
    sprintf(cmd, "USER %s\r\n", user);
    write(sock, cmd, strlen(cmd));
    responseCode = getServerResponse(sock, response);
    if (responseCode != SV_READY4PASS)
    {
        printf("Username not accepted (code %d)\n", responseCode);
        return -1;
    }

    // Send password
    printf("Sending PASS command...\n");
    sprintf(cmd, "PASS %s\r\n", pass);
    write(sock, cmd, strlen(cmd));
    responseCode = getServerResponse(sock, response);
    if (responseCode != SV_LOGINSUCCESS)
    {
        printf("Password not accepted (code %d)\n", responseCode);
        return -1;
    }

    printf("Authentication successful!\n");
    return 0;
}

int enterPassiveMode(int sock, char *addr, int *port)
{
    char cmd[] = "PASV\r\n";
    char response[BUFFER_SIZE];
    int ip[4], p[2];

    write(sock, cmd, strlen(cmd));
    int responseCode = getServerResponse(sock, response);
    if (responseCode != SV_PASSIVE)
    {
        printf("Error entering passive mode. Server response: %s\n", response);
        return -1;
    }

    if (sscanf(response, PASV_PORT_PATTERN, &ip[0], &ip[1], &ip[2], &ip[3], &p[0], &p[1]) != 6)
    {
        printf("Error parsing passive mode response: %s\n", response);
        return -1;
    }

    sprintf(addr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    *port = p[0] * 256 + p[1];

    printf("Passive mode: connecting to %s:%d\n", addr, *port);

    return 0;
}

int getServerResponse(int sock, char *buffer)
{
    char byte;
    char code[4];
    int index = 0;
    ssize_t bytes_read;
    char temp_buffer[BUFFER_SIZE];

    memset(buffer, 0, BUFFER_SIZE);
    memset(temp_buffer, 0, BUFFER_SIZE);

    // Read the first line to get the response code
    while ((bytes_read = read(sock, &byte, 1)) > 0)
    {
        buffer[index++] = byte;
        if (index >= 2 && buffer[index - 1] == '\n' && buffer[index - 2] == '\r')
        {
            break;
        }
        if (index >= BUFFER_SIZE - 1)
            return -1;
    }

    if (bytes_read <= 0 || index < 3)
    {
        printf("Error reading from server\n");
        return -1;
    }

    // Extract response code
    memcpy(code, buffer, 3);
    code[3] = '\0';
    int response_code = atoi(code);

    printf("Server Response: %s", buffer);
    return response_code;
}

int downloadFile(int ctrlSock, int dataSock, char *filename)
{
    FILE *file;
    char buffer[BUFFER_SIZE];
    int bytes;
    char filepath[MAX_LENGTH];
    
    // Use relative path to Downloads folder in project directory
    snprintf(filepath, sizeof(filepath), "downloads/%s", filename);
    
    if (!(file = fopen(filepath, "wb")))
    {
        printf("Cannot create file in Downloads folder: %s (Error: %s)\n", 
               filepath, strerror(errno));
        return -1;
    }

    printf("Downloading to: %s\n", filepath);
    
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
    char response[BUFFER_SIZE];

    printf("Requesting file: %s\n", path);
    sprintf(cmd, "RETR %s\r\n", path);
    write(sock, cmd, strlen(cmd));

    int responseCode = getServerResponse(sock, response);
    if (responseCode != SV_READY4TRANSFER)
    {
        printf("Error requesting file. Server response: %s\n", response);
        return -1;
    }

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
    if (ctrlSock < 0)
    {
        printf("Failed to create control socket\n");
        return 1;
    }

    if (authenticate(ctrlSock, url.user, url.password) != 0)
    {
        printf("Authentication failed\n");
        closeConnection(ctrlSock);
        return 1;
    }

    char dataAddr[BUFFER_SIZE];
    int dataPort;
    if (enterPassiveMode(ctrlSock, dataAddr, &dataPort) != 0)
    {
        printf("Failed to enter passive mode\n");
        closeConnection(ctrlSock);
        return 1;
    }

    int dataSock = createSocket(dataAddr, dataPort);
    if (dataSock < 0)
    {
        printf("Failed to create data connection\n");
        closeConnection(ctrlSock);
        return 1;
    }

    if (requestFile(ctrlSock, url.resource) != 0)
    {
        printf("Failed to request file\n");
        close(dataSock);
        closeConnection(ctrlSock);
        return 1;
    }

    if (downloadFile(ctrlSock, dataSock, url.file) != 0)
    {
        printf("Failed to download file\n");
        close(dataSock);
        closeConnection(ctrlSock);
        return 1;
    }

    close(dataSock);
    closeConnection(ctrlSock);

    return 0;
}