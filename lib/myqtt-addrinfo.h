/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2015 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For commercial support on build MQTT enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/mqtt
 *                        http://www.aspl.es/myqtt
 */

#ifndef __MYQTT_ADDRINFO_H__
#define __MYQTT_ADDRINFO_H__

#if defined(AXL_OS_UNIX)
#  ifndef NI_MAXHOST
/* define some values */
#  define   NI_MAXHOST 1025
#  define   NI_MAXSERV 512

/* define ai_passive */
#ifndef AI_PASSIVE
#define AI_PASSIVE 0x00000001 /* get address to use bind() */
#endif

#ifndef AI_NUMERICHOST
# define AI_NUMERICHOST	0x0004	/* Don't use name resolution.  */
#endif

#ifndef NI_NUMERICHOST
# define NI_NUMERICHOST	1	/* Don't try to look up hostname.  */
#endif
#ifndef NI_NUMERICSERV
# define NI_NUMERICSERV 2	/* Don't convert port number to name.  */
#endif

/** 
 * NOTE: we are including here static definitions because we are
 * compiling with -ansi flag. However, that flag also hides IPv6 new
 * resolution API. We can disable -ansi flag because we want MyQtt
 * Library to remain ANSI-C project so we have to include some static
 * definitions to have them. If you have a better idea or you think
 * this can admit improvements, please, contact us. We will be glad to
 * hear you! */
struct addrinfo {
	int     ai_flags;
	int     ai_family;
	int     ai_socktype;
	int     ai_protocol;
	size_t  ai_addrlen;
	struct  sockaddr *ai_addr;
	char    *ai_canonname;     /* canonical name */
	struct  addrinfo *ai_next; /* this struct can form a linked list */
};

int getaddrinfo (const char *hostname,
		 const char *service,
		 const struct addrinfo *hints,
		 struct addrinfo **res);

int getnameinfo (const struct sockaddr *sa, socklen_t salen,
		 char *host, size_t hostlen,
		 char *serv, size_t servlen,
		 int flags);

const char * gai_strerror (int errcode);

void freeaddrinfo(struct addrinfo *ai);

#  endif
#endif

#endif /* __MYQTT_ADDRINFO_H__ */
