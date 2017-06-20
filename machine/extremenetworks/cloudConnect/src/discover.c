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
//	strcpy(server->hostname, "psimonea-pc2.corp.extremenetworks.com");
	strcpy(server->hostname, "asmola-pc.corp.extremenetworks.com");
	server->port = 8443;

	if (emcValidateServer(server) != TRUE) {
		server->valid = 0;
		return(FALSE);
	}
	server->valid = 0;
	return(TRUE);
}

int emcValidateServer(struct EMC_SERVER *server)
{
//	SSL *ssl_fd;
	
	// Make sure we can connect.
	server->connected = FALSE; // init
	server->ssl_fd = https_connect(server);
	if (server->ssl_fd == NULL) return(FALSE);
	server->connected = TRUE;
	
	// Make sure we get an HTTP 200 from a GET
	httpsGet(server, "/Clients/device/v1/discovery");
	if (server->response.responseCode == 200) return(TRUE);

	return(FALSE);
}


