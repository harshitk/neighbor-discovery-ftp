/*
 *------------------------------------------------------------------------
 * peer.h - Protocol defines 
 *
 * July 2017, harshit kachhwaha
 *
 *------------------------------------------------------------------------
 */

#ifndef __PEER_H__
#define __PEER_H__


int init_peer_network(uint8_t node_no, int port);
void connect_to_peer(char *ip, uint16_t port);
void close_oper();
void transfer_file_to_peer (FILE *fp, const char *filename);

#endif /* __PEER_H__ */