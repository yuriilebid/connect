#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 1500

int main() {
    char receive[128] = "echo";
    int net_socket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr("192.168.0.104"); // 0.0.0.0 // INADDR_ANY;

    bind(net_socket, (struct sockaddr_in *) &server_address, sizeof(server_address));

    listen(net_socket, 1);

    int client_socket = -1;

    while(client_socket == -1) {
        client_socket = accept(net_socket, NULL, NULL);
    }
    while(1) {
        send(client_socket, receive ,sizeof(receive), 0);
    }
    close(net_socket); 
    return 0;
}
