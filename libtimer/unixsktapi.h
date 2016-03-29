/*
 * Copyright (C) 2014
 *
 * This program is free software; 
 *
 * Author(s): GrandJack
 */

/*
 * This C module implements a simple timer library.
 */

#ifndef UNIX_SOCKET_API_H
#define UNIX_SOCKET_API_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SOCKET_PENDING_CONNECTIONS 64

typedef struct {
	unsigned int size;
	char data[0];
}unix_domain_msg;

int unix_domain_socket_init(int *socket_fd, const char *socket_path);
int unix_domain_socket_connect(int *socket_fd, const char *socket_path);
int unix_domain_accept_req(int socket, int *acpt_socket, struct timeval * timeout);
int unix_domain_send_msg(int socket, void * msg, int msg_len, struct timeval * timeout);
int unix_domain_recv_msg(int socket, unix_domain_msg * req, unsigned int size, struct timeval * timeout);
int unix_domain_socket_clean(int socket, const char *socket_path);

#ifdef __cplusplus
}
#endif

#endif /*UNIX_SOCKET_API_H*/