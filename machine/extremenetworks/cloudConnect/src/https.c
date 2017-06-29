/*
 * Copyright (c) Extreme Networks Inc. 2017
 * All rights reserved
 *
 * Module:
 *     https.c
 *
 * Author:
 *     Steve Delahunty <sdelahunty@extremenetworks.com>
 *
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *     
 * Description:
 *     This module implements the HTTPS protocol using sockets and openSSL
 *     functions. No other dependencies are required for communications
 *     with a server over HTTPS.
 *
 *  HTTP Headers Notes:
 *     This module builds some of the HTTP headers from constants defined
 *     in the file "http.h": (please update this doc if changed in header file)
 *         HTTP_PORT               "8443"
 *         HTTP_CONNECTION         "keep-alive"
 *         HTTP_ACCEPT_ENCODING    "gzip, deflate"
 *         HTTP_ACCEPT             "*\*"
 *         HTTP_USER_AGENT         "python-requests/2.17.3"
 *         EXTREME_DEMO_TAG        "1"
 *         HTTP_CONTENT_TYPE_JSON  "application/json"
 *         HTTP_AUTHORIZATION      "
 */
#include "onie_connector.h"

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0) ;
}


int socks_get_ipaddr(char *ipaddress, char *device)
{
	struct ifreq ifr;
	int s;
	int error=0;
    struct sockaddr_in *sa;
	
	s = socket(AF_INET, SOCK_DGRAM, 0);
	strcpy(ifr.ifr_name, device);
	*ipaddress=0;
	if (ioctl(s, SIOCGIFADDR, &ifr) < 0) {
		error=1;
	}
	else {
		sa =(struct sockaddr_in *)&ifr.ifr_addr;
		strcpy(ipaddress, inet_ntoa(sa->sin_addr));
		error=0;
	}
	close(s);
	return(error);
}


SSL_CTX* InitCTX(void)
{
	const SSL_METHOD *method;
	SSL_CTX *ctx;

	/* 'Force of habit' aka boilerplate methods. */
	OpenSSL_add_all_algorithms();
	// Load all cryptographic algos. On OpenSSL.org's documentation this method is actually depricated. Not sure why it's here.
	SSL_load_error_strings();
	// Optional, makes debuging easier when there's an error. This method simply loads a bunch of strings into ram saying what error number means what.

	/* Generate context */
	method = TLSv1_2_client_method();
	//Now once again, openssl.org says this is deprieiated and you should just use TLS_client_method() -- however I get undefined reference error tying to use it.
	ctx = SSL_CTX_new(method);
	//That's the global context structure which is created by a server or client once per program life-time and which holds mainly default values for the SSL structures which are later created for the connections.
	if ( ctx == NULL ) {
		ERR_print_errors_fp(stderr);
		abort();
	}
	return ctx;
}


SSL *https_connect(struct EMC_SERVER *server)
{
    struct sockaddr_in sa;
    struct hostent *result;
    int fd;
	SSL *ssl;
	SSL_CTX *ctx;
	
    // Initialize
    memset((struct sockaddr *) &sa, 0, sizeof(sa));

    // Get information about the host
    result = gethostbyname(server->hostname);
    if (result) {
        memcpy(&sa.sin_addr, result->h_addr_list[0], 
               result->h_length);
    } else {
        debug_printf("      ERROR: Unknown host: %s\n", server->hostname);
        return(NULL);
        
    }

    // Load up the sockaddr_in structure
    sa.sin_family = AF_INET;
    sa.sin_port = htons(server->port);

    // Get a socket
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return(NULL);

    // Make the connection
    if (connect(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        //printf("Can't connect to the remote server(%s:%d).\n",
		//	   hostname, port);
        close(fd);
        return(NULL);
    }

	// Make an SSL connection
	ctx = InitCTX();
	// Add certificates later.
    ssl = SSL_new(ctx);
	SSL_set_fd(ssl, fd);
	if (SSL_connect(ssl) <0) {
		printf("AGH! No connection!\n");
		exit(0);
	}
	debug_printf("    Connected to %s:%d with %s cipher..\n",
				 server->hostname, server->port,
				 SSL_get_cipher(ssl));
	
    return(ssl);
}

int httpsGet(struct EMC_SERVER *server, const char *path)
{
	return(httpsRequest("GET", server, path, NULL, 0));
}
int httpsPut(struct EMC_SERVER *server, const char *path,
			 char *payload, int payloadLen)
{
	return(httpsRequest("PUT", server, path, payload, payloadLen));
}

int httpsRequest(const char *method, struct EMC_SERVER *server,
				 const char *path, char *payload, int payloadLen)
{
	char tmp[8192];
	const char *setCookieKeyword = "Set-Cookie: ";
	char *ptr;

	// Clear out previous requests and responses
	memset(server->request.buffer,  0, sizeof(server->request.buffer));
	memset(server->response.buffer,  0, sizeof(server->response.buffer));

	// Make sure we init the response code to 0 (indicates no response)
	server->response.responseCode = 0; // init for no response
	debug_printf("HTTP %s %s\n", method, path);
	
	// if we're not connected, then connect.
	if (server->connected != TRUE) {
		server->ssl_fd = https_connect(server);
		if (server->ssl_fd == NULL) return(FALSE);
		server->connected = TRUE;
	}

	/*
	 * We are now connected. Send the request.
	 */
	sprintf(server->request.buffer, "%s %s HTTP/1.1\r\n", method, path);
	sprintf(tmp, "Host: %s:%d\r\n", server->hostname, server->port);
	strcat(server->request.buffer, tmp);
	strcat(server->request.buffer, "Connection: keep-alive\r\n");
	strcat(server->request.buffer, "Accept-Encoding: gzip, deflate\r\n");
	strcat(server->request.buffer, "Accept: */*\r\n");

	/*
	 * This is our special HTTP header that instructs the server not to
	 * authenticate beyond the basics.
	 */
	if (server->useExtremeDemoTag == 1) {
		strcat(server->request.buffer, "ExtremeDemoTag: 1\r\n");
	}
	
	
	strcat(server->request.buffer, "User-Agent: python-requests/2.17.3\r\n");

	if (payloadLen > 0) {
		strcat(server->request.buffer, "Content-Type: application/json\r\n");
		sprintf(tmp,"Content-Length: %d\r\n", payloadLen);
		strcat(server->request.buffer, tmp);
	}
	
	if (server->sendAuthentication == 1) {
		strcat(server->request.buffer, "Authorization: Basic dkI2NVNETnVpKzNmTjIvSnZoOVJlZz09OnZCNjVTRE51aSszZk4yL0p2aDlSZWc9PQ==\r\n");
	}
	
	/*
	 * Do we have a cookie?
	 */
	if (strlen(server->cookie) > 0) {
		sprintf(tmp, "Cookie: %s\r\n", server->cookie);
		strcat(server->request.buffer, tmp);
	}
	
	strcat(server->request.buffer, "\r\n");
	if (payloadLen > 0) {
		strcat(server->request.buffer, payload);
		strcat(server->request.buffer, "\r\n");
	}
	
//	debug_printf("%s\n", server->request.buffer);
		
	SSL_write(server->ssl_fd, server->request.buffer,
			  strlen(server->request.buffer));
//	debug_printf("Wrote %d bytes.\n", count);
		
	SSL_read(server->ssl_fd,
			 server->response.buffer, sizeof(server->response.buffer));
//	debug_printf("Read %d bytes.\n", count);

//	debug_printf("Discovery Response: %s\n", server->response.buffer);

	// Was a cookie returned?
	ptr = strstr(server->response.buffer, setCookieKeyword);
	if (ptr && strlen(server->cookie) == 0) {
		char *end = strchr(ptr, ';');
		if (end)*end=0;
		ptr += strlen(setCookieKeyword);
		strncpy(server->cookie, ptr, MAX_COOKIE_LEN);
		server->cookie[MAX_COOKIE_LEN] = 0;
	}

	/*
	 * Get the response code now. We do this by coping the first 100 bytes
	 * into a temporary buffer so that we can apply string tokenization.
	 * This will preserve the original response in 'buffer'.
	 */
	const char *delims = " \n\r";
	char tmp2[100];
	strncpy(tmp2, server->response.buffer, sizeof(tmp2)-1);
	tmp[sizeof(tmp2)-1] = 0; // Make sure to terminate.
	ptr = strtok(tmp2, delims);
	if (strcmp(ptr, "HTTP/1.1")) return(FALSE);
	server->response.responseCode= atoi(strtok(NULL, delims));
	ptr = strtok(NULL, delims);
	strncpy(server->response.responseText, ptr, MAX_RESPONSETEXT_LEN);
	server->response.responseText[MAX_RESPONSETEXT_LEN] = 0;
	debug_printf("    Full Response = %d %s\n",
				 server->response.responseCode,
				 server->response.responseText);

	/*
	 * We want to scan down through the response until we
	 * find the payload. This effectively discards all headers
	 * so we can find the encapsulated json.
	 */
	char *resp = server->response.buffer;
	while (1) {
		ptr = strstr(resp, "\r\n");
		if (ptr == NULL) break;
		*ptr=0;
//		debug_printf("Parse Line = (%s)\n", resp);
		if (strlen(resp) == 0) {
			resp = ptr + 2;
			strcpy(server->response.payload, resp);
			break;
		}
		resp = ptr + 2;
	}
	return(TRUE);
}

