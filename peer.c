

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
}
