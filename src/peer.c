/*
 *------------------------------------------------------------------------
 * Peer.c - Peer to Peer File Transfer
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
#include <sys/time.h>
#include <netinet/ip.h>

#include "header.h"
#include "common.h"
#include "peer.h"
#include "discovery.h"

/* Static declaration */
static struct sockaddr_in  	g_addr_me;
static struct sockaddr_in  	g_add_peer;
static struct sockaddr_in	g_peer_storage;
static int				g_peerSocket;
static socklen_t		g_peer_addr_size, g_peer_st_size;
static uint8_t			g_waiting_ack;
static int8_t			g_filename[FILE_NAME_SIZE];
static int8_t			g_node_no;
static uint32_t			g_file_size;
static uint32_t			g_last_block_no;
static uint32_t			g_block_count;
static int				g_udpSocket;
static MD5_CTX 			g_mdRcontext;
static MD5_CTX 			g_mdWcontext;

/* NON Static, access by other files */
FILE 					*gptr_writeFp;
uint8_t					g_peerConn;
uint8_t 				g_peer_ipaddr[IP_LEN];

/* API's */
static int recvfromTimeOut(int socket, long sec, long usec);
static int get_file_size(FILE *fp);
static void do_file_download_start(packet_t *pkt);
static void do_file_transfer(packet_t *pkt);
static int send_response_to_peer(packet_t *pkt);
static void send_discovery_response();
static int send_packet_to_peer(packet_t *pkt);
static int read_file(FILE *fp, unsigned char *payload, uint16_t size);

static int peer_start_file_tarnsfer(FILE *fp, const char *filename,
								   uint32_t block_count,
							       unsigned short last_block_data_size,
								   unsigned int size);

/* Error Code */
static char err_msg [7][ERR_MSG_SIZE] = {"Not defined, see error message if any",
                                  "Access Violation",
                                  "Disk full, or allocation exceeded",
                                  "Unknown transfer ID",
                                  "File Operation Failed",
                                  "Check Sum Mismatch!!",
                                  "Block Number Mismatch"};

/**
 * recvfromTimeOut:  Handles Socket Receive timeout.
 * @param	int socket		- Socket fd
			long sec		- Timeout in Sec
			long usec		- Timeout in usec
 * @return	int
 */
static int
recvfromTimeOut(int socket, long sec, long usec)
{
	// Setup timeval variable
	struct timeval timeout;
	timeout.tv_sec = sec;
	timeout.tv_usec = usec;
	// Setup fd_set structure
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(socket, &fds);
	// >0: data ready to be read
	return select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
}

/**
 * create_udp_socket:  It will create UDP socket, so that other peers can connect
 * @param	void
 * @return	void
 */
int
create_udp_socket(int port)
{
	socklen_t 		addr_size;
	int32_t 		ret;
	int8_t			loopch = 0;
	struct ip_mreq  group;
	struct in_addr	localInterface;

	/*Create UDP socket*/
	g_udpSocket = socket(PF_INET, SOCK_DGRAM, 0);

	/*Configure settings in address struct*/
	g_addr_me.sin_family = AF_INET;

	/* local host case */
	if (port != 0) {
		INFO("Creating Local Host\n");
		g_addr_me.sin_port = htons(port);
		g_addr_me.sin_addr.s_addr = inet_addr("127.0.0.1");
	} else {
		INFO("Creating On Interface\n");
		g_addr_me.sin_port = htons(PEER_DEFAULT_PORT);
		g_addr_me.sin_addr.s_addr = INADDR_ANY;//htonl(INADDR_ANY); //inet_addr("127.0.0.1");
	}
	memset(g_addr_me.sin_zero, '\0', sizeof g_addr_me.sin_zero);

	/*Initialize size variable to be used later on*/
	g_peer_st_size = sizeof g_peer_storage;
	ret = bind(g_udpSocket, (struct sockaddr *)
							&g_addr_me, sizeof(g_addr_me));
	/*Bind socket with address struct*/
	if (ret != 0) {
		printf("Socket Bind Error [errno:%d]",ret);
	}

	/* Join the multicast group 226.1.1.1 on the local
	 * interface. Note that this IP_ADD_MEMBERSHIP option must be
	 * called for each local interface over which the multicast
	 * datagrams are to be received.
	 */
	group.imr_multiaddr.s_addr = inet_addr("226.1.1.1");
	group.imr_interface.s_addr = INADDR_ANY;//inet_addr("192.168.1.104");
	if(setsockopt(g_udpSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0)
	{
		perror("Adding multicast group error");
		INFO("\nAdding multicast group \t\t\t[FAIL]\n");
		close(g_udpSocket);
	}
	else{
		INFO("\nAdding multicast group \t\t\t[\x1B[32mOK\x1B[37m]\n");
	}

	localInterface.s_addr = INADDR_ANY;//inet_addr("203.106.93.94");
	if(setsockopt(g_udpSocket, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0)
	{
	  perror("Setting local interface error");
	  INFO("Setting the local interface \t\t[FAIL]\n");
    }
	else {
	  INFO("Setting the local interface \t\t[\x1B[32mOK\x1B[37m]\n");
	}

	if(setsockopt(g_udpSocket, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch,
															sizeof(loopch)) < 0) {
		perror("Setting IP_MULTICAST_LOOP error");
		INFO("Disabling the loopback \t\t\t[FAIL]\n");
		close(g_udpSocket);
	}
	else {
		INFO("Disabling the loopback \t\t\t[\x1B[32mOK\x1B[37m]\n");
	}

	return ret;
}

/**
 * get_file_size:  It will return sixe of file in Bytes
 * @param	file pointer
 * @return	file size in bytes
 */
static int
get_file_size(FILE *fp){
    int prev;
	int size;

	prev = ftell(fp);
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    fseek(fp, prev, SEEK_SET); // Go back to where we were
    return size;
}

/**
 * do_file_download_start:  It will handle file transfer Request from Peer Device
 * @param	packet_t *pkt
 * @return	void
 */
static void
do_file_download_start(packet_t *pkt)
{
	packet_t	respPkt;

	memcpy(g_filename, pkt->u.req.filename, FILE_NAME_SIZE);
	g_file_size = pkt->u.req.size;
	g_block_count = pkt->u.req.bc;

	INFO("\nNew file request from Peer [%s]\n",g_filename);

	if (pkt->u.req.mode == ASCII_MODE) {
		gptr_writeFp = fopen(g_filename, "w");
		if (gptr_writeFp > 0 ) {
			respPkt.opcode = ACK;
			respPkt.u.ack.block_no = 0;
		}
		else {
			respPkt.opcode = ERROR;
			respPkt.u.err.errcode = ERR_FOPS_FAIL;
			respPkt.u.err.block_no = 0;
			memcpy(respPkt.u.err.errmsg, err_msg[ERR_FOPS_FAIL],ERR_MSG_SIZE);
		}
	}
	else {
		respPkt.opcode = ERROR;
		respPkt.u.err.errcode = ERR_UNKNOWN;
		respPkt.u.err.block_no = 0;
		memcpy(respPkt.u.err.errmsg, err_msg[ERR_UNKNOWN],ERR_MSG_SIZE);
	}

	send_response_to_peer(&respPkt);
}

/**
 * do_file_receive:  It will receive file in blocks, also calculate Checksum
 * @param	packet_t *pkt
 * @return	void
 */
static void
do_file_receive(packet_t *pkt)
{
	packet_t		respPkt;
	unsigned char 	md5_chkSum[MD5_DIGEST_LENGTH];

	/* Seq Block by block Transfer */
	if (++g_last_block_no == pkt->u.data.block_no) {
		int ret;
		DEBUG("SIZE[%d] in BLOCK[%u]\n",pkt->u.data.size, g_last_block_no);
		ret = fwrite(pkt->u.data.buf, 1, pkt->u.data.size, gptr_writeFp);

		/* Calculate MD5 Checksum for Received file */
		MD5_Update (&g_mdWcontext, pkt->u.data.buf, pkt->u.data.size);

		if (pkt->u.data.block_no == g_block_count) {
			/* reset last block count to zero for next req
			 * close write file, as this is the last block
			 * we received
			 */
			g_last_block_no = 0;
			fclose(gptr_writeFp);

			/* Since this is our last packet
			 * Finalize MD5 Checksum, and send this
			 * checksum to last packet.
			 */
			MD5_Final (md5_chkSum,&g_mdWcontext);

			//#ifdef PRINT
				printf("\nCheckSum Calculated :");
					for(int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", md5_chkSum[i]);
				printf("\nCheckSum Received   :");
					for(int i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", pkt->u.data.chksum[i]);
				printf("\n");
			//#endif
			if (strncmp(md5_chkSum, pkt->u.data.chksum, MD5_DIGEST_LENGTH) == 0) {
				INFO("\rBlock(s) Remaining:%s%08u%s/%u", KCYN,(g_block_count-pkt->u.data.block_no), KNRM, g_block_count);
				INFO("\nFile Download Success [%s]",g_filename);
				/* Send Ack packet (contains block number) to sender peer */
				respPkt.opcode = ACK;
				respPkt.u.ack.block_no = pkt->u.data.block_no;
			} else {
				printf("\nFile Download Failed [%s] Checksum Mismatch!!",g_filename);
				/* Send Checksum Error to Peer */
				respPkt.opcode = ERROR;
				respPkt.u.err.errcode = ERR_CHKSUM_MISMATCH;
				respPkt.u.err.block_no = pkt->u.data.block_no;
				memcpy(respPkt.u.err.errmsg, err_msg[ERR_CHKSUM_MISMATCH],ERR_MSG_SIZE);
			}
		} else {
			/* Send Ack packet (contains block number) to sender peer */
			respPkt.opcode = ACK;
			respPkt.u.ack.block_no = pkt->u.data.block_no;
			INFO("\rBlock(s) Remaining:%s%08u%s/%u", KCYN,(g_block_count-pkt->u.data.block_no), KNRM, g_block_count);
		}
	}
	else {
		respPkt.opcode = ERROR;
		respPkt.u.err.errcode = ERR_BN_MISMATCH; /* Block Number Mismatch */
		respPkt.u.err.block_no = pkt->u.data.block_no;
		memcpy(respPkt.u.err.errmsg, err_msg[ERR_BN_MISMATCH],ERR_MSG_SIZE);
	}

	send_response_to_peer(&respPkt);
}

/**
 * peer_recv_thread is used as the peer file recive from other peers
 * it accepts a void pointer
 */
void
peer_recv_thread( void *ptr )
{
	int 		nBytes;
	packet_t 	recvPkt;

	/* Waiting for Peer File Transfer Request*/
    while (1) {
		nBytes = recvfrom(g_udpSocket,&recvPkt,sizeof(recvPkt), 0,(struct sockaddr *)&g_peer_storage, &g_peer_st_size);
		if (nBytes != -1) {
			DEBUG("Recv Success[%d] %d\n", __LINE__, nBytes);
			switch (recvPkt.opcode) {
				case DISC_REQ:
					INFO("\nDISCOVERY REQUEST From Peer[%d]\n",recvPkt.u.discovery.req.node_no);
					/* Update Neighbour IP Table
					 * Send Discovery Response
					 */
					do_update_neighbour_table(recvPkt.u.discovery.req.node_no, g_peer_storage.sin_addr);
					INFO("Neighbour Added IP:[%s] To IP Table\n",get_neighbour_ip(recvPkt.u.discovery.req.node_no));

					send_discovery_response(g_peer_storage);

					break;

				case DISC_RESP:
					INFO(" DISCOVERY RESPONSE NOT Supported\n");
					// Update Neighbour IP Table
					do_update_neighbour_table(recvPkt.u.discovery.req.node_no, g_peer_storage.sin_addr);
					INFO("Neighbour Added IP:[%s] To IP Table\n",get_neighbour_ip(recvPkt.u.discovery.req.node_no));

					break;

				case WRITE_RQ:
					DEBUG(" Write REQ Recived\n");
					do_file_download_start(&recvPkt);
					break;

				case DATA:
					DEBUG(" DATA Packet Recived\n");
					do_file_receive(&recvPkt);
					break;

				default:
					DEBUG(" OPCODE Mismatch!!\n");
					break;
			}

		} else {
			printf(" Recv Failed %d\n",nBytes);
		}
	}
}

/**
 * connect_to_peer:
 * @param	IP address
 * @return	void
 */
void
connect_to_peer(char *ip,  unsigned short port)
{
	struct timeval tv;
	tv.tv_sec = 3;
	tv.tv_usec = 0;

	/*Create UDP socket*/
	g_peerSocket = socket(PF_INET, SOCK_DGRAM, 0);

	/*Configure settings in address struct*/
	g_add_peer.sin_family = AF_INET;

	if (ip != NULL) {
		g_add_peer.sin_port = htons(PEER_DEFAULT_PORT);
		g_add_peer.sin_addr.s_addr = inet_addr(ip);
	} else {
		g_add_peer.sin_port = htons(port);
		g_add_peer.sin_addr.s_addr = inet_addr("127.0.0.1");
	}

	g_add_peer.sin_port = htons(port);
	g_add_peer.sin_addr.s_addr = inet_addr("127.0.0.1");

	memset(g_add_peer.sin_zero, '\0', sizeof g_add_peer.sin_zero);

	/*Initialize size variable to be used later on*/
	g_peer_addr_size = sizeof g_add_peer;

	if (setsockopt(g_peerSocket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
		perror("Error");
	}

	g_peerConn	= 1;
}

/**
 * send_discovery_response: Send Discovery response to peer device
 * @param	packet_t *pkt 	Packet to be sent
 * @return	int  ret code
 */
static void
send_discovery_response(struct sockaddr_in neighbour_addr)
{
	/* Construct Discover Response Packet */
	packet_t	discPkt;
	int 		pktLen = 0;

	discPkt.opcode = DISC_RESP;
	discPkt.u.discovery.req.node_no = g_node_no;
	// FIXME Node Name
	memcpy(discPkt.u.discovery.req.node_name, "NODE#NN", strlen("NODE#NN"));

	//neighbour_addr.sin_family = AF_INET;
	//neighbour_addr.sin_port = htons(7891);

	/* Send a message to the Neighbour*/
	pktLen = (sizeof(discPkt.u.discovery) + OPCODE_DATA_SIZE);
	if(sendto(g_udpSocket, &discPkt, pktLen, 0, (struct sockaddr*)&neighbour_addr,
												sizeof(neighbour_addr)) < 0) {
		perror("Sending datagram message error");
	}
	else {
	  printf("Sending Discover message \t\t[\x1B[32mOK\x1B[37m]\n");
	}
}

/**
 * send_response_to_peer: Send response to peer device
 * @param	packet_t *pkt 	Packet to be sent
 * @return	int  ret code
 */
static int
send_response_to_peer(packet_t *pkt)
{
	int		 	ret;
	ret = sendto(g_udpSocket,pkt, sizeof(packet_t), 0,
					(struct sockaddr *)&g_peer_storage, g_peer_st_size);
	return ret;
}

/**
 * send_packet_to_peer: Send packet to peer, also retries in error condition
 * @param	packet_t *pkt 	Packet to be sent
 * @return	int  ret code
 */
static int
send_packet_to_peer(packet_t *pkt)
{
	int		 	ret;
	int 	 	nBytes;
	packet_t 	recvPkt;
	int			retry_count = 0;
	int			req_wait_count = 0;

retry:
	ret = sendto(g_peerSocket,pkt, sizeof(packet_t), 0,
							(struct sockaddr *)&g_add_peer, g_peer_addr_size);
	DEBUG("PACKET SENT Waiting for ACK [%d]!!\n",ret);

/**
 * wait_loop is only for WRITE_REQ, as in other peer user
 * is providing some input (i.e. Accept Req and File Name)
 */
wait_loop:
	nBytes = recvfrom(g_peerSocket,&recvPkt,sizeof(recvPkt),0,NULL, NULL);
	if (nBytes != -1) {
		DEBUG("Recv Success %d\n",nBytes);
		switch (recvPkt.opcode) {
				case ACK:
					DEBUG(" ACK Received %u %u\n", pkt->u.ack.block_no,
												   recvPkt.u.ack.block_no);
					/* Check Block No Block Number 0 for Transfer Request
					 * Else Compare with data block bumber
					 */
					if (recvPkt.u.ack.block_no == 0 ||
							pkt->u.data.block_no == recvPkt.u.ack.block_no)  {
						DEBUG("ACK Block Received matched\n");
						return ret;	/** Success case return no of bytes sent */
					}
					break;

				case ERROR:
					DEBUG(" ERROR Recived\n");
					if (pkt->u.ack.block_no == recvPkt.u.err.block_no)  {
						if (retry_count != RETRY) {
							retry_count++;
							goto retry;
						} else {
							printf(" ERROR Recived [%s]\n",recvPkt.u.err.errmsg);
						}
					}
					break;

				default:
					printf(" Response OPCODE Mismatch!!\n");
					break;
			}

	} else {
		if (pkt->opcode == WRITE_RQ)  {
			if (req_wait_count != 10) {
				req_wait_count++;
				goto wait_loop;
			}
		}
		printf(" Recv Failed %d\n",nBytes);

		return nBytes;
	}

	return -1;
}

/**
 * read_file: Read file from disk
 * @param
 *			FILE *fp				 	file pointer
 *			unsigned char *payload		read data copied to payload
 * @return	int  Number of bytes read
 */
static int
read_file(FILE *fp, unsigned char *payload, uint16_t size)
{
	int ret;
	ret = fread(payload, 1, size, fp);
	return ret;
}


/**
 * peer_start_file_tarnsfer: Start Block transfer
 * @param
 *			FILE fp				 				file pointer
 *			const char *filename 				name of file
 *			unsigned short last_block_data_size data size in last block
 * @return	int retCode
 */
static int
peer_start_file_tarnsfer(FILE *fp, const char *filename,
								   uint32_t block_count,
							       unsigned short last_block_data_size,
								   unsigned int size)
{
	int8_t 		 firsttrip = 1;
	packet_t 	 pkt;
	char 		 buf[BLOCK_SIZE];
	int 		 ret = 0;
	uint32_t	 block_no = 0;
	uint8_t		 retry_last_pkt = 0;
	uint8_t		 retCode = 0;
	unsigned char md5_chkSum[MD5_DIGEST_LENGTH];

	DEBUG("\nBLOCK COUNT:%u\n",block_count);
	printf("\n");
	do {
		if (firsttrip) {
			make_request(&pkt, WRITE_RQ, filename, block_count, size);
			DEBUG("\nOPCODE :%d",pkt.opcode);
			DEBUG("\nFILENAME :%s",pkt.u.req.filename);
			DEBUG("\nMODE:%d\n",pkt.u.req.mode);
			firsttrip = 0;
		}
		else if ((block_count - block_no) > 1) {
			memset(pkt.u.data.buf, 0, BLOCK_SIZE);
			ret = read_file(fp, pkt.u.data.buf, BLOCK_SIZE);

			pkt.opcode = DATA;
			pkt.u.data.block_no = ++block_no;
			pkt.u.data.size = BLOCK_SIZE;

			//printf("\nBUF :%s",pkt.u.data.buf);
			DEBUG("\nSIZE :%d<",pkt.u.data.size);
			DEBUG("\nBLOCK NO:%u\n",pkt.u.data.block_no);

			/* Calculate MD5 Checksum */
			MD5_Update (&g_mdRcontext, pkt.u.data.buf, BLOCK_SIZE);
		}
		else {
			memset(pkt.u.data.buf, 0, BLOCK_SIZE);
			ret = read_file(fp, pkt.u.data.buf, last_block_data_size);

			pkt.opcode = DATA;
			pkt.u.data.block_no = ++block_no;
			pkt.u.data.size = last_block_data_size;

			//DEBUG("\nBUF :%s<",pkt.u.data.buf);
			DEBUG("\nSIZE :%d<",pkt.u.data.size);
			DEBUG("\nBLOCK NO:%u\n",pkt.u.data.block_no);

			/* Calculate MD5 Checksum */
			MD5_Update (&g_mdRcontext, pkt.u.data.buf, last_block_data_size);

			/* Since this is our last packet
			 * Finalize MD5 Checksum, and send this
			 * checksum to last packet.
			 */
			memset(pkt.u.data.chksum, 0, MD5_DIGEST_LENGTH);
			MD5_Final (pkt.u.data.chksum,&g_mdRcontext);


		}
		if (send_packet_to_peer(&pkt) > 0) {
			INFO("\rBlock(s) Transferred:%s%u%s/%u", KCYN,block_no, KNRM, block_count);
			retry_last_pkt = 1;
		} else {
			printf("Send Failed (Retry Performed!) terminating File transfer \n");
			retCode = -1;
			break;
		}

	}while (block_no < block_count);

	#ifdef PRINT
		if (retCode == 0) {
			printf("\nCheckSum :");
			for(int i = 0; i < MD5_DIGEST_LENGTH; i++) {
				printf("%02x", pkt.u.data.chksum[i]);
			}
			printf("\n");
		}
	#endif
	return retCode;
}


/**
 * transfer_file_to_peer: Transfer file the peer block by block
 * @param
 *			FILE fp				 file pointer
 *			const char *filename name of file
 * @return	void
 */
void
transfer_file_to_peer (FILE *fp, const char *filename)
{
	unsigned int no_of_bytes = 0;
	uint32_t block_count = 0;
	unsigned short last_block_data_size = 0;	/* unsigned short, data < 512 */

	/* Get File Size */
	no_of_bytes = get_file_size(fp);

	/* Check if ZERO size FILE */
	if (no_of_bytes == 0) {
		printf("File Is Empty\n");
		return;
	}
	printf("Number of Byte(s) %d\n",no_of_bytes);

	/* Calculate Block Count (BLOCK_SIZE = 512 BYTES)*/
	block_count = (no_of_bytes / BLOCK_SIZE);

	/* Calculate Last Block Data size */
	if (no_of_bytes % BLOCK_SIZE) {
		last_block_data_size = (no_of_bytes % BLOCK_SIZE);
		block_count += 1;
	}

	/* BLOCKING CALL - NOW Start Peer File Transfer */
	peer_start_file_tarnsfer(fp, filename, block_count, last_block_data_size, no_of_bytes);

	/* Can Close Read File as transfer is Done */
	fclose(fp);
}

int
init_peer_network(uint8_t node_no, int port)
{
	int 	  ret;
	pthread_t recv_thread;  /* thread variables */

	g_node_no = node_no;

	/* Init MD5 Context for Checksum calculation*/
	MD5_Init (&g_mdRcontext); 		// For Transmit File
	MD5_Init (&g_mdWcontext);		// For Receive  File

	/* Creat UDP Socket */
	create_udp_socket(port);

	/* Start Neighbour Discovery */
	INFO("Starting Neighbour Discovery \t\t[\x1B[32mOK\x1B[37m]\n");
	start_neighbour_discovery(g_udpSocket, node_no);

	/* create threads for listner from other peers */
    ret = pthread_create(&recv_thread, NULL, (void *) &peer_recv_thread, NULL);
	if (ret) {
		printf("Peer Recive thread create failed");
		INFO("Starting Neighbour Discovery \t\t[FAIL]\n");
		return -1;
	}
	return 0;
}


/**
 * close_oper is to close socket and other oper
 * @param	void
 * @return	void
 */
void
close_oper(){
	/* 1. Clode socket */
	close(g_udpSocket);
	/* 2. Destroy Neighbour IP Table */
	destroy_neighbour_table();
}
