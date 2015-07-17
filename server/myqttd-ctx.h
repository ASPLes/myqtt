/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2015 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation version 2.0 of the
 *  License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is GPL software 
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
#ifndef __MYQTTD_CTX_H__
#define __MYQTTD_CTX_H__

#include <myqttd.h>

/**
 * \addtogroup myqttd_ctx
 * @{
 */

MyQttdCtx * myqttd_ctx_new           (void);

void        myqttd_ctx_set_myqtt_ctx (MyQttdCtx * ctx, 
				      MyQttCtx  * myqtt_ctx);

/** 
 * @brief Allows to get the myqtt context associated to the
 * myqttd context provided.
 * 
 * @param _myqtt_ctx The myqttd context which is required to return the
 * myqtt context associated.
 * 
 * @return A reference to the myqtt context associated.
 */
#define MYQTTD_MYQTT_CTX(_myqtt_ctx) (myqttd_ctx_get_myqtt_ctx (_myqtt_ctx))

void            myqttd_ctx_add_on_publish (MyQttdCtx       * ctx, 
					   MyQttdOnPublish   on_publish, 
					   axlPointer        user_data); 

void            myqttd_ctx_add_listener_activator (MyQttdCtx               * ctx,
						   const char              * proto,
						   MyQttdListenerActivator   listener_activator,
						   axlPointer                user_data);
						   

MyQttCtx     * myqttd_ctx_get_myqtt_ctx (MyQttdCtx * ctx);

void            myqttd_ctx_set_data       (MyQttdCtx * ctx,
					       const char    * key,
					       axlPointer      data);

void            myqttd_ctx_set_data_full  (MyQttdCtx * ctx,
					       const char    * key,
					       axlPointer      data,
					       axlDestroyFunc  key_destroy,
					       axlDestroyFunc  value_destroy);

axlPointer      myqttd_ctx_get_data       (MyQttdCtx * ctx,
					       const char    * key);

void            myqttd_ctx_wait           (MyQttdCtx * ctx,
					       long microseconds);

axl_bool        myqttd_ctx_is_child       (MyQttdCtx * ctx);

void            myqttd_ctx_free           (MyQttdCtx * ctx);

/* @} */

#endif
