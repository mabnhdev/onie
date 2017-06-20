/*
 * Copyright (c) Extreme Networks Inc. 2017
 * All rights reserved
 *
 * Module:
 *     onie_connector
 *
 * Author:
 *     Steve Delahunty <sdelahunty@extremenetworks.com>
 *
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *     
 * Description:
 *     This module implements the bare-minimum cloudConnector functionality
 *     required to authenticate and retrieve the URI of an image to install
 *     on the current system. The idea is to provide a stripped down and
 *     easily portable application that can work with ONIE. We limit the
 *     number of external libraries required by implementing as much as we
 *     can within this codebase. The only external libraries which we need
 *     are openSSL (the connection to EMC is via SSL) and UcLibc (or some
 *     "C" library). 
 *
 *  Authentication Notes:
 *     The normal authentication mechansim for communicating with EMC is via
 *     an HTTPS connection with the username/password encrypted with Base64,
 *     DES3 and some type of obfuscation. To save development time, we
 *     short-circuit this by passing up a well known, yet still encrypted
 *     username/password and using a special HTTP header called
 *     "ExtremeDemoTag" that will tell the server not to use the standard
 *     decryption mechansims and only accept the original username/password.
 *     
 * These are the new sysObjectID's for ONIE-based devices
 * (Received from Charles McTague [mailto:cmctague@extremenetworks.com]
 * 
 *  summitX870x32CxONIE    287    Summit X870-32C: 32 QSFP+ (100Gb)
 *                                Open Network Install Environment (ONIE)
 *
 *  summitX870x32CxSLX     288    Summit X870-32C: 32 QSFP+ (100Gb)
 *                                SLX Software
 *
 *  summitX870x96Xx8CxONIE 289    Summit X870-96X-8C: 24 QSFP+ (4x10Gb),
 *                                8 QSFP+ (100Gb)
 *                                Open Network Install Environment (ONIE)
 *
 *  summitX870x96Xx8CxSLX  290    Summit X870-96X-8C: 24 QSFP+ (4x10Gb),
 *                                8 QSFP+ (100Gb)
 *                                SLX Software
 */


#include "onie_connector.h"

// Forwards
SSL *socks_connect_ssl(char *hostname, int port);

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

int main()
{
	int fd;
	char json[8192];
	char json2[8192];
	int count;
	int sleepTime = 20;
	jsmn_parser parser;
	int i, r;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */
	struct ImageUpgradeResponse imageUpgradeResponse;
	char command[MAX_COMMAND_LEN+1];
	
	// Init SSL
	SSL_load_error_strings();
	SSL_library_init();

	
	fd = open("json.dat", O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(2);
	}
	count = read(fd, json, sizeof(json));
	close(fd);
	printf("Read %d bytes of json data!\n", count);
	fd = open("json2.dat", O_RDONLY);
	if (fd < 0) {
		perror("open");
		exit(2);
	}
	count = read(fd, json2, sizeof(json2));
	close(fd);
	printf("Read %d bytes of json2 data!\n", count);

	struct EMC_SERVER server;

	while (1) {
		// Initialize.
		memset(&server, 0, sizeof(server));

		// Find a server, and try to download an image
		if (emcDiscoverServer(&server) == TRUE) {
			debug_printf("Found an EMC server at %s:%d\n", server.hostname, server.port);
			/*
			 * Try to connect to the EMC server.
			 */
//			server.useExtremeDemoTag=1;
			server.sendAuthentication=1;
			if (httpsPut(&server, "/Clients/device/v1/switch/1405A-2016-Steve/connect",
						 json, strlen(json)) == TRUE) {
				if (server.response.responseCode == 200) {
					printf("Connect was successful!\n");
					/* Send in the image upgrade request. */
					server.useExtremeDemoTag=1;
					server.sendAuthentication=1;
					if (httpsPut(&server, "/Clients/device/v1/switch/1405A-2016-Steve/imageupgrade",
						 json2, strlen(json2)) == TRUE) {
						if (server.response.responseCode == 200) {
							printf("Image upgrade was successful!\n");
//							printf("%s\n", server.response.payload);
							/*
							 * Parse image name
							 */
							jsmn_init(&parser);
							r = jsmn_parse(&parser, server.response.payload,
										   strlen(server.response.payload), t,
										   sizeof(t)/sizeof(t[0]));
							if (r > 0) {
//								printf("Okay. We parsed it successfully. r=%d\n", r);
								for (i = 1; i < r; i++) {
									if (jsoneq(server.response.payload, &t[i], "upgrade") == 0) {
										/* We may use strndup() to fetch string value */
										sprintf(imageUpgradeResponse.upgrade,
												"%.*s", t[i+1].end-t[i+1].start,
												server.response.payload + t[i+1].start);
										i++;
									}
									if (jsoneq(server.response.payload, &t[i], "uri") == 0) {
										/* We may use strndup() to fetch string value */
										sprintf(imageUpgradeResponse.uri,
												"%.*s", t[i+1].end-t[i+1].start,
												server.response.payload + t[i+1].start);
										i++;

										/*
										 * We need to make some adjustments to the URI like
										 * pulling out the username, password as well as the
										 * IP address and the path.
										 */
										r = sscanf(imageUpgradeResponse.uri,
												   "%[^:]://%[^:]:%[^@]@%[^:]:%d/%s",
												   imageUpgradeResponse.protocol,
												   imageUpgradeResponse.username,
												   imageUpgradeResponse.password,
												   imageUpgradeResponse.ipaddress,
												   &imageUpgradeResponse.port,
												   imageUpgradeResponse.path
										);
										
										printf("Protocol = (%s)\n", imageUpgradeResponse.protocol);
										printf("Username = (%s)\n", imageUpgradeResponse.username);
										printf("Passsword = (%s)\n", imageUpgradeResponse.password);
										printf("IPAddress = (%s)\n", imageUpgradeResponse.ipaddress);
										printf("Port = (%d)\n", imageUpgradeResponse.port);
										printf("Path = (%s)\n", imageUpgradeResponse.path);

										/*
										 * We only support scp for now.
										 */
										if (!strcmp(imageUpgradeResponse.protocol, "scp")) {
											sprintf(command,
													"DROPBEAR_PASSWORD=%s scp -P %d %s@%s:/%s onieimage.bin\n",
													imageUpgradeResponse.password,
													imageUpgradeResponse.port,
													imageUpgradeResponse.username,
													imageUpgradeResponse.ipaddress,
													imageUpgradeResponse.path);
											system(command);
											system("/bin/sh onieimage.bin");
											system("reboot");
											while (1) {
												sleep(1);
											}
											
										}
									}
									
								} // end of walking tokens
								
								/*
								 * If 'doupgrade' is TRUE, then do the actual upgrade.
								 */
								if (!strcasecmp(imageUpgradeResponse.upgrade, "true")) {
									printf("DO UPGRADE: %s\n", imageUpgradeResponse.uri);
									printf("%s\n", command);
								} else {
									printf("Do not perform upgrade.\n");
								}
							}
							
						} else {
							printf("Failed to get imageupgrade stuff.\n");
						}
					}
				} else {
					printf("Error encountered when trying to connect!\n");
				}
			} // Connection successful.
		} // emcDiscoverServer

		/*
		 * Getting here means a server was not found, or an image is not available.
		 * Go to sleep, but then keep trying. Nothing else to do anyway.
		 */
		printf("Sleeping for %d seconds...\n", sleepTime);
		
		sleep(sleepTime);
		
	}
}

int debug_printf(char *format, ...)
{
	return(0);
	
	va_list argptr;
	va_start(argptr, format);
	printf("DEBUG: ");
	
	vprintf(format, argptr);
	va_end(argptr);
	return(0);
	
}



