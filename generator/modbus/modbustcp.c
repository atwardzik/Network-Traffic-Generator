// tu z bibliotekami jest brudno, wiem
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
//#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>


#define RESET   "\033[0m"
#define BOLD    "\033[1m"

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"

struct config_t {
    char *address;
    char *file_path;
    int port;
    int unit_id;

    int function;
    int reg;
    int count;
    int value;

    char *rawdata;

    int timeout;
    bool debug;
};

typedef struct {
    struct in_addr current_addr;
    int port;
    char info[20];
} ModbusConn;

void print_help_menu(void) {

    printf(BOLD CYAN "\n=== MODBUS TCP CLI TOOL ===\n" RESET);

    printf(BOLD GREEN "\nWhat is Modbus TCP?\n" RESET);
    printf("Modbus TCP is a communication protocol running over TCP/IP (port 502).\n");
    printf("It is widely used in industrial automation systems.\n");
    printf("This program acts as a " YELLOW "client (master)" RESET " sending requests to devices.\n");

    printf(BOLD GREEN "\nHow does it work?\n" RESET);
    printf("1. Connect to a device (IP + port)\n");
    printf("2. Send a Modbus request frame\n");
    printf("3. Receive response data or status\n");

    printf(BOLD GREEN "\nModbus TCP Frame Structure:\n" RESET);

    printf(BOLD MAGENTA "\n[ Modbus Application Protocol Header (MBAP) ]\n" RESET);
    printf(CYAN "Transaction ID " RESET "- 2 bytes (request identifier)\n");
    printf(CYAN "Protocol ID    " RESET "- 2 bytes (always 0)\n");
    printf(CYAN "Length         " RESET "- 2 bytes (remaining length)\n");
    printf(CYAN "Unit ID        " RESET "- 1 byte (slave ID)\n");

    printf(BOLD MAGENTA "\n[ Protocol Data Unit (PDU) ]\n" RESET);
    printf(CYAN "Function Code  " RESET "- 1 byte (e.g. 3 = read registers)\n");
    printf(CYAN "Data           " RESET "- depends on function (max 260B)\n");

    printf(BOLD GREEN "\nExample Modbus Functions:\n" RESET);
    printf("  " CYAN "1" RESET "  - Read Coils\n");
    printf("  " CYAN "2" RESET "  - Read Discrete Inputs\n");
    printf("  " CYAN "3" RESET "  - Read Holding Registers\n");
    printf("  " CYAN "4" RESET "  - Read Input Registers\n");
    printf("  " CYAN "5" RESET "  - Write Single Coil\n");
    printf("  " CYAN "6" RESET "  - Write Single Register\n");
    printf("  " CYAN "16" RESET " - Write Multiple Registers\n");

    printf(BOLD GREEN "\nRAW mode:\n" RESET);
    printf("You can send a custom raw frame as hex:\n");
    printf(YELLOW "--rawdata \"00 01 00 00 00 06 01 03 00 00 00 02\"\n" RESET);
    printf("The program will send these bytes exactly as provided.\n");

    printf(BOLD GREEN "\nUsage:\n" RESET);

    printf(BOLD "\nRead registers:\n" RESET);
    printf(GREEN "./modbustcp -a 192.168.0.10 -u 1 -f 3 -r 0 -c 10\n" RESET);

    printf(BOLD "\nWrite register:\n" RESET);
    printf(GREEN "./modbustcp -a 192.168.0.10 -u 1 -f 6 -r 0 -v 123\n" RESET);

    printf(BOLD "\nRAW request:\n" RESET);
    printf(GREEN "./modbustcp -a 192.168.0.10 --rawdata \"00 01 00 00 00 06 01 03 00 00 00 02\"\n" RESET);

    printf(BOLD GREEN "\nOptions:\n" RESET);
    printf("  -a, --address     Device IP address\n");
    printf("  -p, --port        Port (default 502)\n");
    printf("  -u, --unit-id     Unit ID\n");
    printf("  -f, --function    Modbus function code\n");
    printf("  -r, --register    Register address\n");
    printf("  -c, --count       Number of registers\n");
    printf("  -v, --value       Value (for write)\n");
    printf("  -w, --rawdata     Raw HEX frame\n");
    printf("  -t, --timeout     Timeout (ms)\n");
    printf("  -d, --debug       Debug mode\n");
    printf("  -h, --help        Show help\n");

    printf(BOLD GREEN "\nNOTES:\n" RESET);

    printf(BOLD CYAN "\n============================\n\n" RESET);
}

void print_debug(struct config_t *cfg){
    printf(BOLD GREEN "==== DEBUG INFO ====\n" RESET);
    printf(BOLD YELLOW "address: %s\n" RESET, cfg->address);
    printf(BOLD YELLOW "port: %d\n\n" RESET, cfg->port);
    printf(BOLD CYAN "unit_id: %d\n" RESET, cfg->unit_id);
    printf(BOLD CYAN "function: %d\n" RESET, cfg->function);
    printf(BOLD CYAN "reg: %d\n" RESET, cfg->reg);
    printf(BOLD CYAN "count: %d\n" RESET, cfg->count);
    printf(BOLD CYAN "value: %d\n\n" RESET, cfg->value);
    printf(BOLD MAGENTA "rawdata: %s\n\n" RESET, cfg->rawdata);
    printf(BOLD MAGENTA "timeout: %d\n" RESET, cfg->timeout);
    printf(BOLD MAGENTA"debug: %s\n", cfg->debug ? BOLD GREEN "true" RESET : BOLD RED "false" RESET);
}

int build_frame(struct config_t *cfg, uint8_t *frame){
    // kod ukradziony gdzieś z czeluści internetu
    int i = 0;

    // MBAP
    frame[i++] = 0; frame[i++] = 1;
    frame[i++] = 0; frame[i++] = 0;

    int len_pos = i;
    frame[i++] = 0; frame[i++] = 0;

    frame[i++] = (uint8_t)cfg->unit_id;

    // PDU
    frame[i++] = (uint8_t)cfg->function;

   
    if (cfg->function == 3 || cfg->function == 4) {
       
        frame[i++] = cfg->reg >> 8;
       
        frame[i++] = cfg->reg & 0xFF;

        frame[i++] = cfg->count >> 8;
        frame[i++] = cfg->count & 0xFF;
    }
    else if (cfg->function == 6) {
        frame[i++] = cfg->reg >> 8;
        frame[i++] = cfg->reg & 0xFF;

        frame[i++] = cfg->value >> 8;
        frame[i++] = cfg->value & 0xFF;
    }

    // length = reszta po MBAP (UnitID + PDU)
    uint16_t len = i - 6;

    frame[len_pos] = len >> 8;
    frame[len_pos + 1] = len & 0xFF;

    return i;
}

int parse_hex(const char *str, uint8_t *out){
    // kod ukradziony gdzieś z czeluści internetu
    int count = 0;

    while (*str) {

        // pomiń spacje
        while (*str == ' ') str++;

        //czy zero na koncu napisu
        if (!*str) break;

        unsigned int byte;
        int n;

        // czytaj 1 lub 2 znaki hex
        if (sscanf(str, "%2x%n", &byte, &n) != 1)
            return -1;

        out[count++] = (uint8_t)byte;
        str += n;
    }

    return count;
}


void perform_send(const char *ip, struct config_t *cfg, uint8_t *buffer, size_t len) {

int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        if(cfg->debug) perror(BOLD RED "?? socket error ??" RESET);
        return;
    }

    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_port = htons(cfg->port);

    if (inet_pton(AF_INET, ip, &remote.sin_addr) != 1) {
        if(cfg->debug) fprintf(stderr, BOLD RED "?? invalid address: %s ??\n" RESET, ip);
        close(sock);
        return;
    }

    // Attempt to connect
    if (connect(sock, (struct sockaddr*)&remote, sizeof(remote)) < 0) {
        if(cfg->debug) printf(RED "Failed to connect to %s\n" RESET, ip);
        close(sock);
        return;
    }

    // Send the data provided in the buffer
    ssize_t n = write(sock, buffer, len);
    if (n > 0) {
        printf(BOLD GREEN "Sent to %s (%zd bytes)\n" RESET, ip, n);
    } else {
        if(cfg->debug) printf(RED "Write error on %s\n" RESET, ip);
    }

    close(sock);
}


int main(int argc, char *argv[])
{
    if(argc == 1){
        printf(BOLD GREEN "Try -h or --help\n");
        return 0;
    }

    struct config_t cfg = {0};
    cfg.address = "127.0.0.1";
    cfg.port = 502;
    cfg.unit_id = 1;
    cfg.timeout = 1000;
    cfg.debug = false;

    int c;
    int option_index = 0;

    static struct option long_options[] = {
        {"address", required_argument, 0, 'a'},
        {"file",    required_argument, 0, 'F'},
        {"port", required_argument, 0, 'p'},
        {"unit-id", required_argument, 0, 'u'},

        {"function", required_argument, 0, 'f'},
        {"register", required_argument, 0, 'r'},
        {"count", required_argument, 0, 'c'},
        {"value", required_argument, 0, 'v'},

        {"rawdata", required_argument, 0, 'w'},

        {"timeout", required_argument, 0, 't'},
        {"debug", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "ha:F:p:u:f:r:c:v:w:t:d:", long_options, &option_index)) != -1) {
        switch(c){
            case 'a': cfg.address = optarg; break;
            case 'F': cfg.file_path = optarg; break;
            case 'p': cfg.port = atoi(optarg); break;
            case 'u': cfg.unit_id = atoi(optarg); break;

            case 'f': cfg.function = atoi(optarg); break;
            case 'r': cfg.reg = atoi(optarg); break;
            case 'c': cfg.count = atoi(optarg); break;
            case 'v': cfg.value = atoi(optarg); break;

            case 'w': cfg.rawdata = optarg; break;
            case 't': cfg.timeout = atoi(optarg); break;
            case 'd': cfg.debug = true; break;
            case 'h': print_help_menu(); return 0;
            case '?':
                perror(BOLD RED "?? Unknown option ??\n" RESET);
                exit(1);

            default:
                exit(1);
        }
    }


    uint8_t frame[260];
    uint8_t rawbuf[260];
    uint8_t *buffer;
    size_t len;

    if (cfg.rawdata != NULL) {
        int parsed_len = parse_hex(cfg.rawdata, rawbuf);
        if (parsed_len < 0) {
            fprintf(stderr, BOLD RED "?? Invalid hex format ??\n" RESET);
            return 1;
        }
        len = (size_t)parsed_len;
        buffer = rawbuf;
    } else {
        len = (size_t)build_frame(&cfg, frame);
        buffer = frame;
    }

    // --- 2. EXECUTE BASED ON CHOICE ---
    if (cfg.file_path != NULL) {
        // Read Binary File
        FILE *fp = fopen(cfg.file_path, "rb"); 
        if (!fp) {
            perror("Error opening binary file");
            return 1;
        }

        ModbusConn temp;
        // Read one struct 
        while (fread(&temp, sizeof(ModbusConn), 1, fp) == 1) {
            // Convert binary address back to string for perform_send
            char *ip_str = inet_ntoa(temp.current_addr);
            perform_send(ip_str, &cfg, buffer, len);
        }
        fclose(fp);
    } 
    else if (cfg.address != NULL) {
        // CHOICE 2: Single IP from Terminal
        perform_send(cfg.address, &cfg, buffer, len);
    } 
    else {
        printf(YELLOW "Error: Provide an address (-a) or a scanner results file (-F).\n" RESET);
    }
   
    return 0;
}
