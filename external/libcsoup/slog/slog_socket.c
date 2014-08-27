
/*  slog.c - a simple interface for logging/debugging

    Copyright (C) 2011  "Andy Xuming" <xuming@users.sourceforge.net>

    This file is part of CSOUP, Chicken Soup library

    CSOUP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CSOUP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#define CSOUP_DEBUG_LOCAL     SLOG_CWORD(CSOUP_MOD_SLOG, SLOG_LVL_INFO)

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <signal.h>  
#include <pthread.h>
#include <errno.h>

#include "csoup_internal.h"

struct	slog_socket	{
	pthread_t	listenner;
	pthread_mutex_t	sync;
	SMMDBG		*dbgc;

	int		sock_listen;
	int		port_listen;
	int		sock_stream;
};

static void *slog_tcp_host(void *self);
static int slog_tcp_output(void *maindbg, void *self, char *buf);
static int slog_unbind_tcp(SMMDBG *dbgc);


void *slog_bind_tcp(SMMDBG *dbgc, int port)
{
	struct	sockaddr_in	servaddr;
	struct	slog_socket	*netobj;

	if (dbgc->f_inet) {
		slog_unbind_tcp(dbgc);
	}
	if (port <= 0) {
		return NULL;
	}

	if ((netobj = smm_alloc(sizeof(struct slog_socket))) == NULL) {
		return NULL;
	}

	netobj->sock_stream = -1;
	netobj->port_listen = port;
	netobj->dbgc = dbgc;
	if ((netobj->sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		smm_free(netobj);
		return NULL;
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if (bind(netobj->sock_listen, (struct sockaddr *)&servaddr, 
				sizeof(servaddr)) < 0) {
		close(netobj->sock_listen);
		smm_free(netobj);
		return NULL;
	}

	listen(netobj->sock_listen, 1);

	pthread_mutex_init(&netobj->sync, NULL);
	pthread_mutex_lock(&netobj->sync);	/* turn to a semaphore */

	if (pthread_create(&netobj->listenner, NULL, slog_tcp_host, netobj)) {
		close(netobj->sock_listen);
		pthread_mutex_destroy(&netobj->sync);
		smm_free(netobj);
		return NULL;
	}
	return NULL;
}

static void *slog_tcp_host(void *self)
{
	struct	slog_socket	*netobj = self;
	struct	sockaddr_in	clntaddr;
	socklen_t		salen;

	while (1) {
		CDB_FUNC(("slog_tcp_host: waiting on TCP port %d\n", 
					netobj->port_listen));
		salen = sizeof(clntaddr);
		netobj->sock_stream = accept(netobj->sock_listen, 
				(struct sockaddr *) &clntaddr, &salen);
		if (netobj->sock_stream < 0) {
			break;
		}
		CDB_INFO(("slog_tcp_host: connected by %s\n", 
					inet_ntoa(clntaddr.sin_addr)));
		netobj->dbgc->netobj = netobj;			//need lock
		netobj->dbgc->f_inet = slog_tcp_output;		//need lock

		pthread_mutex_lock(&netobj->sync);	/* suspend on sem */
	}
	CDB_ERROR(("slog_tcp_host: broken by %s\n", strerror(errno)));
	return NULL;
}

static int slog_tcp_output(void *maindbg, void *self, char *buf)
{
	struct	slog_socket	*netobj = self;
	SMMDBG	*dbgc = maindbg;
	int	rc, len;

	if ((self == NULL) || (buf == NULL)) {
		return -1;
	}
	
	len = strlen(buf);
	while (len) {
		rc = send(netobj->sock_stream, buf, len, MSG_NOSIGNAL);
		if (rc < 0) {
			CDB_INFO(("slog_tcp_output: peer closed\n"));
			dbgc->f_inet = NULL;		//need lock
			dbgc->netobj = NULL;		//need lock
			close(netobj->sock_stream);
			pthread_mutex_unlock(&netobj->sync);
			return -1;
		}
		buf += rc;
		len -= rc;
	}
	return 0;
}

static int slog_unbind_tcp(SMMDBG *dbgc)
{
	struct	slog_socket	*netobj;

	if (dbgc->f_inet) {
		dbgc->f_inet = NULL;
	}
	if (dbgc->netobj) {
		netobj = dbgc->netobj;
		dbgc->netobj = NULL;
		//kill(0, SIGIO);
		pthread_cancel(netobj->listenner);
		pthread_join(netobj->listenner, NULL);
		pthread_mutex_destroy(&netobj->sync);
		if (netobj->sock_stream) {
			close(netobj->sock_stream);
		}
		if (netobj->sock_listen) {
			close(netobj->sock_listen);
		}
		smm_free(netobj);
	}
	return 0;
}

