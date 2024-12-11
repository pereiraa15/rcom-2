/**
 * @file ftp_client.h
 * @brief Header file for a simple FTP client implementation
 * 
 * This file contains all the necessary declarations for implementing
 * a basic FTP client that can connect to an FTP server, authenticate,
 * and download files. It supports both anonymous and authenticated
 * connections using the following URL formats:
 * - ftp://<host>/<url-path>
 * - ftp://[<user>:<password>@]<host>/<url-path>
 */

#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H

/* Required system headers */
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

/* Constants for buffer sizes and default values */
#define MAX_LENGTH 500        /**< Maximum length for string buffers */
#define FTP_PORT 21          /**< Default FTP control port */
#define BUFFER_SIZE 1024     /**< Size for general purpose buffers */
#define DEFAULT_PORT 21      /**< Default port for FTP connections */

/** Pattern for parsing passive mode response */
#define PASV_PORT_PATTERN "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)"

/* FTP Server Response Codes */
#define SV_READY4AUTH 220      /**< Server ready for authentication */
#define SV_READY4PASS 331      /**< Username OK, need password */
#define SV_LOGINSUCCESS 230    /**< User logged in successfully */
#define SV_PASSIVE 227         /**< Entering passive mode */
#define SV_READY4TRANSFER 150  /**< File status okay; opening data connection */
#define SV_TRANSFER_COMPLETE 226/**< Transfer completed successfully */
#define SV_GOODBYE 221         /**< Server saying goodbye */

/* URL Parsing Regular Expressions */
#define AT "@"                 /**< Separator for user:pass@host */
#define BAR "/"                /**< Path separator */
/** Regex for extracting host from URL without credentials */
#define HOST_REGEX "%*[^/]//%[^/]"
/** Regex for extracting host from URL with credentials */
#define HOST_AT_REGEX "%*[^/]//%*[^@]@%[^/]"
/** Regex for extracting resource path */
#define RESOURCE_REGEX "%*[^/]//%*[^/]/%s"
/** Regex for extracting username */
#define USER_REGEX "%*[^/]//%[^:/]"
/** Regex for extracting password */
#define PASS_REGEX "%*[^/]//%*[^:]:%[^@\n$]"
/** Regex for extracting response code */
#define RESPCODE_REGEX "%d"
/** Regex for parsing passive mode response */
#define PASSIVE_REGEX "%*[^(](%d,%d,%d,%d,%d,%d)%*[^\n$)]"

/* Default credentials for anonymous login */
#define DEFAULT_USER "rcom"        /**< Default username */
#define DEFAULT_PASSWORD "rcom"    /**< Default password */

/**
 * @struct URL
 * @brief Structure to store parsed URL components
 * 
 * This structure holds all the components extracted from an FTP URL,
 * including server information, credentials, and requested resource details.
 */
struct URL {
    char host[MAX_LENGTH];     /**< Server hostname (e.g., 'ftp.up.pt') */
    char resource[MAX_LENGTH]; /**< Resource path (e.g., 'pub/file.txt') */
    char file[MAX_LENGTH];     /**< File name extracted from resource path */
    char user[MAX_LENGTH];     /**< Username for authentication */
    char password[MAX_LENGTH]; /**< Password for authentication */
    char ip[MAX_LENGTH];       /**< Resolved IP address of the host */
};

/* Function Prototypes */

/**
 * @brief Parses an FTP URL into its components
 * 
 * @param input The FTP URL to parse
 * @param url Pointer to URL structure to store parsed components
 * @return int 0 on success, -1 on failure
 */
int parse(char *input, struct URL *url);

/**
 * @brief Creates and connects a socket to the specified address
 * 
 * @param ip Server IP address
 * @param port Server port number
 * @return int Socket file descriptor on success, -1 on failure
 */
int createSocket(char *ip, int port);

/**
 * @brief Closes an FTP connection properly
 * 
 * @param sock Socket to close
 * @return int 0 on success, -1 on failure
 */
int closeConnection(int sock);

/**
 * @brief Authenticates with the FTP server
 * 
 * @param sock Control socket
 * @param user Username
 * @param pass Password
 * @return int 0 on success, -1 on failure
 */
int authenticate(int sock, const char *user, const char *pass);

/**
 * @brief Enters passive mode for data transfer
 * 
 * @param sock Control socket
 * @param addr Buffer to store data connection address
 * @param port Pointer to store data connection port
 * @return int 0 on success, -1 on failure
 */
int enterPassiveMode(int sock, char *addr, int *port);

/**
 * @brief Reads and parses server response
 * 
 * @param sock Socket to read from
 * @param buffer Buffer to store response
 * @return int Response code on success, -1 on failure
 */
int getServerResponse(int sock, char *buffer);

/**
 * @brief Downloads a file from the server
 * 
 * @param ctrlSock Control socket
 * @param dataSock Data socket
 * @param filename Name of file to save
 * @return int 0 on success, -1 on failure
 */
int downloadFile(int ctrlSock, int dataSock, char *filename);

/**
 * @brief Requests a file from the server
 * 
 * @param sock Control socket
 * @param path Path of file to request
 * @return int 0 on success, -1 on failure
 */
int requestFile(int sock, char *path);

#endif /* FTP_CLIENT_H */ 