/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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
#ifndef __MYQTTD_HANDLERS_H__

/* include header */
#include <myqttd.h>

/** 
 * \defgroup myqttd_handlers MyQttd Handlers : Function handler definitions used by the API
 */

/** 
 * \addtogroup myqttd_handlers
 * @{
 */

/** 
 * @brief Handler definition used by process calling to \ref
 * myqttd_signal_install which has visibility to access to the
 * right \ref MyQttdCtx object. This handler must call to \ref
 * myqttd_signal_received in the case and call to
 * myqttd_signal_reconf in the case of SIGHUP received.
 *
 * @param signal The signal that was received.
 */
typedef void (*MyQttdSignalHandler) (int signal);

/** 
 * @brief Handler definition for the set of functions that are able to
 * filter connections to be not broadcasted by the myqttd conn mgr
 * module.
 *
 * The function must return true to filter the connection and avoid
 * sending the message broadcasted.
 * 
 * @param conn The connection which is asked to be filtered or not.
 *
 * @param user_data User defined data associated to the filter
 * configuration.
 * 
 * @return true to filter the connection, otherwise return false.
 */
typedef int  (*MyQttdConnMgrFilter) (MyQttConn * conn, axlPointer user_data);

/** 
 * @brief Handler definition used by myqttd_loop_set_read_handler
 * to notify that the descriptor is ready to be read (either because
 * it has data or because it was closed).
 *
 * @param loop The loop wher the notification was found.
 * @param ctx The MyQttd context where the loop is running.
 * @param descriptor The descriptor that is ready to be read.
 * @param ptr User defined pointer defined at \ref myqttd_loop_set_read_handler and passed to this handler.
 * @param ptr2 User defined pointer defined at \ref myqttd_loop_set_read_handler and passed to this handler.
 *
 * @return The function return axl_true in the case the read operation
 * was completed without problem. Otherwise axl_false is returned
 * indicating that the myqttd loop engine should close the
 * descriptor.
 */
typedef axl_bool (*MyQttdLoopOnRead) (MyQttdLoop   * loop, 
				      MyQttdCtx    * ctx,
				      int            descriptor, 
				      axlPointer     ptr, 
				      axlPointer     ptr2);

/** 
 * @brief Handler used to define the set of functions that can load an
 * user's backend for a given connection and/or myqtt domain
 *
 * @param ctx The context where the operation takes place
 *
 * @param domain The domain where this users' backend is being requested to load.
 *
 * @param conn The connection that triggered loading this database.
 *
 * @param path The Path to the users directory for a particular domain
 * where the configuration and database files are found. This value is
 * the <b>users-db=</b> attribute declarated inside the
 * <b>;lt;domain</b> node, pointing to the directory configured by the
 * system administrator or any other particular setting required by
 * the auth backend. For example, <b>mod-auth-xml</b> module requires
 * this value to point to the directory where the <b>users.xml</b>
 * file is stored. In the case of <b>mod-auth-mysql</b>, this value is
 * ignored and database settings are ignored but taken from
 * $sysconfdir/mysql/mysql.xml file (\ref myqttd_sysconfdir).
 *
 * @return The function must return NULL or a valid reference to the
 * database backend to be used by the engine to complete users'
 * operations.
 */
typedef axlPointer (*MyQttdUsersLoadDb) (MyQttdCtx * ctx, MyQttdDomain * domain, MyQttConn * conn, const char * path);

/** 
 * @brief Handler used to define the set of functions that allows to
 * check if the provided user described by username+clientid or any
 * combination of them exists in the provided backend.
 *
 * @param ctx The context where the operation takes place.
 *
 * @param domain The domain where the operation is taking place. 
 *
 * @param domain_selected External indication to signal auth backend
 * that the domain was already selected or not. Once a domain is
 * <b>selected</b>, it is assumed that any auth operation in this
 * domain with the given username/client_id/password will be final: no
 * other domain will be checked. This is important in the sense that
 * MyQttd engine try to find the domain by various methods, and one of
 * them is by checking auth over all domains. However, because in that
 * context the search for domain is done by using auth operations,
 * then the auth backend may want to disable certain mechanism (like anonymous login) in the case <b>domain_selected == axl_false</b>.
 *
 *
 * @param users The users database backend where the operation is taking place.
 *
 * @param conn The connection where the auth operation is taking
 * place. This reference may not be defined because it is run by a
 * tool in offline mode.
 *
 * @param backend Reference to the backend where the exists operation
 * will be implemented.
 *
 * @param client_id Optionally the client id being checked (you must provided username+client_id or just client_id or user_name).
 *
 * @param user_Name Optionally the user_name being checked (you must provided username+client_id or just client_id or user_name).
 *
 * @return The function returns axl_true if the user_name+client_id,
 * user_name or client_id combination is found. The function will only
 * check for just client_id when user_name is NULL and the same
 * happens for just checking user_name (only done when client_id is
 * NULL).
 */
typedef axl_bool   (*MyQttdUsersExists) (MyQttdCtx    * ctx,
					 MyQttdDomain * domain,
					 axl_bool       domain_selected,
					 MyQttdUsers  * users,
					 MyQttConn    * conn,
					 axlPointer     backend, 
					 const char   * client_id, 
					 const char   * user_name);

/** 
 * @brief Handler used to define the set of functions that allows to
 * implement user authentication with provided user described by
 * username+clientid or any combination of them exists in the provided
 * backend.
 *
 * @param ctx The context where the operation takes place.
 *
 * @param domain The domain where the operation is taking place. 
 *
 * @param domain_selected External indication to signal auth backend
 * that the domain was already selected or not. Once a domain is
 * <b>selected</b>, it is assumed that any auth operation in this
 * domain with the given username/client_id/password will be final: no
 * other domain will be checked. This is important in the sense that
 * MyQttd engine try to find the domain by various methods, and one of
 * them is by checking auth over all domains. However, because in that
 * context the search for domain is done by using auth operations,
 * then the auth backend may want to disable certain mechanism (like anonymous login) in the case <b>domain_selected == axl_false</b>.
 *
 * @param users The users database backend where the operation is taking place.
 *
 * @param conn The connection where the auth operation is taking
 * place. This reference may not be defined because it is run by a
 * tool in offline mode.
 *
 * @param backend Reference to the backend where the auth operation
 * will be implemented.
 *
 * @param client_id Optionally the client id being checked (you must
 * provided username+client_id or just client_id or user_name).
 *
 * @param user_Name Optionally the user_name being checked (you must
 * provided username+client_id or just client_id or user_name).
 *
 * @return The function returns axl_true if the user_name+client_id
 * and password matches, or user_name+password combination is
 * found. Othewise, axl_false is returned.
 */
typedef axl_bool   (*MyQttdUsersAuthUser) (MyQttdCtx    * ctx,
					   MyQttdDomain * domain,
					   axl_bool       domain_selected,
					   MyQttdUsers  * users,
					   MyQttConn    * conn, axlPointer backend, 
					   const char   * client_id, const char * user_name, 
					   const char   * password);

/** 
 * @brief Handler used to define the set of functions that unaloads an
 * user's backend.
 *
 * @param ctx The context where the operation takes place
 *
 * @param backend Reference to the backend to be released and closed.
 *
 */
typedef void (*MyQttdUsersUnloadDb) (MyQttdCtx * ctx, axlPointer backend);

/** 
 * @brief Async notification handler that gets called once a PUBLISH
 * message is received on a listener connection. This handler allows
 * to control message publication to currently registered connection
 * with general options and/or to implement especific actions upon
 * message reception.
 *
 * It's important to note that this handler is called before any
 * publish/relay operation takes place. 
 *
 * This handler is used by:
 *
 * - \ref myqttd_ctx_add_on_publish
 *
 * @param ctx The MyQttdCtx context (not the MyQttCtx) where the operation is taking place.
 *
 * @param domain The MyQttdDomain  domain where the operation is taking place.
 *
 * @param myqtt_ctx The context where the operation takes place (the
 * MyQttCtx engine started for this domain).
 *
 * @param conn The connection where the PUBLISH message was received.
 *
 * @param msg The message received. The handler must not release message received. It can acquire references to it. 
 *
 * @param user_data User defined pointer passed in into the function once it gets called.
 * 
 * @return The function must report the publish code to the engine so it can act upon it.
 *
 */
typedef MyQttPublishCodes (*MyQttdOnPublish) (MyQttdCtx * ctx,       MyQttdDomain * domain, 
					      MyQttCtx  * myqtt_ctx, MyQttConn    * conn, 
					      MyQttMsg  * msg,       axlPointer     user_data);

/** 
 * @brief Handler definition for the set of functions that allows
 * MyQttd server to startup listeners.
 *
 * This handlers are used by the myqttd engine to star listeners
 * according to the protocol declared or defaults values. See \ref myqttd_ctx_add_listener_activator
 *
 * @param ctx The context where the operation is taking place 
 *
 * @param my_ctx The MyQttCtx context where the operation is taking place.
 *
 * @param port_node A reference to the xml node in the configuration
 * file. This is useful in the case the handler needs additional
 * information are is configured in the node.
 *
 * @param bind_addr Declared name value at the configuration in the xml
 * node. This might not be defined.
 *
 * @param port Declared port value at the configuration in the xml
 * node. This is always defined.
 *
 * @param user_data User defined pointer received on this handler and
 * defined at \ref myqttd_ctx_add_listener_activator.
 */
typedef MyQttConn * (*MyQttdListenerActivator) (MyQttdCtx  * ctx, 
						MyQttCtx   * my_ctx, 
						axlNode    * port_node, 
						const char * bind_addr, 
						const char * port, 
						axlPointer   user_data);


#endif

/**
 * @}
 */
