
// BASIC LIBRARIES
// standard input/output - I/O operations
// including file operations
#include <stdio.h>

// standard library
// e.g., dynamic memory management
#include <stdlib.h>

// string manipulation
#include <string.h>

// NETWORK COMMUNICATION
// operations on IP addresses - e.g., conversion to binary form
#include <arpa/inet.h>

// structures and functions for creating sockets
#include <sys/socket.h>

// COMMUNICATION WITH THE OS
// unix standard - POSIX system functions
#include <unistd.h>

// information about network interfaces
#include <ifaddrs.h>

// structure definitions regarding network interfaces
#include <net/if.h>


#include "scanning.h"



int main(int argc, char *argv[]) {
   
    printf("Program uruchomiony\n");
    
    scanning_menu(argc,  argv);
  
    
    return 0;
}
