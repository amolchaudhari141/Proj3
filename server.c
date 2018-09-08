#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <signal.h>
#include <wait.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <asm/param.h> 

#define PORT    33104
#define BUFMAX 	2048             
#define MAX     1024

static char buf[MAX];
unsigned int queryType = 0, queryAcctNumber = 0;
float queryValue = 0;

struct record {
    int acctnum; /* unique key in sorted order */
    char name[20];
    float value;
    char phone[12];
    int age;
};

void signal_catcher(int the_sig) {
    wait(0); // cleanup the zombie 
}

/*void strreplace(char s[]) {
    int i;
    for (i = 0; s[i] != '\0'; i++) {
        if (s[i] == '.') {
            s[i] = ',';
        }
    }
}
*/
void resetQueryInput() {
    queryType = 0;
    queryAcctNumber = 0;
    queryValue = 0;
}

int main() {

    /* Find local host's IP address and create MSG to send to servicemap */
    struct hostent * hostEntity;
    struct in_addr hostIP;

    char * hostname = malloc(MAXHOSTNAMELEN);
    memset(hostname, 0, MAXHOSTNAMELEN);
    gethostname(hostname, MAXHOSTNAMELEN);
    hostEntity = gethostbyname(hostname);
  /*  if (hostEntity) {
        while ( * hostEntity -> h_addr_list) {
            bcopy( * hostEntity -> h_addr_list++, (char * ) & hostIP, sizeof(hostIP));
        }
    } else {
        printf("Getting Local IP Error\n");
    }*/
    const char * serviceCommand = "PUT ";
    const char * serviceName = "BANK620 ";
    const char * servicePort = ",129,80";

    char * tempServiceIP = inet_ntoa(hostIP);
    strreplace(tempServiceIP);
    const char * updatedServiceIP = tempServiceIP;

    char * serviceMsg;
    serviceMsg = malloc(strlen(serviceCommand) + strlen(serviceName) + strlen(updatedServiceIP) + strlen(servicePort) + 1);
    strcpy(serviceMsg, serviceCommand);
    strcat(serviceMsg, serviceName);
    strcat(serviceMsg, updatedServiceIP);
    strcat(serviceMsg, servicePort);
    /* End create service message to send to service map */

    /* Begin sending MSG to servicemap */
    int sk;
    char returnMsg[BUFMAX];

    struct sockaddr_in remote, serviceMap;
    socklen_t serviceMapSize = sizeof(serviceMap);
    bzero( & serviceMap, sizeof(serviceMap));
    struct hostent * hp;

    /* Create an Internet domain datagram socket */
    sk = socket(AF_INET, SOCK_DGRAM, 0);

    remote.sin_family = AF_INET;
    remote.sin_port = ntohs(PORT);
    remote.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    /* setsockopt is required on Linux, but not on Solaris */
    if (setsockopt(sk, SOL_SOCKET, SO_BROADCAST, (struct sockaddr * ) & remote, sizeof(remote)) < 0) {
        perror("Setting broadcast option error");
    }

    /* set timeout to handle case were service map is unavailable */
    //struct timeval tv;
    //tv.tv_sec = 5;
    //tv.tv_usec = 0;
    //if (setsockopt(sk, SOL_SOCKET, SO_RCVTIMEO, & tv, sizeof(tv)) < 0) {
    //    perror("Setting timeout error");
    //}

    /* Send the message */
    if (sendto(sk, serviceMsg, strlen(serviceMsg) + 1, 0, (struct sockaddr * ) & remote, sizeof(remote)) < 0) {
        perror("Unable to send broadcast message to service map");
        return 8;
    }

    /* Get the reply */
    if (recvfrom(sk, returnMsg, sizeof(returnMsg), 0, (struct sockaddr * ) & serviceMap, & serviceMapSize) < 0) {
        perror("Unable to receive service map response");
        return 7;
    }

    struct sockaddr_in * ipV4 = (struct sockaddr_in * ) & serviceMap;
    struct in_addr ipAddr = ipV4 -> sin_addr;
    char serviceMapAddressString[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, & ipAddr, serviceMapAddressString, INET_ADDRSTRLEN);
    printf("Registration %s from %s\n", returnMsg, serviceMapAddressString);

    close(sk);
    /* End sending MSG to servicemap */

    /* Begin waiting for query from client */
    struct record acct, updatedAcct;
    int fd;

    int orig_sock, // Original socket in server
    new_sock; // New socket from connect
    socklen_t clnt_len; // Length of client address
    struct sockaddr_in clnt_adr, serv_adr; // client and  server addresses
    int len, i; // Misc counters, etc.
    // Catch when child terminates

    fd = open("db17", O_RDWR);

    if (signal(SIGCHLD, signal_catcher) == SIG_ERR) {
        perror("SIGCHLD");
        return 1;
    }
    if ((orig_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("generate error");
        return 2;
    }
    serv_adr.sin_family = AF_INET; // Set address type
    serv_adr.sin_addr.s_addr = INADDR_ANY;
    serv_adr.sin_port = ntohs(PORT);

    // BIND
    if (bind(orig_sock, (struct sockaddr * ) & serv_adr,
            sizeof(serv_adr)) < 0) {
        close(orig_sock);
        perror("bind error");
        return 3;
    }
    if (listen(orig_sock, 5) < 0) { // LISTEN
        close(orig_sock);
        perror("listen error");
        return 4;
    }
    do {
        clnt_len = sizeof(clnt_adr); // ACCEPT a connect
        if ((new_sock = accept(orig_sock, (struct sockaddr * ) & clnt_adr, & clnt_len)) < 0) {
            close(orig_sock);
            perror("accept error");
            return 5;
        }
       // struct sockaddr_in * ipV4 = (struct sockaddr_in * ) & clnt_adr;
        //struct in_addr ipAddr = ipV4 -> sin_addr;
        //char ipAddressString[INET_ADDRSTRLEN];
       // inet_ntop(AF_INET, & ipAddr, ipAddressString, INET_ADDRSTRLEN);
       // printf("Service requested from: %s\n", ipAddressString);
        int callNumber = 0;

        if (fork() == 0) { // Generate a CHILD
            while ((len = recv(new_sock, buf, MAX, 0)) > 0) {
                callNumber++;
                char returnMessage[100];
                if (callNumber == 1 && queryType == 0) {
                    queryType = * (int * ) buf;
                    sprintf(returnMessage, "COMMAND_RECEIVED\n");
                    send(new_sock, returnMessage, MAX, 0); // Write back to socket					
                } else if (callNumber == 2) {
                    queryAcctNumber = * (int * ) buf;
                    if (queryType == 1001) {
                        queryAcctNumber = * (int * ) buf;

                        lseek(fd, 0, 0); //move to first byte in filelength
                        lockf(fd, F_LOCK, sizeof(struct record));
                        read(fd, & acct, sizeof(struct record));
                        lockf(fd, F_ULOCK, sizeof(struct record));

                        while (queryAcctNumber != acct.acctnum) {
                            lseek(fd, sizeof(struct record), 1); //move to next record
                            lockf(fd, F_LOCK, sizeof(struct record));
                            read(fd, & acct, sizeof(struct record));
                            lockf(fd, F_ULOCK, sizeof(struct record));
                        }
                        sprintf(returnMessage, "%s %d %0.1f %s %d\n", acct.name, acct.acctnum, acct.value, acct.phone, acct.age);
                        send(new_sock, returnMessage, MAX, 0); // Write back to socket	
                        close(new_sock); // In CHILD process

                        resetQueryInput();
                        callNumber = 0;
                        return 0;
                    } else {
                        sprintf(returnMessage, "ACCT_NUM_RECEIVED\n");
                        send(new_sock, returnMessage, MAX, 0); // Write back to socket	
                    }
                } else if (callNumber == 3 && queryType == 1002) {
                    queryValue = * (float * ) buf;
                    lseek(fd, 0, 0); //move to first byte in filelength
                    lockf(fd, F_LOCK, sizeof(struct record));
                    read(fd, & acct, sizeof(struct record));
                    lockf(fd, F_ULOCK, sizeof(struct record));
                    while (queryAcctNumber != acct.acctnum) {
                        lseek(fd, sizeof(struct record), 1); //move to next record
                        lockf(fd, F_LOCK, sizeof(struct record));
                        read(fd, & acct, sizeof(struct record));
                        lockf(fd, F_ULOCK, sizeof(struct record));
                    }
                    // Update acct amount
                    updatedAcct = acct;
                    updatedAcct.value = acct.value + queryValue;
                    lockf(fd, F_LOCK, sizeof(struct record));
                    if (write(fd, & updatedAcct, sizeof(struct record)) == 0) {
                        printf("Nothing was written\n");
                    }
                    // Query for updated acct record and send back to client
                    lockf(fd, F_ULOCK, sizeof(struct record));
                    lseek(fd, -sizeof(struct record), 1);
                    lockf(fd, F_LOCK, sizeof(struct record));
                    read(fd, & acct, sizeof(struct record));
                    lockf(fd, F_ULOCK, sizeof(struct record));
                    sprintf(returnMessage, "%s %d %0.1f\n", acct.name, acct.acctnum, acct.value);

                    send(new_sock, returnMessage, MAX, 0); // Write back to socket	
                    close(new_sock); // In CHILD process

                    resetQueryInput();
                    callNumber = 0;
                    return 0;
                }
                if (buf[0] == '.') break; // Are we done yet?
            }
        } else
            close(new_sock); // In PARENT process
    } while (1); // FOREVER

    close(fd);
    return 0;
    /* End waiting for query from client */
}