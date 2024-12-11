#include "ftp_client.h"

int createSocket(char *ip, int port)
{
    printf("\n=== SOCKET CREATION ===\n");
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

int closeConnection(int sock)
{
    char cmd[] = "QUIT\r\n";
    char answer[BUFFER_SIZE];

    write(sock, cmd, strlen(cmd));
    getServerResponse(sock, answer);

    return close(sock);
} 