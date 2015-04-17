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

#ifndef __MYQTT_CONNECTION_PRIVATE_H__
#define __MYQTT_CONNECTION_PRIVATE_H__

/** 
 * @internal
 * @brief Internal MyQttConnection representation.
 */
struct _MyQttConn {
	/** 
	 * @brief MyQtt context where the connection was created.
	 */
	MyQttCtx  * ctx;

	/** 
	 * @brief MyQtt Connection unique identifier.
	 */
	int          id;

	/** 
	 * @brief MQTT client identifier.
	 */
	char * client_identifier;

	/* qos */
	char      * will_topic;
	char      * will_msg;
	MyQttQos    will_qos;

	char * username;
	char * password;

	/* serverName: indication available only when using MQTT under
	   certain protocols (TLS and WebSocket) */
	char * serverName;

	/** 
	 * @brief Host name this connection is actually connected to.
	 */
	char       * host;
	char       * host_ip;

	/** 
	 * @brief Port this connection is actually connected to.
	 */
	char       * port;

	/** 
	 * @brief Contains the local address that is used by this connection.
	 */
	char       * local_addr;
	/** 
	 * @brief Contains the local port that is used by this connection.
	 */
	char       * local_port;

	/** 
	 * @brief Allows to hold and report current connection status.
	 */
	axl_bool     is_connected;

	/** 
	 * @brief Records if the provided connection has session
	 * enabled (clean_session=axl_false).
	 */
	axl_bool     clean_session;
	/** 
	 * @brief Keeps track about storage initilization.
	 */
	int          myqtt_storage_init;

	/** 
	 * @brief Internal variable to track if there is already a thread flushing 
	 */
	axl_bool     flushing;

	/** 
	 * @internal Variable used to track if a
	 * myqtt_connection_close process is already activated.
	 */
	axl_bool     close_called;

	/** 
	 * @brief Underlying transport descriptor.
	 * This is actually the network descriptor used to send and
	 * received data.
	 */
	MYQTT_SOCKET  session;
	/** 
	 * @brief Last error reported by connection operation.
	 */
	MyQttConnAckTypes  last_err;

	/** 
	 * @brief Allows to configure if the given session (actually
	 * the socket) associated to the given connection should be
	 * closed once __myqtt_connection_set_not_connected function.
	 */
	axl_bool       close_session;

	/** 
	 * @internal Internal reference to connection especific
	 * options.
	 */
	MyQttConnOpts    * opts;

	/** 
	 * Hash to hold miscellaneous data, sometimes used by the
	 * MyQtt Library itself, but also exposed to be used by the
	 * MyQtt Library programmers through the functions:
	 *   - \ref myqtt_connection_get_data 
	 *   - \ref myqtt_connection_set_data
	 */
	MyQttHash * data;

	/** 
	 * @brief The ref_count.
	 *
	 * This enable a myqtt connection to keep track of the
	 * reference counting.  The reference counting is controlled
	 * thought the myqtt_connection_ref and
	 * myqtt_connection_unref.
	 * 
	 */
	int  ref_count;
	
	/** 
	 * @brief The ref_mutex
	 * This mutex is used at myqtt_connection_ref and
	 * myqtt_connection_unref to avoid race conditions especially
	 * at myqtt_connection_unref. Because several threads can
	 * execute at the same time this unref function this mutex
	 * ensure only one thread will execute the myqtt_connection_free.
	 * 
	 */
	MyQttMutex  ref_mutex;

	/** 
	 * @brief The op_mutex
	 * This mutex allows to avoid race condition on operating with
	 * the same connection from different threads.
	 */
	MyQttMutex op_mutex;
	
	/** 
	 * @brief The handlers mutex
	 *
	 * This mutex is used to secure code sections destined to
	 * handle handlers stored associated to the connection.
	 */
	MyQttMutex handlers_mutex;

	/** 
	 * @brief The peer role
	 * 
	 * This is used to know current connection role.	
	 */
	MyQttPeerRole role;

	/** 
	 * @brief Writer function used by the MyQtt Library to actually send data.
	 */
	MyQttSend    send;

	/** 
	 * @brief Writer function used by the MyQtt Library to actually received data
	 */
	MyQttReceive receive;

	/** 
	 * @brief On close handler, extended version.
	 */
	axlList * on_close_full;

	/** 
	 * @brief Mutex used to open the pending errors list.
	 */
	MyQttMutex pending_errors_mutex;

	/** 
	 * @internal Reference to implement connection I/O block.
	 */
	axl_bool                is_blocked;

	/** 
	 * @internal Value to track connection activity.
	 */
	long                    last_idle_stamp;
	int                     keep_alive;

	/** 
	 * @internal Value to track now many bytes has being received
	 * and sent on this connection.
	 */
	long                    bytes_received;
	long                    bytes_sent;

	/** 
	 * @internal Value that makes the connection to be
	 * unwatched from the reader process.
	 */
	axl_bool                reader_unwatch;

	/** 
	 * @internal Value to signal initial accept stage associated
	 * to a connection in the middle of the greetings.
	 */
	axl_bool                initial_accept;
	axl_bool                transport_detected;
	axl_bool                connect_received;

	/** 
	 * @internal Reference to a line that wasn't totally read when
	 * call myqtt_msg_readline.
	 */ 
	char                  * pending_line;

	/** 
	 * @internal Pointer used to store the buffer that is holding
	 * the content of the next msg.
	 */
	unsigned char             * buffer;
	
	/** 
	 * @internal Pointer to the last msg being read at the
	 * connection.
	 */
	MyQttMsg                  * last_msg;

	/** 
	 * @internal Variable that is used by myqtt_msg_get_next to
	 * track status for partial reads.
	 */
	int                          remaining_bytes;
	int                          bytes_read;
	int                          no_data_opers;

	/** reference to the user land hook pointer **/
	axlPointer                   hook;

	/** reference to the transport used by the library */
	MyQttNetTransport           transport;

	/** 
	 * @internal Reference to the list of sent pkg ids that are
	 * waiting for publish confirmation.
	 */
	axlList                   * sent_pkgids;

	/** 
	 * @internal Reference to keep track about wait replies ( pkg
	 * id => async queues) where the async queue are used to push
	 * replies received for a certain pkd id.
	 */
	axlHash                   * wait_replies;

	/*** subscriptions ***/
	axlHash                   * subs;
	axlHash                   * wild_subs;

	/*** on message support */
	MyQttOnMsgReceived          on_msg;
	axlPointer                  on_msg_data;

	/*** pingreq support ****/
	MyQttAsyncQueue           * ping_resp_queue;

	/*** pre read support ***/
	MyQttPreRead                preread_handler;
	axlPointer                  preread_user_data;

	/* tls/ssl support */
	axlPointer      ssl_ctx;
	axlPointer      ssl;
	axl_bool        tls_on;
	axl_bool        pending_ssl_accept;
	char          * certificate;
	char          * private_key;
	char          * chain_certificate;
};

struct _MyQttConnOpts {
	/** 
	 * @internal The following is an indication to avoid releasing this object if reuse == axl_true.
	 */
	axl_bool        reuse;

	/* support for auth */
	axl_bool        use_auth;
	char          * username;
	char          * password;

	/* will topic and will message configuration */
	MyQttQos        will_qos;
	char          * will_topic;
	char          * will_message;
	int             will_retain;

	/** ssl/tls support **/
	/* What ssl protocol should be used */
	int             ssl_protocol;

	/* SSL options */
	char * certificate;
	char * private_key;
	char * chain_certificate;
	char * ca_certificate;

	axl_bool  disable_ssl_verify;
};

axl_bool               myqtt_conn_ref_internal           (MyQttConn   * conn, 
							  const char  * who,
							  axl_bool      check_ref);

void                   myqtt_conn_set_initial_accept     (MyQttConn   * conn,
							  axl_bool      status);

void                   myqtt_conn_report_and_close       (MyQttConn   * conn, 
							  const char  * msg);

axl_bool               __myqtt_conn_pub_send_and_handle_reply (MyQttCtx      * ctx, 
							       MyQttConn     * conn, 
							       int             packet_id, 
							       MyQttQos        qos, 
							       axlPointer      handle, 
							       int             wait_publish, 
							       unsigned char * msg, 
							       int             size);

MyQttConn            * myqtt_conn_new_full_common        (MyQttCtx             * ctx,
							  const char           * client_identifier,
							  axl_bool               clean_session,
							  int                    keep_alive,
							  const char           * host, 
							  const char           * port,
							  MyQttSessionSetup      setup_handler,
							  axlPointer             setup_user_data,
							  MyQttConnNew           on_connected, 
							  MyQttNetTransport      transport,
							  MyQttConnOpts        * opts,
							  axlPointer             user_data);

MyQttNetTransport      __myqtt_conn_detect_transport (MyQttCtx * ctx, const char * host);

#endif /* __MYQTT_CONNECTION_PRIVATE_H__ */
