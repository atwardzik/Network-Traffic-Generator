//
//  scanning.h
//  NetworkApp
//

//

#ifndef SCANNING_H
#define SCANNING_H

#define CONFIG_FILE "ports.cfg"
#define MAX_PORTS 10



int scanning_menu(int argc, char *argv[]);

int verify_is_modbus(int sock);

int is_modbus_active(const char *ip);

void scan_auto_local(void);

void scan_custom_range(const char *start_ip_str, const char *end_ip_str);

#endif
