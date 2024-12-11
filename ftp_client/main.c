/**
 * @file main.c
 * @brief Main program for FTP client
 *
 * This program implements a command-line FTP client that can download files from
 * FTP servers. It supports both anonymous and authenticated connections.
 *
 * Usage: ./download ftp://[<user>:<password>@]<host>/<url-path>
 *
 * Example URLs:
 * - Anonymous: ftp://ftp.up.pt/pub/file.txt
 * - Authenticated: ftp://user:pass@ftp.example.com/path/to/file.txt
 *
 * Program Flow:
 * 1. Parse command line arguments and URL
 * 2. Establish control connection
 * 3. Authenticate with server
 * 4. Enter passive mode for data transfer
 * 5. Request and download file
 * 6. Clean up connections
 */

#include "ftp_client.h"

/**
 * @brief Main entry point for the FTP client
 *
 * Orchestrates the FTP download process:
 * 1. Validates command line arguments
 * 2. Parses the FTP URL
 * 3. Establishes connections
 * 4. Handles the file transfer
 * 5. Performs cleanup
 *
 * Error handling is implemented at each step, with appropriate
 * cleanup on failure to prevent resource leaks.
 *
 * @param argc Number of command line arguments
 * @param argv Array of command line arguments
 * @return int 0 on successful download, 1 on error
 */
int main(int argc, char *argv[])
{
    // Validate command line arguments
    if (argc != 2)
    {
        printf("Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        return 1;
    }

    // Initialize URL structure and parse the URL
    struct URL url;
    memset(&url, 0, sizeof(url));
    if (parse(argv[1], &url) != 0)
    {
        printf("Parse error. Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        return 1;
    }

    // Display connection information
    printf("\n=== CONNECTION DETAILS ===\n");
    printf("Host: %s\n", url.host);
    printf("Resource: %s\n", url.resource);
    printf("File: %s\n", url.file);
    printf("User: %s\n", url.user);
    printf("Password: %s\n", url.password);
    printf("IP Address: %s\n", url.ip);
    printf("Port: %d\n", FTP_PORT);
    printf("=======================\n");

    // Establish control connection
    int ctrlSock = createSocket(url.ip, FTP_PORT);
    if (ctrlSock < 0)
    {
        printf("Failed to create control socket\n");
        return 1;
    }

    // Authenticate with the server
    if (authenticate(ctrlSock, url.user, url.password) != 0)
    {
        printf("Authentication failed\n");
        closeConnection(ctrlSock);
        return 1;
    }

    // Enter passive mode for data transfer
    char dataAddr[BUFFER_SIZE];
    int dataPort;
    if (enterPassiveMode(ctrlSock, dataAddr, &dataPort) != 0)
    {
        printf("Failed to enter passive mode\n");
        closeConnection(ctrlSock);
        return 1;
    }

    // Establish data connection
    int dataSock = createSocket(dataAddr, dataPort);
    if (dataSock < 0)
    {
        printf("Failed to create data connection\n");
        closeConnection(ctrlSock);
        return 1;
    }

    // Request the file from the server
    if (requestFile(ctrlSock, url.resource) != 0)
    {
        printf("Failed to request file\n");
        close(dataSock);
        closeConnection(ctrlSock);
        return 1;
    }

    // Download the file
    if (downloadFile(ctrlSock, dataSock, url.file) != 0)
    {
        printf("Failed to download file\n");
        close(dataSock);
        closeConnection(ctrlSock);
        return 1;
    }

    // Clean up connections
    close(dataSock);
    closeConnection(ctrlSock);

    return 0;
} 