#include <stdio.h>
#include <stdlib.h>
#include <errno.h>		/* ETIMEDOUT */
#include <string.h>		/* memcpy */
#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/socket.h> 

#include "unixsktapi.h"

int unix_domain_socket_init(int *socket_fd, const char *socket_path)
{
	int rc;
	int fd;
	int len;
	long fd_flags;
	struct sockaddr_un unix_addr;

	*socket_fd = 0;
	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		fprintf(stderr, "socket() failed, err: %d,[%s]", errno, strerror(errno));
		return (-1);
	}

	fd_flags = fcntl(fd, F_GETFD, 0);
	fcntl(fd, F_SETFD, fd_flags | FD_CLOEXEC);

	fd_flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, fd_flags | O_NONBLOCK);

	unlink(socket_path);

	memset(&unix_addr, 0, sizeof(unix_addr));
	unix_addr.sun_family = AF_UNIX;
	strcpy(unix_addr.sun_path, socket_path);
	len = sizeof(unix_addr);

	rc = bind(fd, (struct sockaddr *)&unix_addr, len);
	if (rc < 0) {
		fprintf(stderr, "bind() failed, err: %d,[%s]", errno, strerror(errno));
		close(fd);
		unlink(socket_path);
		return (-1);
	}

	rc = chmod(socket_path, 0666);
	if (rc < 0) {
		fprintf(stderr, "chmod() failed, err: %d,[%s]", errno, strerror(errno));
		close(fd);
		unlink(socket_path);
		return (-1);
	}

	rc = listen(fd, MAX_SOCKET_PENDING_CONNECTIONS);
	if (rc < 0) {
		fprintf(stderr, "listen() failed, err: %d,[%s]", errno, strerror(errno));
		close(fd);
		unlink(socket_path);
		return (-1);
	}

	*socket_fd = fd;

	return (0);

}

int unix_domain_socket_connect(int *socket_fd, const char *socket_path)
{
	int rc;
	int fd;
	long fd_flags;
	socklen_t len;
	struct sockaddr_un addr;

	if(socket_path == NULL) {
		fprintf(stderr, "invalid argument.\n");
		return (-1);
	}

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		fprintf(stderr, "socket() failed, err: %d,[%s]\n", errno, strerror(errno));
		return (-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, socket_path);
	len = sizeof(addr);

	fd_flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, fd_flags | O_NONBLOCK);

	rc = connect(fd, (struct sockaddr *)&addr, len);
	if (rc < 0) {
		if (errno != EINPROGRESS) {
			fprintf(stderr, "connect() failed, err: %d,[%s]\n", errno, strerror(errno));
			close(fd);
			return (-1);
		}
	}

	*socket_fd = fd;

	return (0);
}

int unix_domain_accept_req(int socket, int *acpt_socket, struct timeval * timeout)
{
	int fd;
	long fd_flags;
	int select_counter;
	fd_set recv_set;
	struct sockaddr_un addr;
	socklen_t len = sizeof(struct sockaddr);

	do {
		FD_ZERO(&recv_set);
		FD_SET(socket, &recv_set);

		select_counter = select(socket + 1, &recv_set, NULL, NULL, timeout);
		if (select_counter == 0) {
			if (timeout != NULL) {/* Time out... */				
				//fprintf(stdout, "select() timeout for accept().\n");
				return 0;
			} else {
				return (-1);
			}
		} else if (select_counter < 0) {
			return (-1);
		}

		if (FD_ISSET(socket, &recv_set)) {
			fd = accept(socket, (struct sockaddr *)&addr, &len);
			if (fd < 0) {
				fprintf(stderr, "accept() failed, err: %d,[%s].\n", errno, strerror(errno));
				usleep(100);
				continue;
			}
			fd_flags = fcntl(fd, F_GETFL, 0);
			fcntl(fd, F_SETFL, fd_flags | O_NONBLOCK);
			break;
		}
	} while (1);

	*acpt_socket = fd;
	return (1);

}
int unix_domain_send_msg(int socket, void * msg, int msg_len, struct timeval * timeout)
{
	int ret;
	int rc;
	int send_len;
	int select_counter;
	unsigned char *tx_buff;
	fd_set send_set;

	ret = 0;
	tx_buff = (unsigned char *)msg;
	send_len = 0;

	do {
		FD_ZERO(&send_set);
		FD_SET(socket, &send_set);

		select_counter = select(socket + 1, NULL, &send_set, NULL, timeout);
		if (select_counter == 0) {
			if (timeout != NULL) {
				/* Time out... */
				fprintf(stderr, "select() timeout for send() err: %d,[%s]\n", errno, strerror(errno));
				ret = -1;
				goto done;
			} else {
				continue;
			}
		} else if (select_counter < 0) {
			/* select be interrupt? */
			if (errno != EINTR) {
				fprintf(stderr, "select() return %d, err: %d,[%s]\n", select_counter, errno, strerror(errno));
				ret = -1;
				goto done;
			} else {
				continue;
			}
		}

		if (FD_ISSET(socket, &send_set)) {
			rc = send(socket, (const void *)&tx_buff[send_len], msg_len - send_len, 0);
			if (rc <= 0) {
				if ((errno == EAGAIN) || (errno == EINTR)
				    || (errno == EWOULDBLOCK)) {
					continue;
				} else {
					fprintf(stderr, "send() return %d, err: %d,[%s]\n", rc, errno, strerror(errno));
					ret = -1;
					goto done;
				}
			} else {
				send_len += rc;
			}
		}
	} while (send_len < msg_len);

done:
	return (ret);
}

int unix_domain_recv_msg(int socket, unix_domain_msg *req, unsigned int size, struct timeval * timeout)
{
	int ret = 0;
	int rc = 0;
	int fd = 0;
	unsigned int msg_len = 0;
	unsigned int recv_len = 0;
	int select_counter = 0;
	int got_msg_hdr = 0;
	unix_domain_msg *org_msg = NULL;
	unsigned char *rx_buff = NULL;
	fd_set recv_set;

	ret = 0;
	fd = socket;
	org_msg = req;
	rx_buff = (unsigned char *)org_msg;
	recv_len = 0;
	msg_len = size;
	got_msg_hdr = 0;

	do {
		FD_ZERO(&recv_set);
		FD_SET(fd, &recv_set);

		select_counter = select(fd + 1, &recv_set, NULL, NULL, timeout);
		if (select_counter == 0) {
			if (timeout != NULL) {
				/* Time out... */
				fprintf(stderr, "select() timeout for recv() err: [%d],[%s].\n", errno, strerror(errno));
				ret = -1;
				goto done;
			} else {
				continue;
			}
		} else if (select_counter < 0) {
			/* select be interrupt? */
			if (errno != EINTR) {
				fprintf(stderr, "select() return %d err: [%d],[%s].\n", select_counter, errno, strerror(errno));
				ret = -1;
				goto done;
			} else {
				continue;
			}
		}

		if (FD_ISSET(fd, &recv_set)) {
			rc = recv(fd, (void *)&rx_buff[recv_len], msg_len - recv_len, 0);
			if (rc > 0) {
				recv_len += rc;
				if (!got_msg_hdr) {
					if (recv_len >= sizeof(unix_domain_msg)) {
						got_msg_hdr = 1;
						msg_len = sizeof(unix_domain_msg) + req->size;
						if (size < msg_len) {
							rx_buff = (unsigned char *)malloc(msg_len);
							if (rx_buff == NULL) {
								fprintf(stderr, "malloc(%d) failed, err: %d,[%s]\n", msg_len, errno, strerror(errno));
								ret = -1;
								goto done;
							} else {
								memcpy(rx_buff, (void *)org_msg, recv_len);
								req = (unix_domain_msg *) rx_buff;
							}
						}
					}
				}
			} else if (rc < 0) {
				if ((errno == EAGAIN) || (errno == EINTR)
				    || (errno == EWOULDBLOCK)) {
					continue;
				} else {
					fprintf(stderr, "recv() return [%d] err: [%d],[%s].\n", rc, errno, strerror(errno));
					ret = -1;
					goto done;
				}
			} else {
				fprintf(stderr, "recv() peer shutdown!\n");
				ret = -1;
				break;
			}
		}
	} while (recv_len < msg_len);

done:
	return (ret);
}

int unix_domain_socket_clean(int socket, const char *socket_path)
{
	if (socket > 0) {
		close(socket);
		unlink(socket_path);
	}

	return (0);
}