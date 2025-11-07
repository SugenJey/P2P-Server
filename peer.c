

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


int echod(int);
int filed(int);
void reaper(int);

struct pdu {
    char type;
    char data[100];    
};

struct sData {
    char pName[10];
    char cName[90];
};
int registerContent(char* pName, char* cName)
{
    // Registration code here
    return 0;
}

int main(int argc, char **argv)
{
    char    *host = "localhost";
    char ans[BUFLEN];
    char buf[BUFLEN];
    char pName[BUFLEN];
    
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

    }

                                                                               
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
    
    while(1)
    {
        printf("What would you like to do next?(Enter Number):\n1. Register Content \n2. Request Content Download\n 3. De-register Content\n4. List all Content Available\n5. Exit\n");
        memcpy(&rfds, &afds, sizeof(rfds));
        select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
        if (FD_ISSET(s, &rfds)) {
            /* Socket is readable */
            n = recv(s, ans, BUFLEN, 0);
            ans[n] = 0;
            printf("From server: %s", ans);
        }
        if (FD_ISSET(0, &rfds)) {
            memset(buf, '\0', sizeof(buf)); // Clears the entire array
            n = read(0, buf, BUFLEN);
            buf[n] = 0;
            if (strncmp(buf, "1", 1) == 0) {
                //Register code
            }
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
                printf("Request sent to server for name: %s", pduSend.data);
                printf("Request sent to server for file: %s", &pduSend.data[10]);

                write(s, &pduSend, sizeof(pduSend));
                memset(buf, '0', sizeof(buf)); // Clears the entire array
                read(s, buf, BUFLEN);
                memset(&pduSend, '0', sizeof(pduSend));
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

                    registerContent(pName, sd.cName);
                }
                else if(pduSend.type == 'E'){
                    printf("Error from server: %s\n", pduSend.data);
                }
            }
            else if (strncmp(buf, "3", 1) == 0) {
                //De - Register code
            }
            else if (strncmp(buf, "4", 1) == 0) {
                //List Contents code
            }
            else if (strncmp(buf, "5", 1) == 0) {
                //exit code
            }
            else{
                printf("Invalid Option. Please try again.\n");
            }
        }
    }
}