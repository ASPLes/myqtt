/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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

#ifndef __MYQTT_WIN32_H__
#define __MYQTT_WIN32_H__

#include <myqtt.h>

BEGIN_C_DECLS

int           myqtt_win32_init (MyQttCtx * ctx);

int           myqtt_win32_nonblocking_enable (MYQTT_SOCKET socket);

int           myqtt_win32_blocking_enable    (MYQTT_SOCKET socket);

BOOL APIENTRY DllMain                         (HINSTANCE hInst,
					       DWORD reason,
					       LPVOID reserved);

/* gettimeofday support on windows */
#if ! defined(HAVE_GETTIMEOFDAY)

int gettimeofday(struct timeval *tv, axlPointer notUsed);

#endif /* end ! defined(HAVE_GETTIMEOFDAY) */

const char* myqtt_win32_inet_ntop (int af, const void* src, char* dst, int cnt);

END_C_DECLS

#endif
