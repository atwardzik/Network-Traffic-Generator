#include "libc.h"
#include "scanning.h"



int main(int argc, char *argv[]) {
        const char *output_file = DEFAULT_OUT_FILE;

        const char *ip_range_start = nullptr;
        const char *ip_range_end = nullptr;
        bool auto_scanning = false;

        int port = 502;
        int c;
        while ((c = getopt(argc, argv, "o:s:e:ap:h")) != -1) {
                switch (c) {
                        case 'p':
                                port = manual_atoi(optarg);
                                if (port <= 0 || port > 65535) {
                                        printf("Błąd: Nieprawidłowy numer portu.\n");
                                        return 1;
                                }
                                break;
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
                                printf("  -p <port>             Port do skanowania (domyślnie: 502)\n");
                                printf("  -a                    Automatically scans the local network\n");
                                printf("  -s <start> -e <end>   Scans a specific IP range\n");
                                printf("  -o <file>             (Optional) Output path (default: %s)\n",
                                       DEFAULT_OUT_FILE);
                                return 0;
                }
        }


        if (auto_scanning) {
                scan_auto_local(output_file, port);
        }
        else if (ip_range_start && ip_range_end) {
                scan_custom_range(ip_range_start, ip_range_end, output_file, port);
        }
        else {
                printf("Error: Specify starting and ending IP addresses for the range option.\n");
                return 1;
        }

        printf("\nScanning completed. Results saved in: %s\n", output_file);
        // display_saved_results(output_file);

        return 0;
}
