# FTP Client Implementation

## Overview

This project implements a command-line FTP client in C, designed to download files from FTP servers. The implementation strictly adheres to RFC959 (File Transfer Protocol) and RFC1738 (URL syntax) specifications, providing a robust and standards-compliant solution for file transfers over FTP.

The client supports both anonymous and authenticated access, implementing the essential subset of FTP commands necessary for file downloading. A key feature is its passive mode implementation, which allows it to work effectively even behind firewalls and NAT configurations. The client provides real-time progress tracking during downloads, making it user-friendly and practical for file transfer operations.

## Client-Server Architecture in TCP/IP

### Understanding the Model
The implementation demonstrates the fundamental client-server architecture in TCP/IP:

1. **Control Connection**
   - Client initiates connection to server's well-known port (21)
   - Persistent TCP connection for commands
   - Bidirectional command-response flow

2. **Data Connection**
   - Separate connection for file transfers
   - Passive mode implementation for NAT compatibility
   - Dynamic port allocation

3. **State Management**
   - Connection establishment
   - Authentication
   - Data transfer
   - Session termination

### TCP/IP Peculiarities
The implementation handles several TCP/IP specific aspects:
- Connection establishment (3-way handshake)
- Reliable data transfer
- Flow control
- Connection termination

## Protocol Implementation

### FTP Protocol Behavior (RFC959)

1. **Connection Phase**
   ```
   CLIENT                SERVER
   ------                ------
   connect to port 21 →  
                     ←   220 Welcome
   ```

2. **Authentication Phase**
   ```
   USER xxx         →
                     ←   331 Need password
   PASS xxx         →
                     ←   230 Logged in
   ```

3. **Passive Mode Negotiation**
   ```
   PASV             →
                     ←   227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
   [Data Connection Establishment]
   ```

4. **File Transfer Phase**
   ```
   RETR filename    →
                     ←   150 Opening data connection
   [Data Transfer]
                     ←   226 Transfer complete
   ```

5. **Session Termination**
   ```
   QUIT             →
                     ←   221 Goodbye
   ```

### URL Syntax (RFC1738)
The implementation follows RFC1738 specifications for URL parsing:

1. **Basic Format**
   ```
   ftp://[<user>:<password>@]<host>/<url-path>
   ```

2. **Components**
   - Scheme: "ftp://"
   - Authentication: optional user:password
   - Host: domain name or IP address
   - Path: resource location

3. **Special Cases**
   - Anonymous access (default credentials)
   - Special characters in passwords
   - Path encoding

## Technical Implementation

### DNS Resolution
The implementation uses DNS services through:

1. **gethostbyname Function**
   ```c
   struct hostent *h;
   h = gethostbyname(hostname);
   ```
   - Resolves hostnames to IP addresses
   - Handles multiple IP addresses
   - Error checking for resolution failures

2. **DNS Integration**
   - Automatic fallback to IP addresses
   - Hostname validation
   - Error handling for DNS failures

### Socket Programming

1. **Control Socket**
   ```c
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
   ```
   - TCP socket creation
   - Connection establishment
   - Error handling

2. **Data Socket**
   - Passive mode socket creation
   - Dynamic port handling
   - Data transfer management

### File Transfer Process

1. **Success Path**
   ```
   Connect → Login → Passive → Get Path → Download → Success
   ```

2. **Error Handling**
   - Connection failures
   - Authentication errors
   - Transfer interruptions
   - Resource cleanup

## Development Process

### Experimental Phase
The development process started with experimentation using:
- Telnet for FTP command testing
- SMTP protocol analysis
- POP protocol analysis
- HTTP protocol analysis
- FTP protocol deep dive

### Code Reuse and Integration
The implementation builds upon:
- clientTCP.c concepts for connection handling
- getIP.c for DNS resolution
- Enhanced error handling
- Improved resource management

## Building and Usage

### Prerequisites
- GCC compiler
- POSIX-compliant system
- Network connectivity

### Compilation
```bash
make
```

### Execution
```bash
./download ftp://[<user>:<password>@]<host>/<url-path>
```

### Example
```bash
./download ftp://ftp.netlab.fe.up.pt/pub/example.txt
```

## Learning Outcomes

1. **Client-Server Architecture**
   - TCP/IP protocol understanding
   - Connection management
   - State handling

2. **Protocol Implementation**
   - RFC document interpretation
   - Command-response patterns
   - Error handling strategies

3. **Network Programming**
   - Socket API usage
   - DNS resolution
   - Buffer management

4. **System Programming**
   - File operations
   - Resource management
   - Error handling

This implementation serves as a practical demonstration of network programming concepts, protocol implementation, and system programming skills, while delivering a functional FTP client that adheres to relevant standards and specifications. 