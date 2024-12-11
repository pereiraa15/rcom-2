/**
 * @file socket_ops.c
 * @brief Socket operations for FTP client
 *
 * This file implements the network socket operations required for FTP communication.
 * It handles:
 * - Socket creation and connection
 * - Connection cleanup
 * - Error handling for network operations
 *
 * The implementation uses TCP/IP sockets (SOCK_STREAM) for reliable data transfer
 * and follows standard network programming practices for error checking.
 */

#include "ftp_client.h"

/**
 * @brief Creates and connects a TCP socket to a specified address
 *
 * This function performs the following steps:
 * 1. Validates the IP address format
 * 2. Creates a TCP socket
 * 3. Sets up the server address structure
 * 4. Establishes connection to the server
 *
 * Debug messages are printed at each step to aid in troubleshooting
 * network connectivity issues.
 *
 * @param ip Server IP address in dot notation (e.g., "192.168.1.1")
 * @param port Server port number
 * @return int Socket file descriptor on success, -1 on failure
 */
int createSocket(char *ip, int port)
{
    printf("\n=== SOCKET CREATION ===\n");
    int sockfd;
    struct sockaddr_in server_addr;

    printf("Debug: Creating socket for %s:%d\n", ip, port);

    // Validate IP address format
    if (inet_addr(ip) == INADDR_NONE)
    {
        printf("Error: Invalid IP address format: %s\n", ip);
        return -1;
    }

    // Initialize server address structure
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Create TCP socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        return -1;
    }

    printf("Debug: Socket created successfully\n");
    printf("Debug: Attempting to connect to %s:%d...\n", ip, port);

    // Establish connection
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

/**
 * @brief Properly closes an FTP connection
 *
 * This function performs a clean shutdown of an FTP connection:
 * 1. Sends the QUIT command to the server
 * 2. Waits for server acknowledgment
 * 3. Closes the socket
 *
 * The QUIT command follows RFC 959 specification for proper
 * FTP session termination.
 *
 * @param sock Socket to be closed
 * @return int 0 on success, -1 on failure
 */
int closeConnection(int sock)
{
    char cmd[] = "QUIT\r\n";
    char answer[BUFFER_SIZE];

    // Send QUIT command
    write(sock, cmd, strlen(cmd));
    // Wait for server acknowledgment
    getServerResponse(sock, answer);

    return close(sock);
} 