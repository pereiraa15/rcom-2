#include "ftp_client.h"

int getServerResponse(int sock, char *buffer)
{
    char byte;
    char code[4];
    int index = 0;
    ssize_t bytes_read;
    char temp_buffer[BUFFER_SIZE];

    memset(buffer, 0, BUFFER_SIZE);
    memset(temp_buffer, 0, BUFFER_SIZE);

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

    printf("Server Response: %s", buffer);

    if (buffer[0] >= '0' && buffer[0] <= '9')
    {
        memcpy(code, buffer, 3);
        code[3] = '\0';
        return atoi(code);
    }
    else
    {
        return 0;
    }
}

int authenticate(int sock, const char *user, const char *pass)
{
    printf("\n=== SERVER WELCOME ===\n");
    char cmd[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int responseCode;

    do {
        responseCode = getServerResponse(sock, response);
        if (responseCode < 0) return -1;
    } while (responseCode == 0 || response[3] == '-');

    printf("\n=== AUTHENTICATION ===\n");
    printf("Sending USER command...\n");
    sprintf(cmd, "USER %s\r\n", user);
    write(sock, cmd, strlen(cmd));
    
    do {
        responseCode = getServerResponse(sock, response);
        if (responseCode < 0) return -1;
    } while (responseCode == 0 || response[3] == '-');

    printf("Sending PASS command...\n");
    sprintf(cmd, "PASS %s\r\n", pass);
    write(sock, cmd, strlen(cmd));
    
    do {
        responseCode = getServerResponse(sock, response);
        if (responseCode < 0) return -1;
    } while (responseCode == 0 || response[3] == '-');

    printf("Authentication successful!\n");
    return 0;
}

int enterPassiveMode(int sock, char *addr, int *port)
{
    printf("\n=== PASSIVE MODE ===\n");
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

int downloadFile(int ctrlSock, int dataSock, char *filename)
{
    printf("\n=== FILE DOWNLOAD ===\n");
    FILE *file;
    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    char filepath[MAX_LENGTH];
    size_t total_bytes = 0;
    time_t start_time = time(NULL);
    
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
        total_bytes += bytes;
        
        time_t current_time = time(NULL);
        double elapsed = difftime(current_time, start_time);
        if (elapsed > 0) {
            double speed = total_bytes / (1024.0 * 1024.0) / elapsed;
            printf("\rDownloaded: %.2f MB (%.2f MB/s)", 
                   total_bytes / (1024.0 * 1024.0), speed);
            fflush(stdout);
        }
    }
    printf("\nDownload completed. Total: %.2f MB\n", total_bytes / (1024.0 * 1024.0));

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
    if (responseCode != SV_READY4TRANSFER && responseCode != 125)
    {
        printf("Error requesting file. Server response: %s\n", response);
        return -1;
    }

    return 0;
} 