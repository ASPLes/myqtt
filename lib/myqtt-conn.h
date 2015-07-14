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

#ifndef __MYQTT_CONN_H__
#define __MYQTT_CONN_H__

#include <myqtt.h>

/** 
 * \addtogroup myqtt_conn
 * @{
 */

/** 
 * @brief Allows to get the context associated to the provided
 * conn, logging a more verbose message if the context is null
 * or the conn provided is null.
 *
 * For myqtt, the context object is central for its
 * function. Providing a function that warns that a null context is
 * returned is a key to find bugs, since no proper result could be
 * expected if no context is provided (\ref MyQttCtx). 
 *
 * @param c The conn that is required to return the context
 * associated.
 */
#define CONN_CTX(c) myqtt_conn_get_ctx(c)

MyQttConn  * myqtt_conn_new                    (MyQttCtx        * ctx,
						const char      * client_identifier,
						axl_bool          clean_session,
						int               keep_alive,
						const char      * host, 
						const char      * port,
						MyQttConnOpts   * opts,
						MyQttConnNew      on_connected, 
						axlPointer        user_data);

MyQttConn  * myqtt_conn_new6                   (MyQttCtx       * ctx,
						const char     * client_identifier,
						axl_bool         clean_session,
						int              keep_alive,
						const char     * host, 
						const char     * port,
						MyQttConnOpts  * opts,
						MyQttConnNew     on_connected, 
						axlPointer       user_data);

axl_bool            myqtt_conn_reconnect       (MyQttConn * conn,
						MyQttConnNew on_connected,
						axlPointer user_data);

void                myqtt_conn_send_connect_reply (MyQttConn * conn, MyQttConnAckTypes response);

MyQttConnAckTypes   myqtt_conn_get_last_err    (MyQttConn * conn);

const char        * myqtt_conn_get_code_to_err (MyQttConnAckTypes code);

axl_bool            myqtt_conn_pub             (MyQttConn           * conn,
						const char          * topic_name,
						const axlPointer      app_message,
						int                   app_message_size,
						MyQttQos              qos,
						axl_bool              retain,
						int                   wait_publish);

axl_bool            myqtt_conn_offline_pub     (MyQttCtx            * ctx,
						const char          * client_identifier,
						const char          * topic_name,
						const axlPointer      app_message,
						int                   app_message_size,
						MyQttQos              qos,
						axl_bool              retain);

axl_bool            myqtt_conn_sub             (MyQttConn           * conn,
						int                   wait_sub,
						const char          * topic_filter,
						MyQttQos              qos,
						int                 * subs_result);

axl_bool            myqtt_conn_unsub           (MyQttConn           * conn,
						const char          * topic_filter,
						int                   wait_unsub);

axl_bool            myqtt_conn_ping            (MyQttConn           * conn,
						int                   wait_pingresp);

axl_bool            myqtt_conn_close           (MyQttConn  * conn);

MyQttConnOpts     * myqtt_conn_opts_new (void);

void                myqtt_conn_opts_set_auth (MyQttConnOpts * opts, 
					      const    char * username, 
					      const    char * password);

void                myqtt_conn_opts_set_reuse (MyQttConnOpts * opts,
					       axl_bool        reuse);

void                myqtt_conn_opts_set_will (MyQttConnOpts  * opts,
					      MyQttQos         will_qos,
					      const char     * will_topic,
					      const char     * will_message,
					      axl_bool         will_retain);

void                myqtt_conn_opts_set_reconnect (MyQttConnOpts * opts,
						   axl_bool        enable_reconnect);

void                myqtt_conn_opts_set_init_session_setup_ptr (MyQttConnOpts               * opts,
								MyQttInitSessionSetupPtr      init_session_setup_ptr,
								axlPointer                    user_data,
								axlPointer                    user_data2,
								axlPointer                    user_data3);

void                myqtt_conn_opts_free     (MyQttConnOpts  * opts);

MYQTT_SOCKET       myqtt_conn_sock_connect     (MyQttCtx    * ctx,
						const char  * host,
						const char  * port,
						int         * timeout,
						axlError   ** error);

MYQTT_SOCKET       myqtt_conn_sock_connect_common    (MyQttCtx            * ctx,
						      const char           * host,
						      const char           * port,
						      int                  * timeout,
						      MyQttNetTransport     transport,
						      axlError            ** error);

axl_bool            myqtt_conn_ref                    (MyQttConn * conn,
							const char       * who);

axl_bool            myqtt_conn_uncheck_ref            (MyQttConn * conn);

void                myqtt_conn_unref                  (MyQttConn * conn,
							const char       * who);

int                 myqtt_conn_ref_count              (MyQttConn * conn);

MyQttConn         * myqtt_conn_new_empty              (MyQttCtx        * ctx,
						       MYQTT_SOCKET      socket,
						       MyQttPeerRole     role);

MyQttConn         * myqtt_conn_new_empty_from_conn (MyQttCtx        * ctx,
						    MYQTT_SOCKET      socket, 
						    MyQttConn * __conn,
						    MyQttPeerRole     role);

axl_bool            myqtt_conn_set_socket                (MyQttConn * conn,
							  MYQTT_SOCKET      socket,
							  const char       * real_host,
							  const char       * real_port);

void                myqtt_conn_timeout                (MyQttCtx        * ctx,
						       long               microseconds_to_wait);
void                myqtt_conn_connect_timeout        (MyQttCtx        * ctx,
						       long               microseconds_to_wait);

long                myqtt_conn_get_timeout            (MyQttCtx        * ctx);
long                myqtt_conn_get_connect_timeout    (MyQttCtx        * ctx);

axl_bool            myqtt_conn_is_ok                  (MyQttConn * conn, 
							axl_bool   free_on_fail);

void                __myqtt_conn_shutdown_and_record_error (MyQttConn * conn,
							     MyQttStatus       status,
							     const char       * message,
							     ...);

void                myqtt_conn_free                   (MyQttConn * conn);

MYQTT_SOCKET        myqtt_conn_get_socket             (MyQttConn * conn);

void                myqtt_conn_set_close_socket       (MyQttConn * conn,
							      axl_bool           action);

const char        * myqtt_conn_get_host               (MyQttConn * conn);

const char        * myqtt_conn_get_host_ip            (MyQttConn * conn);

const char        * myqtt_conn_get_port               (MyQttConn * conn);

const char        * myqtt_conn_get_local_addr         (MyQttConn * conn);

const char        * myqtt_conn_get_local_port         (MyQttConn * conn);

void                myqtt_conn_set_host_and_port      (MyQttConn   * conn, 
						       const char  * host,
						       const char  * port,
						       const char  * host_ip);

int                 myqtt_conn_get_id                 (MyQttConn * conn);

const char *        myqtt_conn_get_username           (MyQttConn * conn);

const char *        myqtt_conn_get_password           (MyQttConn * conn);

const char *        myqtt_conn_get_client_id          (MyQttConn * conn);

axl_bool            myqtt_conn_set_blocking_socket    (MyQttConn * conn);

axl_bool            myqtt_conn_set_nonblocking_socket (MyQttConn * conn);

axl_bool            myqtt_conn_set_sock_tcp_nodelay   (MYQTT_SOCKET socket,
						       axl_bool      enable);

axl_bool            myqtt_conn_set_sock_block         (MYQTT_SOCKET socket,
							      axl_bool      enable);

void                myqtt_conn_set_data               (MyQttConn * conn,
						       const char       * key,
						       axlPointer         value);

void                myqtt_conn_set_data_full          (MyQttConn * conn,
						       char             * key,
						       axlPointer         value,
						       axlDestroyFunc     key_destroy,
						       axlDestroyFunc     value_destroy);

void                myqtt_conn_set_hook               (MyQttConn * conn,
							axlPointer         ptr);

axlPointer          myqtt_conn_get_hook               (MyQttConn * conn);

void                myqtt_conn_delete_key_data        (MyQttConn * conn,
							const char       * key);


axlPointer          myqtt_conn_get_data               (MyQttConn * conn,
							      const char       * key);

MyQttPeerRole       myqtt_conn_get_role               (MyQttConn * conn);

MyQttConn         * myqtt_conn_get_listener           (MyQttConn * conn);

const char        * myqtt_conn_get_server_name        (MyQttConn * conn);

void                myqtt_conn_set_server_name        (MyQttConn  * conn, 
						       const char * serverName);

MyQttCtx          * myqtt_conn_get_ctx                (MyQttConn * conn);

MyQttSend              myqtt_conn_set_send_handler    (MyQttConn * conn,
						       MyQttSend  send_handler);

MyQttReceive           myqtt_conn_set_receive_handler (MyQttConn * conn,
						       MyQttReceive receive_handler);

void                   myqtt_conn_set_default_io_handler (MyQttConn * conn);

void                   myqtt_conn_set_on_msg         (MyQttConn * conn,
						      MyQttOnMsgReceived on_msg,
						      axlPointer     on_msg_data);

void                   myqtt_conn_set_on_close       (MyQttConn         * conn,
						      axl_bool            insert_last,
						      MyQttConnOnClose    on_close_handler,
						      axlPointer          data);

void                   myqtt_conn_set_on_msg_sent    (MyQttConn         * conn,
						      MyQttOnMsgSent      on_msg_sent,
						      axlPointer          on_msg_sent_data);

void                   myqtt_conn_set_on_reconnect   (MyQttConn         * conn,
						      MyQttOnReconnect    on_reconnect,
						      axlPointer          on_reconnect_data);

MyQttMsg *             myqtt_conn_get_next (MyQttConn * conn, long timeout);

axl_bool            myqtt_conn_remove_on_close       (MyQttConn              * conn, 
						      MyQttConnOnClose         on_close_handler,
						      axlPointer               data);

int                 myqtt_conn_invoke_receive         (MyQttConn        * conn,
						       unsigned char    * buffer,
						       int                buffer_len);

int                 myqtt_conn_invoke_send            (MyQttConn             * conn,
							const unsigned char  * buffer,
							int                    buffer_len);

void                myqtt_conn_sanity_socket_check        (MyQttCtx * ctx, axl_bool      enable);

void                myqtt_conn_shutdown                   (MyQttConn * conn);

void                myqtt_conn_shutdown_socket            (MyQttConn * conn);

void                myqtt_conn_set_receive_stamp            (MyQttConn * conn, long bytes_received, long bytes_sent);

void                myqtt_conn_get_receive_stamp            (MyQttConn * conn, long * bytes_received, long * bytes_sent, long * last_idle_stamp);

void                myqtt_conn_check_idle_status            (MyQttConn * conn, MyQttCtx * ctx, long time_stamp);

void                myqtt_conn_block                        (MyQttConn * conn,
								    axl_bool           enable);

axl_bool            myqtt_conn_is_blocked                   (MyQttConn  * conn);

axl_bool            myqtt_conn_half_opened                  (MyQttConn  * conn);

/** 
 * @internal
 * Do not use the following functions, internal MyQtt Library purposes.
 */

void                myqtt_conn_init                   (MyQttCtx        * ctx);

void                myqtt_conn_cleanup                (MyQttCtx        * ctx);

void                __myqtt_conn_set_not_connected    (MyQttConn * conn, 
							const char       * message,
							MyQttStatus       status);

int                 myqtt_conn_do_a_sending_round     (MyQttConn * conn);

int                 myqtt_conn_get_mss                (MyQttConn * conn);

axl_bool            myqtt_conn_check_socket_limit     (MyQttCtx        * ctx, 
							MYQTT_SOCKET      socket);

#endif

/* @} */


