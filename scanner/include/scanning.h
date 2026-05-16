//
//  scanning.h
//  NetworkApp
//

#ifndef SCANNING_H
#define SCANNING_H

#define DEFAULT_OUT_FILE "modbus_results"
#define MAX_PORTS 10


void scan_auto_local(const char *filename);

int scan_custom_range(const char *start_ip_str, const char *end_ip_str, const char *filename);

void display_saved_results(const char *filename);

#endif
