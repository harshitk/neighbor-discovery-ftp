/*
 *------------------------------------------------------------------------
 * discovery.h - neighbour discovery protocol
 *
 * July 2017, harshit kachhwaha
 *
 *------------------------------------------------------------------------
 */

#ifndef __DISCOVERY_H__
#define __DISCOVERY_H__

#include <netinet/in.h>


void start_neighbour_discovery(int socket, uint8_t my_node_no);
void do_update_neighbour_table(uint8_t node_no, struct in_addr addr);
char *get_neighbour_ip(int node_no);
void destroy_neighbour_table();
#endif /* __DISCOVERY_H__ */