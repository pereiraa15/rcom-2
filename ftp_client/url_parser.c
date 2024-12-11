/**
 * @file url_parser.c
 * @brief URL parsing implementation for FTP client
 *
 * This file implements the URL parsing functionality for the FTP client.
 * It handles two URL formats:
 * 1. ftp://<host>/<url-path>
 * 2. ftp://[<user>:<password>@]<host>/<url-path>
 *
 * The parser extracts:
 * - Host information
 * - Authentication credentials (if provided)
 * - Resource path
 * - Filename
 * And resolves the host to its IP address.
 */

#include "ftp_client.h"

/**
 * @brief Parses an FTP URL into its components
 *
 * This function breaks down an FTP URL into its constituent parts:
 * 1. Validates URL format using regex patterns
 * 2. Extracts host, credentials, and resource information
 * 3. Resolves hostname to IP address
 *
 * URL Format Examples:
 * - Anonymous: ftp://ftp.example.com/path/to/file.txt
 * - Authenticated: ftp://user:pass@ftp.example.com/path/to/file.txt
 *
 * For anonymous URLs, default credentials (defined in ftp_client.h)
 * are used.
 *
 * @param input The FTP URL to parse
 * @param url Pointer to URL structure to store parsed components
 * @return int 0 on success, -1 on parsing failure
 */
int parse(char *input, struct URL *url)
{
    regex_t regex;

    // Check for required '/' character
    regcomp(&regex, BAR, 0);
    if (regexec(&regex, input, 0, NULL, 0))
        return -1;

    // Check for '@' to determine URL format
    regcomp(&regex, AT, 0);
    if (regexec(&regex, input, 0, NULL, 0) != 0)
    { 
        // Format: ftp://<host>/<url-path>
        sscanf(input, HOST_REGEX, url->host);
        strcpy(url->user, DEFAULT_USER);
        strcpy(url->password, DEFAULT_PASSWORD);
    }
    else
    { 
        // Format: ftp://[<user>:<password>@]<host>/<url-path>
        sscanf(input, HOST_AT_REGEX, url->host);
        sscanf(input, USER_REGEX, url->user);
        sscanf(input, PASS_REGEX, url->password);
    }

    // Extract resource path and filename
    sscanf(input, RESOURCE_REGEX, url->resource);
    strcpy(url->file, strrchr(input, '/') + 1);

    // Validate host and resolve IP address
    struct hostent *h;
    if (strlen(url->host) == 0)
        return -1;
    if ((h = gethostbyname(url->host)) == NULL)
    {
        printf("Invalid hostname '%s'\n", url->host);
        exit(-1);
    }
    strcpy(url->ip, inet_ntoa(*((struct in_addr *)h->h_addr)));

    // Verify all required fields are present
    return !(strlen(url->host) && strlen(url->user) &&
             strlen(url->password) && strlen(url->resource) && strlen(url->file));
} 