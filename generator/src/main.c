#include "libc.h"
#include "socket.h"
#include "modbus.h"

int main(void) {
        unsigned char buffer[] = {'M', 'O', 'D', 'B', 'U', 'S'};

        struct sockaddr_in remote;
        memset(&remote, 0, sizeof(remote));
        remote.sin_family = AF_INET;
        // inet_pton(AF_INET, "127.0.0.1", &remote.sin_addr);
        // remote.sin_port = htons(4444); // standardowy port to 502

        int sock = socket(AF_INET, SOCK_STREAM, 0);

        connect(sock, (void *) &remote, sizeof(remote));
        write(sock, buffer, sizeof(buffer));

        printf("wyslano");

        close(sock);
        return 0;
}
