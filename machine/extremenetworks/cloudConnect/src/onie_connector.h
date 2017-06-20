/*
 * Copyright (c) Extreme Networks Inc. 2017
 * All rights reserved
 *
 * Module:
 *     onie_connector.h
 *
 * Author:
 *     Steve Delahunty <sdelahunty@extremenetworks.com>
 *
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *     
 * Description:
 *     This header file contains all the constants, macros, etc needed
 *     to implement the onie_connector.
 */
#ifndef __ONIE_CONNECTOR__H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/icmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <getopt.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <regex.h>
#include <ifaddrs.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include "jsmn.h"


// Maxs/mins
#define MAX_HOSTNAME_LEN               (64)
#define MAX_DOMAINNAME_LEN             (64)
#define MAX_FQHN_LEN                   ((MAX_HOSTNAME_LEN+MAX_DOMAINNAME_LEN)+16)
#define MAX_URIPATH_LEN                (1024)
#define MAX_RESPONSE_LEN               (4096*8)
#define MAX_REQUEST_LEN                (8192)
#define MAX_COOKIE_LEN                 (1024)
#define MAX_RESPONSETEXT_LEN           (128)
#define MAX_JSON_VALUE_LEN             (128)
#define MAX_COMMAND_LEN                (256)

// Other contants
#define TRUE                           (1)
#define FALSE                          (0)
	

// Main structure containing the location of the discovered EMC server
struct EMC_SERVER {
	char hostname[MAX_FQHN_LEN+1];
	int port;
	char valid;
	SSL *ssl_fd;
	int connected;
	char cookie[MAX_COOKIE_LEN+1];
	int useExtremeDemoTag;
	int sendAuthentication;
	
	struct HTTP_REQUEST {
		char path[MAX_URIPATH_LEN+1];
		char buffer[MAX_REQUEST_LEN+1];
	} request;
		
	struct HTTP_RESPONSE {
		char buffer[MAX_RESPONSE_LEN+1];
		char payload[MAX_RESPONSE_LEN+1];
		int responseCode;
		char responseText[MAX_RESPONSETEXT_LEN+1];
		
	} response;
};

struct ImageUpgradeResponse {
	char upgrade[MAX_JSON_VALUE_LEN+1];
	char uri[MAX_JSON_VALUE_LEN+1];
	char protocol[MAX_JSON_VALUE_LEN+1];
	char username[MAX_JSON_VALUE_LEN+1];
	char password[MAX_JSON_VALUE_LEN+1];
	char ipaddress[MAX_JSON_VALUE_LEN+1];
	int port;
	char path[MAX_JSON_VALUE_LEN+1];
	
};

	
#include "http.h"

// Functions defined in discover.c
extern int emcDiscoverServer(struct EMC_SERVER *server);
extern int emcValidateServer(struct EMC_SERVER *server);

// Functions defined in onie_main.c
extern int debug_printf(char *format, ...);

// Functions defined in https.c
extern SSL *https_connect(struct EMC_SERVER *server);
extern int httpsGet(struct EMC_SERVER *server, char *path);
extern int httpsPut(struct EMC_SERVER *server, char *path,
					char *payload, int payloadLen);
extern int httpsRequest(char *method, struct EMC_SERVER *server,
						char *path, char *payload, int payloadLen);

#endif // __ONIE_CONNECTOR_H__
