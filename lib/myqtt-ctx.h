/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2014 Advanced Software Production Line, S.L.
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
#ifndef __MYQTT_CTX_H__
#define __MYQTT_CTX_H__

#include <myqtt.h>

BEGIN_C_DECLS

MyQttCtx  * myqtt_ctx_new                       (void);

void        myqtt_ctx_set_data                  (MyQttCtx       * ctx, 
						 const char      * key, 
						 axlPointer        value);

void        myqtt_ctx_set_data_full             (MyQttCtx       * ctx, 
						 const char      * key, 
						 axlPointer        value,
						 axlDestroyFunc    key_destroy,
						 axlDestroyFunc    value_destroy);

axlPointer  myqtt_ctx_get_data                  (MyQttCtx       * ctx,
						  const char      * key);

/*** global event notificaitons ***/
void        myqtt_ctx_set_on_publish            (MyQttCtx                       * ctx,
						 MyQttOnPublish                   on_publish,
						 axlPointer                       user_data);

void        myqtt_ctx_set_on_msg                (MyQttCtx             * ctx,
						 MyQttOnMsgReceived     on_msg,
						 axlPointer             on_msg_data);

void        myqtt_ctx_set_on_header             (MyQttCtx               * ctx,
						 MyQttOnHeaderReceived    on_header,
						 axlPointer               on_header_data);

void        myqtt_ctx_set_idle_handler          (MyQttCtx                       * ctx,
						 MyQttIdleHandler                 idle_handler,
						 long                              max_idle_period,
						 axlPointer                        user_data,
						 axlPointer                        user_data2);

void        myqtt_ctx_set_on_connect            (MyQttCtx                * ctx, 
						 MyQttOnConnectHandler     on_connect, 
						 axlPointer                user_data);

void        myqtt_ctx_notify_idle               (MyQttCtx     * ctx,
						 MyQttConn    * conn);

void        myqtt_ctx_install_cleanup           (MyQttCtx * ctx,
						  axlDestroyFunc cleanup);

void        myqtt_ctx_remove_cleanup            (MyQttCtx * ctx,
						  axlDestroyFunc cleanup);

axl_bool    myqtt_ctx_ref                       (MyQttCtx  * ctx);

axl_bool    myqtt_ctx_ref2                      (MyQttCtx  * ctx, const char * who);

void        myqtt_ctx_unref                     (MyQttCtx ** ctx);

void        myqtt_ctx_unref2                    (MyQttCtx ** ctx, const char * who);

int         myqtt_ctx_ref_count                 (MyQttCtx  * ctx);

void        myqtt_ctx_free                      (MyQttCtx * ctx);

void        myqtt_ctx_free2                     (MyQttCtx * ctx, const char * who);

void        myqtt_ctx_set_on_finish        (MyQttCtx              * ctx,
					     MyQttOnFinishHandler    finish_handler,
					     axlPointer               user_data);

void        myqtt_ctx_check_on_finish      (MyQttCtx * ctx);

void        __myqtt_ctx_set_cleanup (MyQttCtx * ctx);

END_C_DECLS

#endif /* __MYQTT_CTX_H__ */
