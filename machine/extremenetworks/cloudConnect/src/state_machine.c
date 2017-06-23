/*
 * Copyright (c) Extreme Networks Inc. 2017
 * All rights reserved
 *
 * Module:
 *     state_machine.c
 *
 * Author:
 *     Steve Delahunty <sdelahunty@extremenetworks.com>
 *
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *  ##### DEMO-ONLY CODE!!  DO NOT USE IN PRODUCTION!!!!! #########
 *     
 * Description:
 *     This module contains the functions that make up the state engine.
 *     Each function name begins with StateMachine_ and ends with the
 *     actual state name. e.g. StateMachine_Init().
 */
#include "onie_connector.h"

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

char *systemCommand(const char *cmd) 
{
	char buffer[MAX_COMMAND_LEN+1];
	static char response[MAX_RESPONSE_LEN+1];
	int ret, fd;
	char *ptr;

	// Init response buffer
	memset(response, 0, sizeof(response));

	/*
	 * Build up the actual command.
	 */
	strcpy(buffer, cmd);
	strcat (buffer, " > /tmp/sysout.tmp");

	// Execute
	ret = system(buffer);
	
	// Make sure it worked
	if (ret != 0) return(response);

	/*
	 * Get the command response. Throw away newlines at the end.
	 */
	fd = open("/tmp/sysout.tmp", O_RDONLY);
	read(fd, response, sizeof(response));
	close(fd);
	ptr = strchr(response,'\n');
	if (ptr) *ptr=0;

	// All done.
	return(response);
}

char *eepromValue(const char *cmd) 
{
	char buffer[MAX_COMMAND_LEN+1];
	static char response[MAX_RESPONSE_LEN+1];
	int ret, fd;
	char *ptr;

	// Init response buffer
	memset(response, 0, sizeof(response));

	/*
	 * Build up the actual command.
	 */
	strcpy(buffer, "cat /tmp/syseeprom.txt | grep ");
	strcat(buffer, cmd);
	strcat (buffer, " > /tmp/sysout.tmp");

	// Execute
	ret = system(buffer);
	
	// Make sure it worked
	if (ret != 0) return(response);

	/*
	 * Get the command response. Throw away newlines at the end.
	 */
	fd = open("/tmp/sysout.tmp", O_RDONLY);
	read(fd, response, sizeof(response));
	close(fd);
	ptr = strchr(response,'\n');
	if (ptr) *ptr=0;

	// All done.
	return(response);
}


extern void StateMachine_Init(struct EMC_SERVER *server)
{
	debug_printf("STATE: Init - Initialize internal data structures...\n");
	const char *cmd;
	char *ptr, *resp;
	
	// Initialize the server
	memset(server, 0, sizeof(struct EMC_SERVER));

	// Initialize variables
	server->retrySleep = 30; // seconds

	/*
	 * Some of these should come from the actual system or
	 * from the values stored in EEPROM
	 */
	cmd = "'Base MAC Address' | awk '{print $6}'";
	strcpy(server->macAddress, eepromValue(cmd));
	cmd = "'ONIE Version' | awk '{print $5}'";
	strcpy(server->ruSwVersion, eepromValue(cmd));
	cmd = "'Product Name' | awk '{print $5}'";
	strcpy(server->ruModel, eepromValue(cmd));
	cmd = "'Serial Number' | awk '{print $5}'";
	strcpy(server->ruSerialNumber, eepromValue(cmd));
	cmd = "hostname";
	strcpy(server->sysName, systemCommand(cmd));
	cmd = "uname -r";
	strcpy(server->firmwareRevision, systemCommand(cmd));
	cmd = "uname -r";
	strcpy(server->operatingSystem, systemCommand(cmd));
	cmd = "cat /proc/uptime | awk '{print $1}'";
	resp = systemCommand(cmd);
	ptr = strchr(resp, '.');
	if (ptr) *ptr=0; // remove the period
	strcpy(server->sysUpTime, resp);

	/*
	 * These variables are hardcoded and do not change between
	 * systems.
	 */
	strcpy(server->sysObjectId, "1.3.6.1.4.1.1916.2.287");
	strcpy(server->serialNumber, "999-888-111");
	strcpy(server->sysDescr, "Linux ONIE");
	strcpy(server->sysContact, "email:admin@extremenetworks.com, +1 (xxx)xxx-xxxx");
	
	// Move to the "discover" state
	server->state = STATE_DISCOVER;
}

extern void StateMachine_Discover(struct EMC_SERVER *server)
{
	debug_printf("STATE: Discover - Find an EMC server...\n");
	if (emcDiscoverServer(server) == TRUE) {
		debug_printf("    Found an EMC server at %s:%d\n", server->hostname, server->port);
		server->state = STATE_CONNECT;
	} else {
		debug_printf("    Unable to find an EMC server\n");
		server->state = STATE_RETRY;
	}
}

extern void StateMachine_Connect(struct EMC_SERVER *server)
{
	char json[8192];
	char uri[MAX_URIPATH_LEN+1];
	
	debug_printf("STATE: Connect - Connect to an EMC server...\n");

	/*
	 * Build up the json object to send in the connect
	 * message.
	 */
	memset(json, 0, sizeof(json));
	strcat(json, "{\n\"apPropertyBlock\": { \n");
	strcat(json, "   \"bpWiredMacaddr\": \"");
	strcat(json, server->macAddress);
	strcat(json, "\",\n   \"ruSwVersion\": \"");
	strcat(json, server->ruSwVersion);
	strcat(json, "\",\n   \"ruModel\": \"");
	strcat(json, server->ruModel);
	strcat(json, "\",\n   \"ruSerialNumber\": \"");
	strcat(json, server->ruSerialNumber);
	strcat(json, "\"\n},\n");
	strcat(json, "\"deviceInfo\": { ");
	strcat(json, "\n    \"sysName\": \"");
	strcat(json, server->sysName);
	strcat(json, "\",\n    \"sysObjectId\": \"");
	strcat(json, server->sysObjectId);
	strcat(json, "\",\n    \"macAddr\": \"");
	strcat(json, server->macAddress);
	strcat(json, "\",\n    \"serialNumber\": \"");
	strcat(json, server->serialNumber);
	strcat(json, "\",\n    \"capabilities\": []");
	strcat(json, ",\n    \"ports\": [\n      {\n        \"portType\": \"access\",");
	strcat(json, "\n        \"portSpeed\": 1000, ");
	strcat(json, "\n        \"portList\": [ \"eth0\" ]\n      }\n    ],");
	strcat(json, "\n    \"firmwareRevision\": \"");
	strcat(json, server->firmwareRevision);
	strcat(json, "\",\n    \"sysUpTime\": \"");
	strcat(json, server->sysUpTime);
	strcat(json, "\",\n    \"operatingSystem\": \"");
	strcat(json, server->operatingSystem);
	strcat(json, "\",\n    \"sysDescr\": \"");
	strcat(json, server->sysDescr);
	strcat(json, "\",\n    \"sysContact\": \"");
	strcat(json, server->sysContact);
	strcat(json, "\"\n    }\n}");
	
//	printf("%s\n", json);
	
	server->useExtremeDemoTag=0;
	server->sendAuthentication=1;
	sprintf(uri, "/Clients/device/v1/switch/%s/connect", server->ruSerialNumber);
	
	if (httpsPut(server, uri, json, strlen(json)) == TRUE) {
		if (server->response.responseCode == 200 || 
			server->response.responseCode == 201) {
			debug_printf("    Successfully connected to %s:%d\n", server->hostname, server->port);
			server->state = STATE_GETIMAGE;
			return;
		}
	}
	debug_printf("    Failed to connect to %s:%d\n", server->hostname, server->port);
	server->state = STATE_RETRY;
}

extern void StateMachine_GetImage(struct EMC_SERVER *server)
{
	jsmn_parser parser;
	int i, r;
	jsmntok_t t[128]; /* We expect no more than 128 tokens */
	char json[8192];
	struct ImageUpgradeResponse imageUpgradeResponse;
	char uri[MAX_URIPATH_LEN+1];
	
	debug_printf("STATE: GetImage - Get an image to install..\n");

	/*
	 * Build up the json object to send in the connect
	 * message.
	 */
	memset(json, 0, sizeof(json));
	strcat(json, "{\n\"apPropertyBlock\": { \n");
	strcat(json, "   \"bpWiredMacaddr\": \"");
	strcat(json, server->macAddress);
	strcat(json, "\",\n   \"ruSwVersion\": \"");
	strcat(json, server->ruSwVersion);
	strcat(json, "\",\n   \"ruModel\": \"");
	strcat(json, server->ruModel);
	strcat(json, "\",\n   \"ruSerialNumber\": \"");
	strcat(json, server->ruSerialNumber);
	strcat(json, "\"\n},\n");
	strcat(json, "\"assets\": [ { ");
	strcat(json, "\n    \"assetName\": \"onie\",");
	strcat(json, "\n    \"assetVersion\": \"1.0.0.0O\",");
	strcat(json, "\n    \"assetType\": \"FIRMWARE\"");
	strcat(json, "\n    } ]\n }\n");

//	printf("%s\n", json);
	

	// Send in the request
	server->useExtremeDemoTag=1;
	server->sendAuthentication=1;
	sprintf(uri, "/Clients/device/v1/switch/%s/imageupgrade", server->ruSerialNumber);
	if (httpsPut(server, uri, json, strlen(json)) != TRUE) {
		debug_printf("    Failed to getimage from %s:%d\n", server->hostname, server->port);
		server->state = STATE_RETRY;
		return;
	}
	
	if (server->response.responseCode != 200) {
		debug_printf("    Failed to getimage from %s:%d\n", server->hostname, server->port);
		server->state = STATE_RETRY;
		return;
	}
	debug_printf("    Success getting image from %s:%d\n", server->hostname, server->port);
	debug_printf("    Response Payload: %s\n", server->response.payload);
	
	/*
	 * Parse image name
	 */
	jsmn_init(&parser);
	r = jsmn_parse(&parser, server->response.payload,
				   strlen(server->response.payload), t,
				   sizeof(t)/sizeof(t[0]));
	if (r <= 0) {
		debug_printf("    Failed to parse json response\n", server->hostname, server->port);
		server->state = STATE_RETRY;
		return;
	}
	for (i = 1; i < r; i++) {
		if (jsoneq(server->response.payload, &t[i], "upgrade") == 0) {
			/* We may use strndup() to fetch string value */
			sprintf(imageUpgradeResponse.upgrade,
					"%.*s", t[i+1].end-t[i+1].start,
					server->response.payload + t[i+1].start);
			i++;
		}
		if (jsoneq(server->response.payload, &t[i], "uri") == 0) {
			/* We may use strndup() to fetch string value */
			sprintf(imageUpgradeResponse.uri,
					"%.*s", t[i+1].end-t[i+1].start,
					server->response.payload + t[i+1].start);
			i++;
		}

	}
	/*
	 * Check the 'upgrade' value to determine of we are supposed to
	 * upgrade this system.
	 */
	if (!strcmp(imageUpgradeResponse.upgrade, "false")) {
		debug_printf("    'upgrade' flag is set to false. Do not upgrade.\n");
		server->state = STATE_RETRY;
		return;
	}
	
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

	/*
	 * Make sure the protocol is "scp"
	 */
	if (strcmp(imageUpgradeResponse.protocol, "scp")) {
		debug_printf("    Invalid imageUpgradeBlock. Missing 'scp' protocol\n");
		server->state = STATE_RETRY;
		return;
	}
	
	
	debug_printf("    Protocol = (%s)\n", imageUpgradeResponse.protocol);
	debug_printf("    Username = (%s)\n", imageUpgradeResponse.username);
	debug_printf("    Passsword = (%s)\n", imageUpgradeResponse.password);
	debug_printf("    IPAddress = (%s)\n", imageUpgradeResponse.ipaddress);
	debug_printf("    Port = (%d)\n", imageUpgradeResponse.port);
	debug_printf("    Path = (%s)\n", imageUpgradeResponse.path);
	
	/*
	 * Build up the command to download
	 */
	sprintf(server->command,
			"DROPBEAR_PASSWORD=%s scp -y -P %d %s@%s:/%s onieimage.bin",
			imageUpgradeResponse.password,
			imageUpgradeResponse.port,
			imageUpgradeResponse.username,
			imageUpgradeResponse.ipaddress,
			imageUpgradeResponse.path);

	// Download the image
	r = system(server->command);
	if (r != 0) {
		debug_printf("    Download failed with code %d\n", r);
		server->state = STATE_RETRY;
		return;
	}
	
	// Advance to the next state.
	server->state = STATE_INSTALL;
}

extern void StateMachine_Install(struct EMC_SERVER *server)
{
	int ret=-1;
	
	debug_printf("STATE: Install - Install the downloaded image..\n");
	ret = system("sh onieimage.bin");
	if (ret != 0) {
		debug_printf("    Install failed with code %d\n", ret);
		server->state = STATE_RETRY;
		return;
	}

	// Advance to the next state.
	server->state = STATE_REBOOT;
}

extern void StateMachine_Reboot(struct EMC_SERVER *server)
{
	debug_printf("STATE: Reboot - Reboot the system..\n");
	debug_printf("    Sending the 'reboot' command....\n");
	system("reboot");
	debug_printf("    Sleeping forever\n");
	while (1) {
		sleep(1000);
	}
}

extern void StateMachine_Retry(struct EMC_SERVER *server)
{
	debug_printf("STATE: Retry - Sleep for %d seconds and retry..\n", server->retrySleep);
	debug_printf("    Sleeping for %d seconds...\n", server->retrySleep);
	sleep(server->retrySleep);

	// Set the state to INIT
	server->state = STATE_INIT;
}

