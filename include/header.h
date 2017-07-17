/*
 *------------------------------------------------------------------------
 * header.h - Protocol defines 
 *
 * July 2017, harshit kachhwaha
 *
 *------------------------------------------------------------------------
 */

#ifndef __HEADER_H__
#define __HEADER_H__
#include <sys/types.h>
#include <stdlib.h>
#include <openssl/md5.h>

/*
 * Packet types. OPCODE
 */
#define DISC_REQ	01
#define DISC_RESP	02

#define	READ_RQ		03			    /* read request */
#define	WRITE_RQ	04				/* write request */
#define	DATA		05			    /* data packet */
#define	ACK			06			    /* acknowledgement */
#define	ERROR		07			    /* error code */


/*
 * SOME DEFINES
 */
#define FILE_NAME_SIZE		25
#define BLOCK_SIZE			512
#define ERR_MSG_SIZE		40
#define IP_LEN				15

#define OPCODE_DATA_SIZE	2		// Opcode data size -> 2bytes

/*
 * File Download Mode
 */
#define ASCII_MODE		0
#define BIN_MODE		1


/*
 * ERROR Code Idx
 */
#define ERR_NOT_DEF				0
#define ERR_ACCESS_VIO			1
#define ERR_DISK_FULL			2
#define ERR_UNKNOWN				3
#define ERR_FOPS_FAIL			4
#define ERR_CHKSUM_MISMATCH		5
#define ERR_BN_MISMATCH			6

typedef unsigned char   	uint8_t;
typedef short           	int16_t;
typedef unsigned short  	uint16_t;
typedef int             	int32_t;
typedef unsigned int    	uint32_t;
//typedef unsigned long long 	uint64t_t;

/***
typedef struct proto {
	uint16_t protocol;
	union {
		dicovery_t d_pkt;
		packet_t   f_pkt; 
	}u;
} prot_t;
****/
typedef struct packet {
	uint16_t opcode;
	union {
		union{
			struct {								// Discovery Request Multicast Packet
				int8_t node_no;
				int8_t node_name[FILE_NAME_SIZE];
			} req;
			struct {								// Discovery Response Packet
				int8_t node_no;
				int8_t node_name[FILE_NAME_SIZE];
			} resp;
		}discovery;
		struct {									// Request Packet
			int8_t filename[FILE_NAME_SIZE];
			int8_t mode;
			int32_t bc;
			int32_t size;
		} req;
		struct {									// Data Packet
			uint32_t block_no;
			int32_t size;
			int8_t buf[BLOCK_SIZE];
			uint8_t chksum[MD5_DIGEST_LENGTH];
		} data;
		struct {									// ACK Packet
			uint32_t block_no;
		} ack;
		struct {									// Error Packet
			uint16_t errcode;
			uint32_t block_no;
			int8_t errmsg[ERR_MSG_SIZE];
		} err;
	} u;
} packet_t;


void make_request(packet_t *pkt, uint16_t opcode,
							const char *filename,
							unsigned int block_count,
							unsigned int size);

#endif /* __HEADER_H__ */
