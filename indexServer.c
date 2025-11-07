/* file_server.c - main */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>

struct pdu {
    char type;
    char data[100];    
};

struct sData {
    char pName[10];
    char cName[90];
};

struct indexData {
	char peerName[10];
	char contentName[10];
	struct  sockaddr_in address;
    int uses;
};

struct indexData RegisteredContent[];
int registeredCount = 0;

struct indexData* findContentByName(const char* contentName, struct indexData* RegisteredContent, int registeredCount) {
    int leastUses = -1;
    struct indexData* result = NULL;
    char buffer[100];
    char temp[100];
    memcpy(temp, contentName, sizeof(temp)-1);

    temp[strcspn(temp, "\n")] = 0; // Remove newline character if present
    for (int i = 0; i < registeredCount; i++) {
        if ( strcmp(RegisteredContent[i].contentName, temp) == 0 && (leastUses == -1 || RegisteredContent[i].uses < leastUses)) {
            leastUses = RegisteredContent[i].uses;
            result = &RegisteredContent[i];
        } 
    }
    result->uses += 1; // Increment usage count
    return result;
}
/*------------------------------------------------------------------------
 * main - Iterative UDP server for TIME service
 *------------------------------------------------------------------------
 */

int
main(int argc, char *argv[])
{
    struct  sockaddr_in fsin;    /* the from address of a client    */
    char    buf[100];        /* "input" buffer; any size > 0    */
    char    *pts;
    int    sock;            /* server socket        */
    time_t    now;            /* current time            */
    int    alen;            /* from-address length        */
    struct  sockaddr_in sin; /* an Internet endpoint address         */
        int     s, type;        /* socket descriptor and socket type    */
    int     port=3000;

    switch(argc){
        case 1:
            break;
        case 2:
            port = atoi(argv[1]);
            break;
        default:
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
            exit(1);
    }


        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
        sin.sin_port = htons(port);
                                                                                                 
    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
        fprintf(stderr, "can't creat socket\n");
                                                                               
    /* Bind the socket */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        fprintf(stderr, "can't bind to %d port\n",port);
        listen(s, 5);    
    alen = sizeof(fsin);

    while (1) {   
    
    // File Pointer declared
    FILE* ptr;
    char full_path[512];
    // File opened
    char    *bp, buf[256],fbuf[101]; // 1 byte for flag, 100 bytes of data
    int     n, bytes_to_read;
    int ch; // Use int to store the character, as EOF is an int
    int count=0;

    struct pdu spdu;
    struct stat file_info;

    n = recvfrom(s, &spdu, sizeof(buf), 0, (struct sockaddr *)&fsin, &alen);
    printf("Received request w/ data: %s\n", spdu.data);
   // buf[n] = '\0'; // Null-terminate the received string

    switch (spdu.type) {
        case 'R':
            // Handle registration
            // (Implementation for registration can be added here)
            break;
        case 'O':
            // Handle query
            // (Implementation for query can be added here)
            break;
        case 'S':
            struct sData sd;
            memcpy(&sd, &spdu.data, sizeof(sd)-1);
            printf("Request sent to server for name: %s", sd.pName);
            printf("Request sent to server for file: %s", sd.cName); 

            struct indexData* content = findContentByName(sd.cName, RegisteredContent, registeredCount);  
            if(content != NULL) {
                content->uses += 1; // Increment usage count
                struct pdu responsePdu;
                responsePdu.type = 'S'; // A for Acknowledge
                memcpy(responsePdu.data, sd.pName, sizeof(sd.pName)-1);
                memcpy(&responsePdu.data[10], &(content->address), sizeof(content->address)-1);
                sendto(s, &responsePdu, sizeof(responsePdu), 0, (struct sockaddr *)&fsin, alen);
            } else {
                struct pdu responsePdu;
                responsePdu.type = 'E'; // E for Error
                char* errorMsg = "Content not found";
                strncpy(responsePdu.data, errorMsg, sizeof(responsePdu.data)-1);
                sendto(s, &responsePdu, sizeof(responsePdu), 0, (struct sockaddr *)&fsin, alen);
            }
            break;
        case 'T':
            // Handle De-registration
            // (Implementation for download can be added here)
            break;
        default:
            // Error: Invalid Use
            // (Implementation for other cases can be added here)
            break;
        }
    }
}


