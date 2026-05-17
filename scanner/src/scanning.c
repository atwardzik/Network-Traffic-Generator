#include "scanning.h"

#include "libc.h"

#include <ifaddrs.h>
#include <net/if.h>

#define MODBUS_PORT 502

struct ModbusConn {
        struct in_addr current_addr;
        int port;
        char info[20];
};

enum ModbusConnectionResponse {
        MODBUS_OK,
        MODBUS_ERR,
        NOT_A_MODBUS,

        HOST_SOCKET_ERROR,
        HOST_SELECT_ERROR,
        HOST_TIMEOUT_ERROR,
        HOST_IMMEDIATE_ERROR,
};


static enum ModbusConnectionResponse verify_is_modbus(const int sock) {
        const unsigned char modbus_ping_query[] = {
                0x00, 0x01, // Transaction ID
                0x00, 0x00, // Protocol ID
                0x00, 0x06, // Length
                0x01,       // Unit ID
                0x03,       // Function Code (Read)
                0x00, 0x00, // Start Address
                0x00, 0x01  // Quantity
        };
        unsigned char response[12];

        if (write(sock, modbus_ping_query, sizeof(modbus_ping_query)) < 0) {
                return 0;
        }

        const ssize_t bytes_received = read(sock, response, sizeof(response));

        // Valid response starts with our Transaction ID (00 01)
        if (bytes_received >= 9 && response[0] == 0x00 && response[1] == 0x01) {
                if (response[7] == 0x03) {
                        return MODBUS_OK;
                }
                if (response[7] == 0x83) {
                        return MODBUS_ERR;
                }
        }

        return NOT_A_MODBUS;
}

static enum ModbusConnectionResponse is_modbus_active(const char *ip, int port) {
        enum ModbusConnectionResponse result = NOT_A_MODBUS;

        struct sockaddr_in server;
        struct timeval tv;
        const int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
                return HOST_SOCKET_ERROR;
        }

        // set socket to non-blocking mode
        const int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        server.sin_addr.s_addr = inet_addr(ip);
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        bool connected = false;
        int res;
        if ((res = connect(sock, (struct sockaddr *) &server, sizeof(server))) == 0) {
                connected = true;
        }
        else {
                if (errno != EINPROGRESS) {
                        result = HOST_IMMEDIATE_ERROR;
                        goto sock_close_and_exit;
                }

                // Connection is in progress, wait with select()
                fd_set fdset;
                FD_ZERO(&fdset);
                FD_SET(sock, &fdset);

                tv.tv_sec = 0;
                tv.tv_usec = 500000;

                // Wait for the socket to become writable (means connection finished)
                res = select(sock + 1, nullptr, &fdset, nullptr, &tv);
                if (res < 0) {
                        result = HOST_SELECT_ERROR;
                        goto sock_close_and_exit;
                }
                if (res == 0) {
                        result = HOST_TIMEOUT_ERROR;
                        goto sock_close_and_exit;
                }

                int so_error;
                socklen_t len = sizeof(so_error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
                if (!so_error) {
                        connected = true;
                }
        }

        if (connected) {
                // Return to blocking mode for the actual data exchange
                fcntl(sock, F_SETFL, flags);

                tv.tv_sec = 0;
                tv.tv_usec = 300000;
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv));

                result = verify_is_modbus(sock);
        }

sock_close_and_exit:
        close(sock);

        return result;
}


void scan_auto_local(const char *filename, int port) {
        struct ifaddrs *ifaddr, *ifa;

        int clean_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        close(clean_fd);

        // there are any intefaces
        if (getifaddrs(&ifaddr) == -1) {
                dprintf(2, "getifaddrs");
                return;
        }

        // Iterate through interfaces
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) {
                        continue;
                }

                // Skip loopback and inactive interfaces
                if (!(ifa->ifa_flags & IFF_LOOPBACK) && (ifa->ifa_flags & IFF_UP)) {
                        struct sockaddr_in *addr = (struct sockaddr_in *) ifa->ifa_addr;
                        struct sockaddr_in *mask = (struct sockaddr_in *) ifa->ifa_netmask;

                        const uint32_t ip_val = ntohl(addr->sin_addr.s_addr);
                        const uint32_t mask_val = ntohl(mask->sin_addr.s_addr);

                        uint32_t start_ip = (ip_val & mask_val) + 1;
                        uint32_t end_ip = (ip_val | ~mask_val) - 1;

                        char start_str[INET_ADDRSTRLEN], end_str[INET_ADDRSTRLEN];
                        struct in_addr s, e;

                        s.s_addr = htonl(start_ip);
                        e.s_addr = htonl(end_ip);

                        inet_ntop(AF_INET, &s, start_str, INET_ADDRSTRLEN);
                        inet_ntop(AF_INET, &e, end_str, INET_ADDRSTRLEN);

                        printf("\n>>> Detected interface: %s (%s)\n", ifa->ifa_name, inet_ntoa(addr->sin_addr));
                        scan_custom_range(start_str, end_str, filename, port);
                }
        }
        freeifaddrs(ifaddr);
}


int scan_custom_range(const char *start_ip_str, const char *end_ip_str, const char *filename,int port) {
        struct in_addr start_addr, end_addr;

        if (inet_aton(start_ip_str, &start_addr) == 0 || inet_aton(end_ip_str, &end_addr) == 0) {
                printf("Error: Invalid IP address format.\n");
                return 1;
        }

        const uint32_t start = ntohl(start_addr.s_addr);
        const uint32_t end = ntohl(end_addr.s_addr);

        printf("\n--- Starting Modbus verification:: %s - %s ---\n", start_ip_str, end_ip_str);


        const int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
                printf("File open failed");
                return 1;
        }

        for (uint32_t i = start; i <= end; i++) {
                struct ModbusConn device = {0};
                device.current_addr.s_addr = htonl(i);
                device.port = port;

                char *ip_to_check = inet_ntoa(device.current_addr);
                const enum ModbusConnectionResponse status = is_modbus_active(ip_to_check, port);

                if (status == MODBUS_OK) {
                        strcpy(device.info, "modbus ok");
                }
                else if (status == MODBUS_ERR) {
                        strcpy(device.info, "ex");
                }
                else {
                        strcpy(device.info, "no");
                        continue;
                }

                printf("Found: %s | Status: %s\n", ip_to_check, device.info);

                dprintf(fd, "%s:%i\n", ip_to_check, device.port);
        }

        close(fd);
        return 0;
}

int manual_atoi(const char *str) {
    int res = 0;
    for (int i = 0; str[i] != '\0'; ++i) {
        // Sprawdź, czy znak jest cyfrą
        if (str[i] < '0' || str[i] > '9') {
            break;
        }
        res = res * 10 + (str[i] - '0');
    }
    return res;
}

void display_saved_results(const char *filename) {
        const int fd = open(filename, O_RDONLY);
        if (fd < 0) {
                printf("No saved data found in %s.\n", filename);
                return;
        }

        printf("\n--- Saved Modbus Devices ---\n");
        printf("IP Address | Port | Status\n");
        printf("------------------------------------------\n");

        char ch;

        while (read(fd, &ch, 1) > 0) {
                // binary to string
                // inet_ntop(AF_INET, &temp.current_addr, ip_str, INET_ADDRSTRLEN);

                // printf("%s | %i | %s\n", ip_str, temp.port, temp.info);

                write(1, &ch, 1);
        }

        close(fd);
}
