/*
 *------------------------------------------------------------------------
 * discovery.c - Peer Discovery Potocol and Neighbour Table defines  
 *
 * July 2017, harshit kachhwaha
 *
 *------------------------------------------------------------------------
 */
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/md5.h>

#include "header.h"
#include "common.h"
#include "discovery.h"


/**
 * neighbour ip table : will contains IP Address to Node number mapping.
 */
static uint8_t *n_table[TABLE_MAX];


/**
 * send_discovery_response: Send Discovery Request Packet to Multicast
 *							Group Joined by peer devices
 * @param	socket - socket fd			
 * @param	my_node_no 	- This device node ID, so that other neighbour 
 *						  can add this device in to thier IP neighbour Table
 * @return	void
 */
void 
start_neighbour_discovery(int socket, uint8_t my_node_no)
{
	struct sockaddr_in groupSock;
	packet_t	discPkt;
	int 		pktLen = 0;
	
	discPkt.opcode = DISC_REQ;
	discPkt.u.discovery.req.node_no = my_node_no;
	// FIXME Node Name
	memcpy(discPkt.u.discovery.req.node_name, "NODE#NN", strlen("NODE#NN"));
	
	/* Send Multicast packet */
	memset((char *) &groupSock, 0, sizeof(groupSock));
	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = inet_addr("226.1.1.1");
	groupSock.sin_port = htons(PEER_DEFAULT_PORT);
	
	
	/* Send a message to the multicast group specified by the*/
	/* groupSock sockaddr structure. */
	pktLen = (sizeof(discPkt.u.discovery) + OPCODE_DATA_SIZE);
	printf("Discover Packet len = [%d]\n",pktLen);
	if(sendto(socket, &discPkt, pktLen, 0, (struct sockaddr*)&groupSock, sizeof(groupSock)) < 0) {
		perror("Sending datagram message error");
	}
	else {
	  printf("Sending Discover message \t\t[\x1B[32mOK\x1B[37m]\n");
	}
}

/**
 * do_update_neighbour_table: will add new IP address and node no mapping
 *							  neighbour table
 * @param	uint8_t node_no			
 * @param	struct in_addr addr
 * @return	void
 */
void
do_update_neighbour_table(uint8_t node_no, struct in_addr addr) 
{
	// FIXME - Need to Add writer lock
	n_table[node_no] = (char *)malloc(SIZE_OF_IP_STR);
	memcpy(n_table[node_no],inet_ntoa(addr), SIZE_OF_IP_STR);
}

/**
 * do_update_neighbour_table: will add new IP address and node no mapping
 *							  neighbour table
 * @param	uint8_t node_no			
 * @return	char * : IP address mapped with input node no
 */
char
*get_neighbour_ip(int node_no)
{
	// FIXME - Need to Add reader lock
	return n_table[node_no];
}

void
destroy_neighbour_table()
{
	uint8_t i = 0;
	for (; i<TABLE_MAX; i++) {
		if (n_table[i] != NULL) {
			free(n_table[i]);
			n_table[i] = NULL;
		}
	}
}