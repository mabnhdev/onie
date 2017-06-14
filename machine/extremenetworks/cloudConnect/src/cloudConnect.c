/*
 * Sample "C" file to make sure an openssl socket application can be built.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>

size_t calcDecodeLength(const char* b64input) 
{
	//Calculates the length of a decoded string
	size_t len = strlen(b64input),
		padding = 0;
	
	if (b64input[len-1] == '=' && b64input[len-2] == '=') //last two chars are =
		padding = 2;
	
	else if (b64input[len-1] == '=') //last char is =
		padding = 1;
	return (len*3)/4 - padding;
}

int Base64Decode(char* b64message, unsigned char** buffer, size_t* length) 
{
	//Decodes a base64 encoded string
	BIO *bio, *b64;

	int decodeLen = calcDecodeLength(b64message);
	*buffer = (unsigned char*)malloc(decodeLen + 1);
	(*buffer)[decodeLen] = '\0';
	bio = BIO_new_mem_buf(b64message, -1);
	b64 = BIO_new(BIO_f_base64());
	bio = BIO_push(b64, bio);

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
	//Do not use newlines to flush buffer
	*length = BIO_read(bio, *buffer, strlen(b64message));

	//length should equal decodeLen, else something went horribly wrong
	BIO_free_all(bio);
	return (0);
}



int Base64Encode(const unsigned char* buffer, size_t length, char** b64text) 
{
	//Encodes a binary safe base 64 string
	BIO *bio, *b64;
	BUF_MEM *bufferPtr;
	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
	BIO_write(bio, buffer, length);
	BIO_flush(bio);
	BIO_get_mem_ptr(bio, &bufferPtr);
	BIO_set_close(bio, BIO_NOCLOSE);
	BIO_free_all(bio);
	*b64text=(*bufferPtr).data;
	return (0);
}

SSL_CTX* InitCTX(void)
{
	const SSL_METHOD *method;
	SSL_CTX *ctx;
	method = TLSv1_2_client_method();
	ctx = SSL_CTX_new(method);
	if ( ctx == NULL ) {
		ERR_print_errors_fp(stderr);
		abort();
	}
	return ctx;
}


SSL *socks_connect_ssl(char *hostname, int port) 
{
    struct sockaddr_in sa;
    struct hostent *result;
    int fd;
	SSL_METHOD *method;
	SSL *ssl;
	SSL_CTX *ctx;
	
    // Initialize
    memset((struct sockaddr *) &sa, 0, sizeof(sa));

    // Get information about the host
    result = gethostbyname(hostname);
    if (result) {
        memcpy(&sa.sin_addr, result->h_addr_list[0], 
               result->h_length);
    } else {
        printf("ERROR: Unknown host: %s\n", hostname);
        return(NULL);
        
    }

    // Load up the sockaddr_in structure
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

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
	printf("Connected with %s cipher..\n", SSL_get_cipher(ssl));
    return(ssl);
}


main() {
	SSL *ssl;

	SSL_load_error_strings();
	SSL_library_init();
	ssl = socks_connect_ssl("psimonea-pc1.corp.extremenetworks.com", 8443);
}


