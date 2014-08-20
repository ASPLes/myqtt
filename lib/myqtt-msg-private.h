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

#ifndef __MYQTT_MSG_PRIVATE__
#define __MYQTT_MSG_PRIVATE__

struct _MyQttMsg {
	/**
	 * Context where the msg was created.
	 */ 
	MyQttCtx       * ctx;

	/** 
	 * Msg unique identifier. Every msg read have a different
	 * msg id. This is used to track down msgs that are
	 * allocated and deallocated. This message id is not the packet id.
	 */
	int               id;
	
	/* the payload message, without including header and remaining length */
	const unsigned char *     payload;

	/* holds size of the payload (it just includes variable header
	 * and payload) */
	int                  size;

	MyQttMutex           mutex;

	/* real reference to the memory allocated, having all the
	 * msg received this is used to avoid double allocating memory
	 * to receive the content and memory to place the content. See
	 * myqtt_msg_get_next for more information. */
	axlPointer           buffer;

	/* msg reference counting */
	int                  ref_count;

	/* message type */
	MyQttMsgType         type;

	/* message qos */
	MyQttQos             qos;

	/* dup flag */
	axl_bool             dup;

	/* packet id if defined */
	int                  packet_id;

	/* reference to app message */
	const unsigned char * app_message;
	int                   app_message_size;

	/* reference to the topic name in the case this is a PUBLISH
	 * message */
	char                * topic_name;
};

#endif
