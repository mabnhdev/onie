/*
 * Copyright (c) Extreme Networks Inc. 2017
 * All rights reserved
 *
 * Module:
 *     discover
 *
 * Author:
 *     Steve Delahunty <sdelahunty@extremenetworks.com>
 *
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *     
 * Description:
 *     This module locates the extremecontrol server by using the following
 *     algorithm:
 *
 *     1) Get the current systems domain name and look for a server with
 *     the name "extremecontrol.{domainname}.
 *     2) Look for a system with the name "extremecontrol".
 *     3) Last Resort: 	psimonea-pc2.corp.extremenetworks.com
 *
 *     If an IP address is resolved, then a check is made to determine
 *     if the path "/Clients/device/v1/discovery" returns a HTTP 200 (success)
 *     from an "HTTP GET". If so, we assume an EMC is available and all
 *     further communications will be against the specified server.
 */
#include "onie_connector.h"

int emcDiscoverServer(struct EMC_SERVER *server) 
{
	/*
	 * This is a list of servers we will try to contact to
	 * find the EMC server. Start with the most generic and end
	 * with a developers or backup system.
	 */
	const char *serverList[] = { "extremecontrol.extremenetworks.com",
								 "extremecontrol",
//						   "psimonea-pc2.corp.extremenetworks.com",
								 "asmola-pc.corp.extremenetworks.com"
	};
	int serverCount = (int)(sizeof(serverList)/sizeof(char *));
	int i;

	for (i=0; i<serverCount; i++) {
		const char *name = serverList[i];
		debug_printf("    Trying to contact: %s...\n", name);
		strcpy(server->hostname, name);
		server->port = 8443;
		if (emcValidateServer(server) != TRUE) continue;
		server->valid = 1;
		return(TRUE);
	}
	server->valid = 0;
	return(FALSE);
}

int emcValidateServer(struct EMC_SERVER *server)
{
//	SSL *ssl_fd;
	
	// Make sure we can connect.
	server->connected = FALSE; // init
	server->ssl_fd = https_connect(server);
	if (server->ssl_fd == NULL) {
		debug_printf("      Unable to connect to server.\n");
		return(FALSE);
	}
	
	server->connected = TRUE;
	
	// Make sure we get an HTTP 200 from a GET
	httpsGet(server, "/Clients/device/v1/discovery");
	if (server->response.responseCode == 200) return(TRUE);
	debug_printf("      Invalid response code: %d\n",
				 server->response.responseCode);
	return(FALSE);
}


