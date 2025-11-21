

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <arpa/inet.h>

#include <netdb.h>



#define BUFLEN        256    /* buffer length */
#define MAX_CONTENT 10


int echod(int);
int filed(int);
void reaper(int);

// Main PDU Structure
struct pdu {
    char type;
    char data[100];    
};

struct rData {
    char pName[10];
    char cName[10]; // MAY NEED TO MODIFY
    struct  sockaddr_in address;
};

// File Search Structure
struct sData {
    char pName[10];
    char cName[90];
};

struct contentEntry {
    char cName[90];
    int tcp_socket;
    struct sockaddr_in address;
};

struct contentEntry registered_content[MAX_CONTENT];
int content_count = 0;
int main_udp_socket;

int registerContent(int udp_sock, char* pName, char* cName);
void handleContentDownload(int tcp_sock);
int deregisterContent(int udp_sock, char* pName, int index, fd_set *afds);
void cleanupAndExit(int udp_sock, char* pName, fd_set *afds);


int main(int argc, char **argv)
{
    char    *host = "localhost";
    char ans[BUFLEN];
    char buf[BUFLEN];
    char pName[BUFLEN];
    int i;
    
    int port = 3000;
    struct hostent  *phe;   /* pointer to host information entry    */
    struct sockaddr_in sin; /* an Internet endpoint address     */
    int s, n, type; /* socket descriptor and socket type    */


    switch (argc) {
    case 1:
        break;
    case 2:
        host = argv[1];
    case 3:
        host = argv[1];
        port = atoi(argv[2]);
        break;
    default:
        fprintf(stderr, "usage: UDPtime [host [port]]\n");
        exit(1);
    }


    memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;                                                                
        sin.sin_port = htons(port);
                                                                                       
    /* Map host name to IP address, allowing for dotted decimal */
    if ( (phe = gethostbyname(host)) ){
        memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
    }
    else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
    fprintf(stderr, "Can't get host entry \n");
                                                                            
    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        fprintf(stderr, "Can't create socket \n");
        exit(1);
    }
    main_udp_socket = s;

                                                                               
    /* Connect the socket */
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        fprintf(stderr, "Can't connect to %s %s \n", host, "Time");
    }

    fd_set rfds, afds;
    FD_ZERO(&afds);
    FD_SET(s, &afds); /* Listening on a UDP socket */
    FD_SET(0, &afds); /* Listening on stdin */
    
    printf("Enter your Peer Name (Max only be 9 char):\n");
    while(1)
    {
        memset(pName, '\0', sizeof(buf)); // Clears the entire array
        n = read(0, pName, BUFLEN);
        pName[n] = 0;
        if(n<=9){
            break;
        }
        printf("Max 9 char, try again:\n");
    }

    int max_fd = s;
    
    while(1)
    {
        printf("What would you like to do next?(Enter Number):\n"
                "    1. Register Content \n"
                "    2. Request Content Download\n"
                "    3. De-register Content\n"
                "    4. List all Content Available\n"
                "    5. Exit\n"
        );

        memcpy(&rfds, &afds, sizeof(rfds));
        select(FD_SETSIZE, &rfds, NULL, NULL, NULL);

        // Check if UDP socket is sending messages
        if (FD_ISSET(s, &rfds)) {
            /* Socket is readable */
            n = recv(s, ans, BUFLEN, 0);
            ans[n] = 0;
            printf("From server: %s\n", ans);
        }
        /* Check all registered content TCP sockets for incoming connections */
        for (i = 0; i < content_count; i++) {
            if (FD_ISSET(registered_content[i].tcp_socket, &rfds)) {
                struct sockaddr_in client_addr;
                socklen_t alen = sizeof(client_addr);
                int new_sock = accept(registered_content[i].tcp_socket, 
                                     (struct sockaddr *)&client_addr, &alen);
                if (new_sock >= 0) {
                    printf("Accepted download request for %s\n", 
                           registered_content[i].cName);
                    // Fork or handle download in separate thread
                    if (fork() == 0) {
                        // Child process handles the download
                        handleContentDownload(new_sock);
                        exit(0);
                    }
                    close(new_sock); // Parent closes the socket
                }
            }
        }
        // Check if stdin is sending messages
        if (FD_ISSET(0, &rfds)) {
            memset(buf, '\0', sizeof(buf)); // Clears the entire array
            n = read(0, buf, BUFLEN);
            buf[n] = 0;
            
            // Register Content
            if (strncmp(buf, "1", 1) == 0) {
                
                char fileName[BUFLEN];
                printf("Enter the name of the file you want to register:\n");
                memset(fileName, '0', sizeof(fileName)); // Clears the entire array
                n = read(0, fileName, BUFLEN);
                if (n > 9) {
                    printf("Max 9 char, try again:\n");
                    break;
                }
                fileName[n] = 0;
                fileName[strcspn(fileName, "\n")] = 0; // Remove newline character if present

                if (registerContent(s, pName, fileName) == 0) {
                    printf("Content '%s' registered successfully!\n", fileName);
                    // Update the file descriptor set with new TCP socket
                    FD_SET(registered_content[content_count - 1].tcp_socket, &afds);
                    if (registered_content[content_count - 1].tcp_socket > max_fd) {
                        max_fd = registered_content[content_count - 1].tcp_socket;
                    }
                } else {
                    printf("Failed to register content.\n");
                }
            }
            
            // Request Content Download
            else if (strncmp(buf, "2", 1) == 0) {
                //Download code
                struct pdu pduSend;
                printf("Enter the name of the file you want to download:\n");
                memset(buf, '0', sizeof(buf)); // Clears the entire array
                n = read(0, buf, BUFLEN);
                buf[n] = 0;
                buf[strcspn(buf, "\n")] = 0; // Remove newline character if present

                pduSend.type = 'S'; // S for search

                struct sData sd;
                strncpy(sd.pName, pName, sizeof(sd.pName)-1);
                strncpy(sd.cName, buf, sizeof(sd.cName)-1);

                memcpy(pduSend.data, &sd, sizeof(sd));
                printf("Request sent to server for name: %s\n", pduSend.data);
                printf("Request sent to server for file: %s\n", &pduSend.data[10]);

                write(s, &pduSend, sizeof(pduSend));
                memset(buf, '0', sizeof(buf)); // Clears the entire array
                read(s, buf, BUFLEN);
                memset(&pduSend, '0', sizeof(pduSend));

                // TODO: I think this is where the problem is, reading into pduSend when it's type indexData or something
                memcpy(&pduSend, buf, sizeof(pduSend));   
                printf("Response from server received with type: %c\n", pduSend.type);
                printf("Response from server received with data: %s\n", &pduSend.data[10]);
                
                if(pduSend.type == 'S'){
                    struct  sockaddr_in address;
                    memcpy(&address, &pduSend.data[10], sizeof(address));
                    /* Allocate a socket */
                    int s2;
                    s2 = socket(AF_INET, SOCK_DGRAM, 0);
                    if (s2 < 0) {
                        fprintf(stderr, "Can't create socket \n");
                    }
                                                                               
                    /* Connect the socket */
                    if (connect(s2, (struct sockaddr *)&address, sizeof(address)) < 0) {

                        fprintf(stderr, "Can't connect to %s \n", host);
                    }

                    // Now request the file from the peer
                    struct pdu filePdu;
                    filePdu.type = 'D'; // D for Download Request
                    memcpy(filePdu.data, &sd, sizeof(sd));

                    write(s2, &filePdu, sizeof(filePdu));

                    // Receive the file data
                    FILE* ptr;
                    char fileBuf[101];

                    ptr = fopen(sd.cName, "w+");        /* pointer to file */
                    if (ptr == NULL) {
                        printf("error opening file\n");
                        close(s2);
                        cleanupAndExit(s, pName, &afds);
                        return(0);    
                    }
                    int bytes_to_read;
                    struct pdu recvPdu;

                    while (strchr(recvPdu.data,EOF) == NULL) {
                        bytes_to_read = read(s2, fileBuf, 101);
                        memcpy(&recvPdu, fileBuf, sizeof(recvPdu));                    
                        fwrite(recvPdu.data, 1, bytes_to_read, ptr);
                        printf("Received %d bytes of file data\n", bytes_to_read - 1);
                    }
                    fclose(ptr);
                    close(s2);
                    printf("File %s downloaded successfully from peer.\n", sd.cName);

                    registerContent(s, pName, sd.cName);
                }
                else if(pduSend.type == 'E'){
                    printf("Error from server: %s\n", pduSend.data);
                }
            }

            // De-register Content
            else if (strncmp(buf, "3", 1) == 0) {
                //De - Register code
                if (content_count == 0) {
                    printf("No content registered to de-register.\n");
                } else {
                    
                    printf("what would you like to de-register?(Enter file name):\n");
                    for (i = 0; i < content_count; i++) {
                        printf("%2d) %s (peer: %s)\n", i + 1, registered_content[i].cName, pName);
                    }
                    memset(buf, '0', sizeof(buf)); // Clears the entire array
                    n = read(0, buf, BUFLEN);
                    buf[n] = 0;
                    buf[strcspn(buf, "\n")] = 0; // Remove newline character if present
    
                    // if (n != 1) {
                    //     printf("Invalid selection. Please try again.\n");
                    //     break;
                    // }
                    int index = atoi(buf) - 1;
                    if (index < 0 || index >= content_count) {
                        printf("Invalid selection. Please try again.\n");
                    } else {
                        deregisterContent(s, pName, index, &afds);
                    }
                }
                
                // deregisterContent(s, pName, buf);
            }

            // List Contents
            else if (strncmp(buf, "4", 1) == 0) {
                //List Contents code

                // send list request (type 'O' PDU) to server
                struct pdu listPdu;
                listPdu.type = 'O'; // O for List Request
                listPdu.data[0] = 0; // No additional data needed
                write(s, &listPdu, sizeof(listPdu));
                memset(buf, '0', sizeof(buf)); // Clears the entire array
                read(s, buf, BUFLEN);
                memset(&listPdu, '0', sizeof(listPdu));
                memcpy(&listPdu, buf, sizeof(listPdu));   
                printf("Response from server received with type: %c\n", listPdu.type);
                printf("Available Content List:\n%s\n", listPdu.data);

            }

            // Exit
            else if (strncmp(buf, "5", 1) == 0) {
                //exit code
                cleanupAndExit(s, pName, &afds);
                
            }
            else{
                printf("Invalid Option. Please try again.\n");
            }
        }
    }
}



int registerContent(int udp_sock, char* pName, char* cName)
{
    if (content_count >= MAX_CONTENT) {
        fprintf(stderr, "Maximum content limit reached\n");
        return -1;
    }
    
    // Create a TCP socket for this content
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        fprintf(stderr, "Can't create TCP socket\n");
        return -1;
    }
    
    // Bind to an automatically assigned port
    struct sockaddr_in reg_addr;
    memset(&reg_addr, 0, sizeof(reg_addr));
    reg_addr.sin_family = AF_INET;
    reg_addr.sin_port = htons(0); // Let OS choose port
    reg_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(tcp_sock, (struct sockaddr *)&reg_addr, sizeof(reg_addr)) < 0) {
        fprintf(stderr, "Can't bind TCP socket\n");
        close(tcp_sock);
        return -1;
    }
    
    // Start listening
    if (listen(tcp_sock, 5) < 0) {
        fprintf(stderr, "Can't listen on TCP socket\n");
        close(tcp_sock);
        return -1;
    }
    
    // Get the actual address assigned
    socklen_t alen = sizeof(reg_addr);
    if (getsockname(tcp_sock, (struct sockaddr *)&reg_addr, &alen) < 0) {
        fprintf(stderr, "Can't get socket name\n");
        close(tcp_sock);
        return -1;
    }
    
    printf("TCP server started on port %d for file %s\n", 
           ntohs(reg_addr.sin_port), cName);
    
    // Send registration PDU to index server
    struct pdu regPdu;
    regPdu.type = 'R'; // R for register
    
    struct rData rd;
    strncpy(rd.pName, pName, sizeof(rd.pName) - 1);
    strncpy(rd.cName, cName, sizeof(rd.cName) - 1);
    rd.address = reg_addr;
    
    memcpy(regPdu.data, &rd, sizeof(rd));
    
    if (write(udp_sock, &regPdu, sizeof(regPdu)) < 0) {
        fprintf(stderr, "Failed to send registration to server\n");
        close(tcp_sock);
        return -1;
    }
    
    // wait for acknowledgment from server
    struct pdu ackPdu;
    read(udp_sock, &ackPdu, sizeof(ackPdu));
    if (ackPdu.type != 'A') {
        printf("Error from Server: %s\n", ackPdu.data);
        close(tcp_sock);
        return -1;
    } else {
        // Store in local registry (registered_content array)
        strncpy(registered_content[content_count].cName, cName, 
                sizeof(registered_content[content_count].cName) - 1);

        registered_content[content_count].tcp_socket = tcp_sock;
        registered_content[content_count].address = reg_addr;
        content_count++;
        return 0;

    }
    
    return 0;
}

int deregisterContent(int udp_sock, char * pName, int index, fd_set *afds) {
    struct pdu deregPdu;
    struct rData deregData;
    int i;
    
    deregPdu.type = 'T'; // T for de-register
    strncpy(deregData.pName, pName, sizeof(deregData.pName) - 1); // Peer name
    strncpy(deregData.cName, registered_content[index].cName, sizeof(deregData.cName) - 1); // Content name
    deregData.address = registered_content[index].address;
    memcpy(deregPdu.data, &deregData, sizeof(deregData));
    write(udp_sock, &deregPdu, sizeof(deregPdu));

    // wait for acknowledgment from server using recv() for UDP socket
    struct pdu ackPdu;
    char buf[BUFLEN];
    memset(buf, 0, sizeof(buf));
    int n = recv(udp_sock, buf, BUFLEN, 0);

    if (n > 0) {
        memcpy(&ackPdu, buf, sizeof(ackPdu));
        if (ackPdu.type != 'A') {
            printf("Error from Server: %s\n", ackPdu.data);
            return -1;
        } else {
            printf("Content '%s' de-registered successfully!\n", deregData.cName);
            
            // Remove the TCP socket from fd_set before closing
            FD_CLR(registered_content[index].tcp_socket, afds);
            close(registered_content[index].tcp_socket);
            
            // Shift remaining entries
            for (i = index; i < content_count - 1; i++) {
                registered_content[i] = registered_content[i + 1];
            }
            content_count--;
            return 0;
        }
    } else {
        printf("Failed to receive acknowledgment from server.\n");
        return -1;
    }
}

void handleContentDownload(int tcp_sock)
{
    struct pdu reqPdu;
    
    // Read download request
    if (read(tcp_sock, &reqPdu, sizeof(reqPdu)) <= 0) {
        close(tcp_sock);
        return;
    }
    
    if (reqPdu.type != 'D') {
        close(tcp_sock);
        return;
    }
    
    // Extract filename from request
    struct sData sd;
    memcpy(&sd, reqPdu.data, sizeof(sd));
    
    printf("Serving file: %s to peer\n", sd.cName);
    
    // Open and send the file
    FILE *fp = fopen(sd.cName, "rb");
    if (fp == NULL) {
        struct pdu errPdu;
        errPdu.type = 'E';
        strcpy(errPdu.data, "File not found");
        write(tcp_sock, &errPdu, sizeof(errPdu));
        close(tcp_sock);
        return;
    }
    
    struct pdu dataPdu;
    size_t bytes_read;
    
    while ((bytes_read = fread(dataPdu.data, 1, 100, fp)) > 0) {
        dataPdu.type = 'D';
        // Pad with zeros if less than 100 bytes
        if (bytes_read < 100) {
            memset(dataPdu.data + bytes_read, 0, 100 - bytes_read);
        }
        write(tcp_sock, &dataPdu, sizeof(dataPdu));
    }
    
    // Send EOF marker
    dataPdu.type = 'E';
    memset(dataPdu.data, 0, 100);
    write(tcp_sock, &dataPdu, sizeof(dataPdu));
    
    fclose(fp);
    close(tcp_sock);
    printf("File transfer complete for %s\n", sd.cName);
}


void cleanupAndExit(int udp_sock, char* pName, fd_set *afds)
                {
                    while (content_count > 0) {
                        deregisterContent(udp_sock, pName, 0, afds);
                    }
                    close(udp_sock);
                    exit(0);
                }