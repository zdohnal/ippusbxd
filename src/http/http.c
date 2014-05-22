#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "http.h"


http_sock *open_http()
{
	http_sock *this = calloc(1, sizeof *this);
	if (this == NULL) {
		ERR("callocing this failed");
		goto error;
	}

	// Open [S]ocket [D]escriptor
	this->sd = -1;
	this->sd = socket(AF_INET6, SOCK_STREAM, 0);
	if (this->sd < 0) {
		ERR("sockect open failed");
		goto error;
	}

	// Configure socket params
	struct sockaddr_in6 addr;
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(8080); // let kernel decide
	addr.sin6_addr = in6addr_any;

	// Bind to localhost
	if (bind(this->sd,
	        (struct sockaddr *)&addr,
	        sizeof addr) < 0) {
		ERR("bind failed");
		goto error;
	}

	// Let kernel over-accept HTTPMAXPENDCONNS number of connections
	if (listen(this->sd, HTTP_MAX_PENDING_CONNS) < 0) {
		ERR("listen failed on socket");
		goto error;
	}

	return this;

error:
	if (this->sd != -1)
		close(this->sd);
	if (this != NULL)
		free(this);

	return NULL;
}

packet *get_packet(message *msg)
{
	size_t capacity = BUFFER_STEP * BUFFER_INIT_RATIO;
	uint8_t *buf = malloc(capacity * (sizeof *buf));
	if (buf == NULL) {
		ERR("malloc failed for buf");
		goto error;
	}

	// Any portion of a packet left from last time?
	
	// Read until we have atleast one packet
	size_t size_read = recv(msg->parent_session->sd, buf, capacity, 0);
	
	// Did we receive more than a packets worth?
	
	// Assemble packet
	packet *pkt = calloc(1, sizeof *pkt);
	if (pkt == NULL) {
		ERR("calloc failed for packet");
		goto error;
	}
	pkt->buffer = buf;
	pkt->size = size_read;
	pkt->parent_message = msg;

	return pkt;	
	 
error:
	if (buf != NULL)
		free(buf);
	if (pkt != NULL)
		free(pkt);
	return NULL;
}

message *get_message(http_conn *conn)
{
	message *msg = calloc(1, sizeof *msg);
	if (msg == NULL) {
		ERR("calloc failed on this");
		goto error;
	}
	msg->parent_session = conn;

	return msg;

error:
	if (msg != NULL)
		free(msg);
	return NULL;
}

http_conn *accept_conn(http_sock *sock)
{
	http_conn *conn = calloc(1, sizeof *conn);
	if (conn == NULL) {
		ERR("Calloc for connection struct failed");
		goto error;
	}

	conn->sd = accept(sock->sd, NULL, NULL);
	if (conn->sd < 0) {
		ERR("accept failed");
		goto error;
	}

	return conn;

error:
	if (conn != NULL)
		free(conn);
	return NULL;
}

void close_http(http_sock *this)
{
	close(this->sd);
	free(this);
}

/* Valid methods for determining http request
 * size are defined by W3 in RFC2616 section 4.4
 * link: http://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.4
 */