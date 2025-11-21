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
#include <arpa/inet.h>

#define MAX 100

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

struct rData {
    char pName[10];
    char cName[10]; // MAY NEED TO MODIFY
    struct  sockaddr_in address;
};

int contentExists(char* cName, char* pName, struct indexData* RegisteredContent, int registeredCount);
int sendErrorMessage(int s, struct sockaddr_in* fsin, socklen_t alen, const char* message);
struct indexData* findContentByName(const char* contentName, struct indexData* RegisteredContent, int registeredCount);

struct indexData RegisteredContent[MAX];
int registeredCount = 0;

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
    socklen_t alen;            /* from-address length        */
    struct  sockaddr_in sin; /* an Internet endpoint address         */
    int     s, type;        /* socket descriptor and socket type    */
    int     port=3000;
    int i;

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
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        fprintf(stderr, "can't bind to %d port\n",port);
    }
    
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

    n = recvfrom(s, &spdu, sizeof(spdu), 0, (struct sockaddr *)&fsin, &alen);
    printf("Received request w/ data: %s\n", spdu.data);
   // buf[n] = '\0'; // Null-terminate the received string

    switch (spdu.type) {
        case 'R': {
            // Handle registration
            // (Implementation for registration can be added here)
            struct rData rd;
            memcpy(&rd, &spdu.data, sizeof(rd)-1);
            printf("Registering content: %s from peer: %s\n", rd.cName, rd.pName);

            // Check for registration limit and existing content
            if(registeredCount > MAX) {
                fprintf(stderr, "Registration limit reached. Cannot register more content.\n");
            } 

            if (contentExists(rd.cName, rd.pName, RegisteredContent, registeredCount) != -1) {
                // Content already registered, send error message
                fprintf(stderr, "Content %s is already registered. Skipping registration.\n", rd.cName);
                sendErrorMessage(s, &fsin, alen, "Content already registered");
            } else {

                // Add new content registration
                struct indexData newContent;
                strncpy(newContent.peerName, rd.pName, sizeof(newContent.peerName)-1);
                strncpy(newContent.contentName, rd.cName, sizeof(newContent.contentName)-1);
                memcpy(&newContent.address, &rd.address, sizeof(rd.address));
                newContent.uses = 0; // Initialize usage count
                RegisteredContent[registeredCount++] = newContent;
                printf("Content %s registered successfully from peer %s.\n", rd.cName, rd.pName);

                // Acknowledge registration to client
                struct pdu okPdu;
                okPdu.type = 'A'; // A for Acknowledge
                //set data to say "Registration Successful"
                strncpy(okPdu.data, "Registration Successful", sizeof(okPdu.data)-1);
                sendto(s, &okPdu, sizeof(okPdu), 0, (struct sockaddr *)&fsin, alen);
            }
            break;
        }
        case 'O': { 
            // Handle query
            // (Implementation for query can be added here)
            struct pdu od;
            // create a string (limit to 100 bytes) with all registered content names
            char contentList[100] = "";
            for (i = 0; i < registeredCount; i++) {
                /* Append safely: always compute remaining space in the destination
                 * and pass that as the maximum number of bytes to copy. This
                 * avoids passing the literal source length (which can equal
                 * the bound and trigger -Wstringop-overflow warnings) and
                 * prevents buffer overruns.
                 */
                size_t rem = sizeof(contentList) - strlen(contentList) - 1;
                if (rem > 0) strncat(contentList, RegisteredContent[i].contentName, rem);

                rem = sizeof(contentList) - strlen(contentList) - 1;
                if (rem > 0) strncat(contentList, " from ", rem); // Space separator

                rem = sizeof(contentList) - strlen(contentList) - 1;
                if (rem > 0) strncat(contentList, RegisteredContent[i].peerName, rem);

                rem = sizeof(contentList) - strlen(contentList) - 1;
                if (rem > 0) strncat(contentList, "\n", rem); // Newline separator
            }
            // Send back the list of content names
            od.type = 'O'; // O for confirming list ready
            strncpy(od.data, contentList, sizeof(od.data)-1);
            sendto(s, &od, sizeof(od), 0, (struct sockaddr *)&fsin, alen);
            break;
        }
        case 'S': {
            struct sData sd;
            memcpy(&sd, &spdu.data, sizeof(sd)-1);
            printf("Request sent to server for name: %s\n", sd.pName);
            printf("Request sent to server for file: %s\n", sd.cName); 

            struct indexData* content = findContentByName(sd.cName, RegisteredContent, registeredCount);  
            printf("Content: %s from Peer: %s at address: %s\n", sd.cName, content != NULL ? content->peerName : "N/A", content != NULL ? inet_ntoa(content->address.sin_addr) : "N/A");
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
        }
        case 'T': {
            // Handle De-registration
            // (Implementation for download can be added here)
            struct sData td;
            memcpy(&td, &spdu.data, sizeof(td)-1);
            // Verify if content exists before de-registering
            int index = contentExists(td.cName, td.pName, RegisteredContent, registeredCount);
            if (index == -1) {
                fprintf(stderr, "Content %s is not registered. Skipping de-registration.\n", td.cName);
                sendErrorMessage(s, &fsin, alen, "Content not registered");
            } else {
                // use index to remove the content from RegisteredContent
                // Shift elements to remove the content
                for (i = index; i < registeredCount - 1; i++) {
                    RegisteredContent[i] = RegisteredContent[i + 1];
                }
                registeredCount--;
                printf("Content %s de-registered successfully from peer %s.\n", td.cName, td.pName);

                // Send Acknowledgment to client
                struct pdu okPdu;
                okPdu.type = 'A'; // A for Acknowledge
                //set data to say "De-registration Successful"
                strncpy(okPdu.data, "De-registration Successful", sizeof(okPdu.data)-1);
                sendto(s, &okPdu, sizeof(okPdu), 0, (struct sockaddr *)&fsin, alen);

            }

            continue;
        }
        default:
            // Error: Invalid Use
            // (Implementation for other cases can be added here)
            sendErrorMessage(s, &fsin, alen, "Invalid request type");
            break;
        }
    }
}



int contentExists(char* cName, char* pName,struct indexData* RegisteredContent, int registeredCount) {
    int i;
    for (i = 0; i < registeredCount; i++) {
        if ((strcmp(RegisteredContent[i].contentName, cName) == 0) && (strcmp(RegisteredContent[i].peerName, pName) == 0)) {
            return i; // Content exists
        }
    }
    return -1; // Content does not exist
}

int sendErrorMessage(int s, struct sockaddr_in* fsin, socklen_t alen, const char* message) {
    struct pdu errorPdu;
    errorPdu.type = 'E'; // E for Error
    strncpy(errorPdu.data, message, sizeof(errorPdu.data)-1);
    return sendto(s, &errorPdu, sizeof(errorPdu), 0, (struct sockaddr *)fsin, alen);
}



struct indexData* findContentByName(const char* contentName, struct indexData* RegisteredContent, int registeredCount) {
    int leastUses = -1;
    int i;
    struct indexData* result = NULL;
    char buffer[100];
    char temp[100];
    memcpy(temp, contentName, sizeof(temp)-1);

    temp[strcspn(temp, "\n")] = 0; // Remove newline character if present
    for (i = 0; i < registeredCount; i++) {
        if ( strcmp(RegisteredContent[i].contentName, temp) == 0 && (leastUses == -1 || RegisteredContent[i].uses < leastUses)) {
            leastUses = RegisteredContent[i].uses;
            result = &RegisteredContent[i];
        } 
    }
    if (result != NULL) {
        result->uses += 1; // Increment usage count
    }
    return result;
}