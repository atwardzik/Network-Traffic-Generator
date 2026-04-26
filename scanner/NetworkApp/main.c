
//PODSTAWOWE BIBLIOTEKI
//standard input/output - operacje wej/wyj
//w tym na plikach
#include <stdio.h>

//standard library
//np zarzadzanie pamiecia dynamiczna
#include <stdlib.h>

//manipulacja ciagami znakow
#include <string.h>

//KOMUNIKACJA SIECIOWA
//operacja na adresach ip - na postac binarna np.
#include <arpa/inet.h>

//struktury i funkcje do tworzenia gniazd
#include <sys/socket.h>

//KOMUNIKACJA Z SYS OP
//unix standard - funkcje systemowe POSIX
#include <unistd.h>

//informacje o interfejsach sieciowych
#include <ifaddrs.h>

//definicje struktur dotyczace intefejsow sieciowych
#include <net/if.h>


#include "scanning.h"



int main(int argc, char *argv[]) {
   
    printf("Program uruchomiony\n");
    
    //argc - ilosc argumentow, conajmniej 1
    //argv - tablica napisow
    
    for(int i =0; i < argc; i++)
    {
        printf("argument nr %d : %s \n", i, argv[i]);
    }
    
   // komunikacja_terminal();
    
    scanning_menu(argc,  argv);
    printf("Koniec\n");
    
    return 0;
}
