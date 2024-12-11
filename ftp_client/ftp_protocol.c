/**
 * @file ftp_protocol.c
 * @brief Implementation of FTP protocol operations
 *
 * This file implements the core FTP protocol operations including:
 * - Server response handling
 * - Authentication
 * - Passive mode negotiation
 * - File transfer operations
 *
 * The implementation follows RFC 959 (File Transfer Protocol)
 * and supports the basic commands required for file download.
 */

#include "ftp_client.h"

/**
 * @brief Reads and processes a server response
 *
 * This function reads the server's response line by line and processes
 * the FTP response code. FTP responses consist of a 3-digit code
 * followed by a message. Multi-line responses are supported, where
 * intermediate lines are indicated by a hyphen after the code (e.g., "230-").
 *
 * Example responses:
 * - "230 User logged in"
 * - "230- Welcome message\r\n230 Login successful"
 *
 * @param sock Socket to read from
 * @param buffer Buffer to store the response
 * @return int Response code (200-599) on success, 0 for continuation, -1 on error
 */
int getServerResponse(int sock, char *buffer)
{
    char byte;
    char code[4];
    int index = 0;
    ssize_t bytes_read;
    char temp_buffer[BUFFER_SIZE];

    memset(buffer, 0, BUFFER_SIZE);
    memset(temp_buffer, 0, BUFFER_SIZE);

    // Read response line character by character
    while ((bytes_read = read(sock, &byte, 1)) > 0)
    {
        buffer[index++] = byte;
        // Check for end of line (\r\n)
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

    // Check if line starts with a response code (digit)
    if (buffer[0] >= '0' && buffer[0] <= '9')
    {
        memcpy(code, buffer, 3);
        code[3] = '\0';
        return atoi(code);
    }
    else
    {
        // Return 0 for continuation lines in multi-line responses
        return 0;
    }
}

/**
 * @brief Authenticates with the FTP server
 *
 * Performs the FTP authentication sequence:
 * 1. Waits for server welcome message (220)
 * 2. Sends USER command and waits for response (331)
 * 3. Sends PASS command and waits for response (230)
 *
 * The function handles multi-line responses at each step.
 * Authentication fails if any step returns an unexpected response code.
 *
 * @param sock Control socket
 * @param user Username for authentication
 * @param pass Password for authentication
 * @return int 0 on successful authentication, -1 on failure
 */
int authenticate(int sock, const char *user, const char *pass)
{
    printf("\n=== SERVER WELCOME ===\n");
    char cmd[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int responseCode;

    // Wait for server welcome message
    do {
        responseCode = getServerResponse(sock, response);
        if (responseCode < 0) return -1;
    } while (responseCode == 0 || response[3] == '-');

    printf("\n=== AUTHENTICATION ===\n");
    // Send username
    printf("Sending USER command...\n");
    sprintf(cmd, "USER %s\r\n", user);
    write(sock, cmd, strlen(cmd));
    
    do {
        responseCode = getServerResponse(sock, response);
        if (responseCode < 0) return -1;
    } while (responseCode == 0 || response[3] == '-');

    // Send password
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

/**
 * @brief Enters passive mode for data transfer
 *
 * Sends the PASV command and parses the server's response to get
 * the data connection address and port. The response format is:
 * "227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)"
 * where h1-h4 form the IP address and p1,p2 form the port number.
 *
 * Port calculation: port = p1*256 + p2
 *
 * @param sock Control socket
 * @param addr Buffer to store the data connection IP address
 * @param port Pointer to store the data connection port
 * @return int 0 on success, -1 on failure
 */
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

    // Parse the passive mode response
    if (sscanf(response, PASV_PORT_PATTERN, &ip[0], &ip[1], &ip[2], &ip[3], &p[0], &p[1]) != 6)
    {
        printf("Error parsing passive mode response: %s\n", response);
        return -1;
    }

    // Construct IP address and calculate port
    sprintf(addr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    *port = p[0] * 256 + p[1];

    printf("Passive mode: connecting to %s:%d\n", addr, *port);
    return 0;
}

/**
 * @brief Downloads a file from the FTP server
 *
 * Handles the data transfer process after the connection is established:
 * 1. Creates a local file in the downloads directory
 * 2. Reads data from the data socket in chunks
 * 3. Writes the data to the local file
 * 4. Displays progress and transfer speed
 *
 * The function automatically creates a 'downloads' directory
 * and saves the file there with the original filename.
 *
 * @param ctrlSock Control socket (for status messages)
 * @param dataSock Data socket (for file transfer)
 * @param filename Name to save the file as
 * @return int 0 on successful download, -1 on failure
 */
int downloadFile(int ctrlSock, int dataSock, char *filename)
{
    printf("\n=== FILE DOWNLOAD ===\n");
    FILE *file;
    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    char filepath[MAX_LENGTH];
    size_t total_bytes = 0;
    time_t start_time = time(NULL);
    
    // Prepare file path in downloads directory
    snprintf(filepath, sizeof(filepath), "downloads/%s", filename);
    
    if (!(file = fopen(filepath, "wb")))
    {
        printf("Cannot create file in Downloads folder: %s (Error: %s)\n", 
               filepath, strerror(errno));
        return -1;
    }

    printf("Downloading to: %s\n", filepath);
    
    // Read data in chunks and write to file
    while ((bytes = read(dataSock, buffer, BUFFER_SIZE)) > 0)
    {
        fwrite(buffer, bytes, 1, file);
        total_bytes += bytes;
        
        // Calculate and display transfer speed
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

/**
 * @brief Requests a file from the FTP server
 *
 * Sends the RETR command to initiate a file transfer.
 * Accepts both 150 and 125 as valid response codes:
 * - 150: File status okay; about to open data connection
 * - 125: Data connection already open; transfer starting
 *
 * @param sock Control socket
 * @param path Path of the file to retrieve
 * @return int 0 if server accepts request, -1 on failure
 */
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