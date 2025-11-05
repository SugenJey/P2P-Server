

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




#define SERVER_TCP_PORT 3000    /* well-known port */
#define BUFLEN        256    /* buffer length */


int echod(int);
int filed(int);
void reaper(int);


int main(int argc, char **argv)
{
    char    *host = "localhost";
    char ans[BUFLEN];
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
    if ( phe = gethostbyname(host) ){
            memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
    }
    else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
    fprintf(stderr, "Can't get host entry \n");
                                                                            
    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
    fprintf(stderr, "Can't create socket \n");

                                                                               
    /* Connect the socket */
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    fprintf(stderr, "Can't connect to %s %s \n", host, "Time");

    fd_set rfds, afds;
    FD_ZERO(&afds);
    FD_SET(s, &afds); /* Listening on a UDP socket */
    FD_SET(0, &afds); /* Listening on stdin */
    while(true)
    {
        printf("What would you like to do next?(Enter Number)\n:
            1. Register Content \n
            2. Request Content Download\n 
            3. De-register Content\n
            4. List all Content Available\n
            5. Exit\n");
        memcpy(&rfds, &afds, sizeof(rfds));
        select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
        if (FD_ISSET(s, &rfds)) {
            /* Socket is readable */
            n = recv(s, ans, BUFLEN, 0);
            ans[n] = 0;
            printf("From server: %s", ans);
        }
        if (FD_ISSET(0, &rfds)) {
            n = read(0, buf, BUFSIZE);
            buf[n] = 0;
            if (strncmp(buf, "1", 1) == 0) {
                //Register code
            }
            else if (strncmp(buf, "2", 1) == 0) {
                //Download code
            }
            else if (strncmp(buf, "3", 1) == 0) {
                //De - Register code
            }
            else if (strncmp(buf, "4", 1) == 0) {
                //List ContentS code
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
