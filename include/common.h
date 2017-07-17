/*
 *------------------------------------------------------------------------
 * common.h - common defines, debugs
 *
 * July 2017, harshit kachhwaha
 *
 *------------------------------------------------------------------------
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#define PEER_DEFAULT_PORT	7890
#define TABLE_MAX			100
#define RETRY   			3
#define SIZE_OF_IP_STR		15

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

/* Uncomment this to print debugs logs */
//#define PRINT

#ifdef PRINT
	#define DEBUG(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )
#else
	#define DEBUG(...)
#endif

#define INFO(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( 0 )


#endif /* __HEADER_H__ */