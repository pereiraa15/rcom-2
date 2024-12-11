#include "ftp_client.h"

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

    printf("\n=== CONNECTION DETAILS ===\n");
    printf("Host: %s\n", url.host);
    printf("Resource: %s\n", url.resource);
    printf("File: %s\n", url.file);
    printf("User: %s\n", url.user);
    printf("Password: %s\n", url.password);
    printf("IP Address: %s\n", url.ip);
    printf("Port: %d\n", FTP_PORT);
    printf("=======================\n");

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