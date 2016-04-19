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
#ifndef __MYQTT_IO_H__
#define __MYQTT_IO_H__

#include <myqtt.h>

/* api to configure current I/O system */
axl_bool             myqtt_io_waiting_use                     (MyQttCtx           * ctx,
								MyQttIoWaitingType   type);

axl_bool             myqtt_io_waiting_is_available            (MyQttIoWaitingType type);

MyQttIoWaitingType  myqtt_io_waiting_get_current             (MyQttCtx           * ctx);

void                 myqtt_io_waiting_set_create_fd_group     (MyQttCtx           * ctx,
								MyQttIoCreateFdGroup create);

void                 myqtt_io_waiting_set_destroy_fd_group    (MyQttCtx           * ctx,
								MyQttIoDestroyFdGroup destroy);

void                 myqtt_io_waiting_set_clear_fd_group      (MyQttCtx           * ctx,
								MyQttIoClearFdGroup clear);

void                 myqtt_io_waiting_set_add_to_fd_group     (MyQttCtx           * ctx,
								MyQttIoAddToFdGroup add_to);

void                 myqtt_io_waiting_set_is_set_fd_group     (MyQttCtx           * ctx,
								MyQttIoIsSetFdGroup is_set);

void                 myqtt_io_waiting_set_have_dispatch       (MyQttCtx           * ctx,
								MyQttIoHaveDispatch  have_dispatch);

void                 myqtt_io_waiting_set_dispatch            (MyQttCtx           * ctx,
								MyQttIoDispatch      dispatch);

void                 myqtt_io_waiting_set_wait_on_fd_group    (MyQttCtx           * ctx,
								MyQttIoWaitOnFdGroup wait_on);

/* api to perform invocations to the current I/O system configured */
axlPointer           myqtt_io_waiting_invoke_create_fd_group  (MyQttCtx           * ctx,
								MyQttIoWaitingFor    wait_to);

void                 myqtt_io_waiting_invoke_destroy_fd_group (MyQttCtx           * ctx,
								axlPointer            fd_group);

void                 myqtt_io_waiting_invoke_clear_fd_group   (MyQttCtx           * ctx,
								axlPointer            fd_group);

axl_bool             myqtt_io_waiting_invoke_add_to_fd_group  (MyQttCtx           * ctx,
								MYQTT_SOCKET         fds, 
								MyQttConn    * connection, 
								axlPointer            fd_group);

axl_bool             myqtt_io_waiting_invoke_is_set_fd_group  (MyQttCtx           * ctx,
								MYQTT_SOCKET         fds, 
								axlPointer fd_group,
								axlPointer user_data);

axl_bool             myqtt_io_waiting_invoke_have_dispatch    (MyQttCtx           * ctx,
								axlPointer            fd_group);

void                 myqtt_io_waiting_invoke_dispatch         (MyQttCtx           * ctx,
								axlPointer            fd_group, 
								MyQttIoDispatchFunc  func,
								int                   changed,
								axlPointer            user_data);

int                  myqtt_io_waiting_invoke_wait             (MyQttCtx           * ctx,
								axlPointer            fd_group, 
								int                   max_fds,
								MyQttIoWaitingFor    wait_to);

void                 myqtt_io_init (MyQttCtx * ctx);

/* internal API */
axlPointer __myqtt_io_waiting_default_create  (MyQttCtx * ctx, MyQttIoWaitingFor wait_to);
void       __myqtt_io_waiting_default_destroy (axlPointer fd_group);
void       __myqtt_io_waiting_default_clear   (axlPointer __fd_group);
int        __myqtt_io_waiting_default_wait_on (axlPointer __fd_group, 
					       int        max_fds, 
					       MyQttIoWaitingFor wait_to);
axl_bool   __myqtt_io_waiting_default_add_to  (int                fds, 
					       MyQttConn * connection,
					       axlPointer         __fd_set);
axl_bool   __myqtt_io_waiting_default_is_set  (int        fds, 
					       axlPointer __fd_set, 
					       axlPointer user_data);

#endif
