/*
 *------------------------------------------------------------------------
 * header.c - Protocol APIs 
 *
 * July 2017, harshit kachhwaha
 *
 *------------------------------------------------------------------------
 */
#include "header.h"
#include <assert.h>
#include <string.h>

/**
 * make_request: Requst Packet Sent
 * @param 	packet_t 	*pkt 	- Packet to be filled
 *			uint16_t  	opcode 
 * 			const char 	*filename
 *			unsigned int block_count
 *			unsigned int size
 * @return  void
 */
void
make_request(packet_t *pkt, uint16_t opcode,
							const char *filename,
							unsigned int block_count,
							unsigned int size)
{
	pkt->opcode = opcode;
	memset(pkt->u.req.filename, 0, 25);
	
	memcpy(pkt->u.req.filename, filename, strlen(filename));
	pkt->u.req.bc = block_count;
	pkt->u.req.mode = 0;
	pkt->u.req.size = size;
}
