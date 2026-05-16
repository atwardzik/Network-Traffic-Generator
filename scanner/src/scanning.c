#include "scanning.h"

#include "libc.h"

#include <ifaddrs.h>
#include <net/if.h>

#define MODBUS_PORT 5059

struct ModbusConn {
        struct in_addr current_addr;
        int port;
        char info[20];
};

int scanning_menu(int argc, char *argv[]) {
        const char *output_file = DEFAULT_OUT_FILE;

        const char *ip_range_start = nullptr;
        const char *ip_range_end = nullptr;
        bool auto_scanning = false;

        int c;
        while ((c = getopt(argc, argv, "o:s:e:ah")) != -1) {
                switch (c) {
                        case 'o':
                                output_file = optarg;
                                if (!output_file) {
                                        printf("Error: After -o you must specify the path to the file.\n");
                                        return 1;
                                }
                                break;
                        case 's':
                                ip_range_start = optarg;
                                break;
                        case 'e':
                                ip_range_end = optarg;
                                break;
                        case 'a':
                                auto_scanning = true;
                                break;
                        case 'h':
                        default:
                                printf("=== Modbus Scanner ===\n");
                                printf("Usage: %s [OPTION] [-o path/to/file.bin]\n", argv[0]);
                                printf("Options:\n");
                                printf("  -a                    Automatically scans the local network\n");
                                printf("  -s <start> -e <end>   Scans a specific IP range\n");
                                printf("  -o <file>             (Optional) Output path (default: %s)\n",
                                       DEFAULT_OUT_FILE);
                                return 0;
                }
        }


        if (auto_scanning) {
                scan_auto_local(output_file);
        }
        else if (ip_range_start && ip_range_end) {
                scan_custom_range(ip_range_start, ip_range_end, output_file);
        }
        else {
                printf("Error: Specify starting and ending IP addresses for the range option.\n");
                return 1;
        }

        printf("\nScanning completed. Results saved in: %s\n", output_file);
        // display_saved_results(output_file);

        return 0;
}


int verify_is_modbus(int sock) {
        // ping message
        unsigned char modbus_query[] = {
                0x00, 0x01, // Transaction ID
                0x00, 0x00, // Protocol ID
                0x00, 0x06, // Length
                0x01,       // Unit ID
                0x03,       // Function Code (Read)
                0x00, 0x00, // Start Address
                0x00, 0x01  // Quantity
        };
        unsigned char response[12];

        if (write(sock, modbus_query, sizeof(modbus_query)) < 0) {
                return 0;
        }

        const ssize_t bytes_received = read(sock, response, sizeof(response));

        // Valid response starts with our Transaction ID (00 01)
        if (bytes_received >= 9 && response[0] == 0x00 && response[1] == 0x01) {
                if (response[7] == 0x03)
                        return 1; // Success!
                if (response[7] == 0x83)
                        return 2; // Modbus Exception - still modbus device
        }
        return 3;
}

int is_modbus_active(const char *ip) {
        struct sockaddr_in server;
        struct timeval tv;
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
                return 0;

        // set socket to non-blocking mode
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        server.sin_addr.s_addr = inet_addr(ip);
        server.sin_family = AF_INET;
        server.sin_port = htons(MODBUS_PORT);


        int res = connect(sock, (struct sockaddr *) &server, sizeof(server));

        if (res < 0) {
                if (errno == EINPROGRESS) {
                        // Connection is in progress, wait with select()
                        fd_set fdset;
                        FD_ZERO(&fdset);
                        FD_SET(sock, &fdset);

                        tv.tv_sec = 0;
                        tv.tv_usec = 500000;

                        // Wait for the socket to become writable (means connection finished)
                        res = select(sock + 1, NULL, &fdset, NULL, &tv);

                        if (res > 0) {
                                // Check if there was an actual error during connection
                                int so_error;
                                socklen_t len = sizeof(so_error);
                                getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
                                if (so_error == 0)
                                        res = 1; // Success
                                else
                                        res = 0; // Connection failed
                        }
                        else {
                                res = 0; // Timeout or select error
                        }
                }
                else {
                        res = 0; // Immediate error (e.g., Network unreachable)
                }
        }
        else {
                res = 1; // Connected immediately
        }

        // Verify and Close
        if (res == 1) {
                // Return to blocking mode for the actual data exchange
                fcntl(sock, F_SETFL, flags);

                // Apply timeouts for the Read/Write phase
                tv.tv_sec = 0;
                tv.tv_usec = 300000;
                setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv));

                int result = verify_is_modbus(sock);
                close(sock);
                return result;
        }

        close(sock);
        return 0;
}


void scan_auto_local(const char *filename) {
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
                if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET)
                        continue;

                // Skip loopback and inactive interfaces
                if (!(ifa->ifa_flags & IFF_LOOPBACK) && (ifa->ifa_flags & IFF_UP)) {
                        // casting
                        struct sockaddr_in *addr = (struct sockaddr_in *) ifa->ifa_addr;
                        struct sockaddr_in *mask = (struct sockaddr_in *) ifa->ifa_netmask;

                        // from network to host
                        uint32_t ip_val = ntohl(addr->sin_addr.s_addr);
                        uint32_t mask_val = ntohl(mask->sin_addr.s_addr);

                        // Calculate range
                        uint32_t start_ip = (ip_val & mask_val) + 1;
                        uint32_t end_ip = (ip_val | ~mask_val) - 1;

                        char start_str[INET_ADDRSTRLEN], end_str[INET_ADDRSTRLEN];
                        struct in_addr s, e;

                        // converting to Network way
                        s.s_addr = htonl(start_ip);
                        e.s_addr = htonl(end_ip);

                        // from binary to text form
                        inet_ntop(AF_INET, &s, start_str, INET_ADDRSTRLEN);
                        inet_ntop(AF_INET, &e, end_str, INET_ADDRSTRLEN);

                        printf("\n>>> Detected interface: %s (%s)\n", ifa->ifa_name, inet_ntoa(addr->sin_addr));
                        scan_custom_range(start_str, end_str, filename);
                }
        }
        freeifaddrs(ifaddr);
}


int scan_custom_range(const char *start_ip_str, const char *end_ip_str, const char *filename) {
        struct in_addr start_addr, end_addr;

        // Convert strings to binary format
        if (inet_aton(start_ip_str, &start_addr) == 0 || inet_aton(end_ip_str, &end_addr) == 0) {
                printf("Error: Invalid IP address format.\n");
                return 1;
        }

        // Convert to host byte order (integer) to allow math/looping
        uint32_t start = ntohl(start_addr.s_addr);
        uint32_t end = ntohl(end_addr.s_addr);

        printf("\n--- Starting Modbus verification:: %s - %s ---\n", start_ip_str, end_ip_str);


        int fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0) {
                printf("File open failed");
                return 1;
        }

        for (uint32_t i = start; i <= end; i++) {
                struct ModbusConn device = {0};
                device.current_addr.s_addr = htonl(i); // Corrects byte order for storage
                device.port = MODBUS_PORT;

                char *ip_to_check = inet_ntoa(device.current_addr);
                int status = is_modbus_active(ip_to_check);

                if (status >= 1 && status < 3) {
                        // Map status to your info string
                        if (status == 1)
                                strcpy(device.info, "modbus ok");
                        else if (status == 2)
                                strcpy(device.info, "ex");
                        else if (status == 3)
                                strcpy(device.info, "no");

                        printf("Found: %s | Status: %s\n", ip_to_check, device.info);

                        dprintf(fd, "%s:%i\n", ip_to_check, device.port);


                        // printf("Found and Saved: %s (%s)\n", ip_to_check, device.info);
                }
        }

        close(fd);
        return 0;
}

void display_saved_results(const char *filename) {
        int fd = open(filename, O_RDONLY);
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
