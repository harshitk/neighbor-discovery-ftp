/*
 *------------------------------------------------------------------------
 * main.c - Main CLI Interface, starts all network module and discovery
 *			protocol
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
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/md5.h>

#include "header.h"
#include "common.h"
#include "peer.h"
#include "discovery.h"


/* Extern variable */
extern uint8_t 		g_peer_ipaddr[IP_LEN];
extern uint8_t		g_peerConn;

FILE	*g_readfp;

/**
 * INThandler:  Handles ctrl + C Interrupt and safe exit.
 * @param	int sig		- Socket fd
 * @return	void
 */
void  INThandler(int sig)
{
    char  c;
    signal(sig, SIG_IGN);
    INFO("OUCH, You hit Ctrl-C?\n");
	close_oper();
	exit(0);
}

int
main(){

	char      file_name[25] = "sample.text";
	uint16_t  port = 0;
	int8_t    ch;
	uint8_t   peer_ip[15];
	uint8_t   localhost = 0;
	int32_t   node_no = 0;
	int c;

	/* Register SIGINT for ctrl+c for Safe Exit */
	signal(SIGINT, INThandler);

	printf("\nEnter Peer Number[0-255]: ");
	fflush(stdin);
	scanf(" %d", &node_no);

	printf("\nDo You want to test With Local Host [y/n] :");
	fflush(stdin);
	scanf(" %c", &ch);
	if (ch == 'y' || ch == 'Y') {

		printf("Enter Port Number [For This PEER]:");
		fflush(stdin);
		scanf(" %hu", &port);
		localhost = 1;
	}
	else {
		printf("Peer File Transfer Starting on default port (%d)\n", PEER_DEFAULT_PORT);
	}

	/* Init Peer Network */
	init_peer_network(node_no, port);

	while(1){
		int a = 0;
		while((c = getchar()) != '\n' && c != EOF);
		printf("\n\n");
		printf("-----------------------------------------\n");
		printf("| 1. Request for download File from Peer |\n");
		printf("| 2. Transfer File                       |\n");
		printf("| 3. Connect/Discover Peer in Network    |\n");
		printf("-----------------------------------------\n");
		printf("| Enter: ");
		scanf(" %d", &a);

		if (a == 2) {
			printf("\nEnter File Name (25 letters): ");
			while((c = getchar()) != '\n' && c != EOF);
			scanf("%s", file_name);

			g_readfp = fopen(file_name, "r");
			if (g_readfp != NULL) {

				if (g_peerConn) {
					transfer_file_to_peer(g_readfp, file_name);
				} else {
					printf("Peer Info Not Found\n");
				}
			} else {
				INFO("File Open Failed \n");
			}

		} else if ( a == 3 ) {
			if (!localhost) {
				printf("| 1. Enter IP Manually       |\n");
				printf("| 2. Show Discovered IP list |\n");
				printf("     Enter: ");
				fflush(stdin);
				scanf(" %d", &a);
				fflush(stdin);
				if (a==1) {
					printf("Enter Peer IP : ");
					while((c = getchar()) != '\n' && c != EOF);
					scanf(" %s", g_peer_ipaddr);
					connect_to_peer(g_peer_ipaddr, 0);
				} else if (a==2) {
					int loop = 0;
					char *ip = NULL;
					printf("\n------------------------------\n");
					printf("|NODE No |    IP ADDRESS     |\n");
					for (;loop < TABLE_MAX ; loop++) {
						ip = get_neighbour_ip(loop);
						if (ip) {
							printf("|   %d    |  %15s  |\n", loop, ip);
						}
					}
					printf("------------------------------\n");
					printf("Set Peer Indo (Enter Node ID)");
					fflush(stdin);
					scanf(" %d", &a);
					ip = get_neighbour_ip(a);
					connect_to_peer(ip, 0);
				}
			}
			else {
				unsigned short peer_port = 0;
				printf("Enter Peer Port Number : ");
				while((c = getchar()) != '\n' && c != EOF);
				scanf(" %hu", &peer_port);
				connect_to_peer(NULL, peer_port);
			}

		}else {
			while((c = getchar()) != '\n' && c != EOF);
			getchar();
			/* Safe Exit */
			close_oper();
			break;
		}
	}
	return 0;
}
