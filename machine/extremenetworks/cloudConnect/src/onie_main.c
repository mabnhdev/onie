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

int main(int argc, char **argv)
{
	struct EMC_SERVER server;

	/*
	 * Extract the system eeprom settings and save them to a temporary
	 * file. We do this because it takes a lot of time to query the
	 * eeprom so we,instead, query the copy.
	 */
	system("onie-syseeprom > /tmp/syseeprom.txt");

	// Init SSL
	SSL_load_error_strings();
	SSL_library_init();

	/*
	 * Initialize the machine state.
	 */
	server.state = STATE_INIT; // prime it.

	/*
	 * Stay in this loop forever. Check the machine state
	 * and call the appropriate function.
	 */
	while (1) {
		switch (server.state) {
		case STATE_INIT:
			StateMachine_Init(&server);
			break;
		case STATE_DISCOVER:
			StateMachine_Discover(&server);
			break;
		case STATE_CONNECT:
			StateMachine_Connect(&server);
			break;
		case STATE_GETIMAGE:
			StateMachine_GetImage(&server);
			break;
		case STATE_INSTALL:
			StateMachine_Install(&server);
			break;
		case STATE_REBOOT:
			StateMachine_Reboot(&server);
			break;
		case STATE_RETRY:
			StateMachine_Retry(&server);
			break;
		default:
			printf("Invalid machine state: %d\n", server.state);
			exit(-2);
			
		} // end of switch
	}
}

int debug_printf(const char *format, ...)
{
//	return(0);
	
	va_list argptr;
	va_start(argptr, format);
	printf("ONIE: ");
	
	vprintf(format, argptr);
	va_end(argptr);
	return(0);
	
}



