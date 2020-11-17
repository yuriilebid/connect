#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h>
#include <sys/socket.h> 
#include <unistd.h>
#include <netinet/in.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 1500

int main() {
    int net_socket;

    net_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_adr;

    server_adr.sin_family = AF_INET;
    server_adr.sin_port = htons(1500);
    server_adr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int connection_status = connect(net_socket, (struct sockaddr *) &server_adr, sizeof(server_adr));

    if(connection_status != 0) {
        printf("Connection failed!\n");
        exit(1);
    }
    else {
        printf("Connection done!\n");
    }

    char server_response[128];

    bzero(server_response, 128);
    recv(net_socket, &server_response, sizeof(server_response), 0);

    printf("Data from server: %s", server_response);
    close(net_socket);
    return 0;
}
