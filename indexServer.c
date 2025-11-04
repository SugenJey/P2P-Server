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

struct indexData {
	char *peerName;
	char *contentName
	int address;
}

indexData RegisteredServers[100]; // array of indexData



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








    while (1) {
    struct pdu spdu;
    struct stat file_info;


    n = recvfrom(s, &spdu, sizeof(buf), 0, (struct sockaddr *)&fsin, &alen);
    printf("Received request for file: %s\n", spdu.data);
    buf[n] = '\0'; // Null-terminate the received string
    ptr = fopen( spdu.data, "r");


    // Failed Condition
    if (ptr == NULL) {
      // Send error message with '0' prefix
      spdu.type = 'E';
    strcpy(spdu.data, "error: File not found");

      sendto(s, &spdu, sizeof(spdu), 0,(struct sockaddr *)&fsin, sizeof(fsin));




      printf("File Not Found. Sending Error Message.\n");


      return(0);
      }
    lstat(spdu.data, &file_info);
    long file_size = file_info.st_size;
    while (file_size>100) {
        spdu.type = 'D';
        bytes_to_read = fread(spdu.data, 1, 100, ptr); // Read up to 100 bytes
        file_size -= bytes_to_read;
    spdu.data[strlen(spdu.data)] = '\0';
        sendto(s, &spdu, sizeof(spdu), 0,(struct sockaddr *)&fsin, sizeof(fsin));
        printf("Sent %d bytes of file data.\n", bytes_to_read);
    memset(spdu.data,'\0',sizeof(spdu.data));
    
    }


        spdu.type = 'F';
        bytes_to_read = fread(spdu.data, 1, file_size, ptr); // Read up to 100 bytes
    spdu.data[strlen(spdu.data)] = '\0';
        file_size -= bytes_to_read;
      sendto(s, &spdu, sizeof(spdu), 0,(struct sockaddr *)&fsin, sizeof(fsin));
        printf("Final Sent %d bytes of file data.\n", bytes_to_read);
    memset(spdu.data,'\0',sizeof(spdu.data));

    fclose(ptr);
    }
    }
}

