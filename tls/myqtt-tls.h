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
#ifndef __MYQTT_TLS_H__
#define __MYQTT_TLS_H__

#include <myqtt.h>

BEGIN_C_DECLS

/** 
 * @brief Digest method provided.
 */
typedef enum {
	/** 
	 * @brief Allows to especify the DIGEST method SHA-1.
	 */
	MYQTT_SHA1 = 1,
	/** 
	 * @brief Allows to especify the DIGEST method MD5.
	 */
	MYQTT_MD5 = 2,
	/** 
	 * @internal Internal value. Do not modify.
	 */
	MYQTT_DIGEST_NUM
} MyQttDigestMethod;



/** 
 * @brief Handler definition used by the TLS profile, to allow the
 * application level to provide the function that must be executed to
 * create an (SSL_CTX *) object, used to perform the TLS activation.
 *
 * This handler is used by: 
 *  - \ref myqtt_tls_set_ctx_creation
 *  - \ref myqtt_tls_set_default_ctx_creation
 *
 * By default the MyQtt TLS implementation will use its own code to
 * create the SSL_CTX object if not provided the handler. However,
 * such code is too general, so it is recomended to provide your own
 * context creation.
 *
 * Inside this function you must configure all your stuff to tweak the
 * OpenSSL behaviour. Here is an example:
 * 
 * \code
 * axlPointer * __ctx_creation (MyQttConn * conection,
 *                              axlPointer         user_data)
 * {
 *     SSL_CTX * ctx;
 *
 *     // create the context using the TLS method (for client side)
 *     ctx = SSL_CTX_new (TLSv1_method ());
 *
 *     // configure the root CA and its directory to perform verifications
 *     if (SSL_CTX_load_verify_locations (ctx, "your-ca-file.pem", "you-ca-directory")) {
 *         // failed to configure SSL_CTX context 
 *         SSL_CTX_free (ctx);
 *         return NULL;
 *     }
 *     if (SSL_CTX_set_default_verify_paths () != 1) {
 *         // failed to configure SSL_CTX context 
 *         SSL_CTX_free (ctx);
 *         return NULL;
 *     }
 *
 *     // configure the client certificate (public key)
 *     if (SSL_CTX_use_certificate_chain_file (ctx, "your-client-certificate.pem")) {
 *         // failed to configure SSL_CTX context 
 *         SSL_CTX_free (ctx);
 *         return NULL;
 *     }
 *
 *     // configure the client private key 
 *     if (SSL_CTX_use_PrivateKey_file (ctx, "your-client-private-key.rpm", SSL_FILETYPE_PEM)) {
 *         // failed to configure SSL_CTX context 
 *         SSL_CTX_free (ctx);
 *         return NULL;
 *     }
 *
 *     // set the verification level for the client side
 *     SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, NULL);
 *     SSL_CTX_set_verify_depth(ctx, 4);
 *
 *     // our ctx is configured
 *     return ctx;
 * }
 * \endcode
 *
 * For the server side, the previous example mostly works, but you
 * must reconfigure the call to SSL_CTX_set_verify, providing
 * something like this:
 * 
 * \code
 *    SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
 * \endcode
 *
 * See OpenSSL documenation for SSL_CTX_set_verify and SSL_CTX_set_verify_depth.
 * 
 * @param connection The connection that has been requested to be
 * activated the TLS profile, for which a new SSL_CTX must be created. 
 * 
 * @param user_data An optional user pointer defined at either \ref
 * myqtt_tls_set_default_ctx_creation and \ref
 * myqtt_tls_set_ctx_creation.
 * 
 * @return You must return a newly allocated SSL_CTX or NULL if the
 * handler must signal that the TLS activation must not be performed.
 */
typedef axlPointer (* MyQttTlsCtxCreation) (MyQttConn * connection,
					    axlPointer        user_data);


/** 
 * @brief Allows to configure a post-condition function to be executed
 * to perform additional checkings.
 *
 * This handler is used by:
 * 
 *  - \ref myqtt_tls_set_post_check
 *  - \ref myqtt_tls_set_default_post_check
 *
 * The function must return axl_true to signal that checkings was
 * passed, otherwise axl_false must be returned. In such case, the
 * connection will be dropped.
 * 
 * @param connection The connection that was TLS-fixated and
 * additional checks were configured.
 * 
 * @param user_data User defined data passed to the function, defined
 * at \ref myqtt_tls_set_post_check and \ref
 * myqtt_tls_set_default_post_check.
 *
 * @param ssl The SSL object created for the process.
 * 
 * @param ctx The SSL_CTX object created for the process.
 * 
 * @return axl_true to accept the connection, otherwise, axl_false must be
 * returned.
 */
typedef axl_bool  (*MyQttTlsPostCheck) (MyQttConn * connection, 
					 axlPointer         user_data, 
					 axlPointer         ssl, 
					 axlPointer         ctx);


/** 
 * @brief Handler called when a failure is found during TLS
 * handshake. 
 *
 * The function receives the connection where the failure * found an
 * error message and a pointer configured by the user at \ref myqtt_tls_set_failure_handler.
 *
 * @param connection The connection where the failure was found.
 *
 * @param error_message The error message describing the problem found.
 *
 * @param user_data Optional user defined pointer.
 *
 * To get particular SSL info, you can use the following code inside the handler:
 * \code
 * // error variables
 * char          log_buffer [512];
 * unsigned long err;
 *
 * // show errors found
 * while ((err = ERR_get_error()) != 0) {
 *     ERR_error_string_n (err, log_buffer, sizeof (log_buffer));
 *     printf ("tls stack: %s (find reason(code) at openssl/ssl.h)", log_buffer);
 * }
 * \endcode
 * 
 *
 */
typedef void      (*MyQttTlsFailureHandler) (MyQttConn   * connection,
					     const char  * error_message,
					     axlPointer    user_data);


axl_bool           myqtt_tls_init                       (MyQttCtx             * ctx);

void               myqtt_tls_set_ctx_creation           (MyQttConn            * connection,
							 MyQttTlsCtxCreation    ctx_creation, 
							 axlPointer             user_data);

void               myqtt_tls_set_default_ctx_creation   (MyQttCtx              * ctx,
							  MyQttTlsCtxCreation    ctx_creation, 
							  axlPointer             user_data);

void               myqtt_tls_set_post_check             (MyQttConn             * connection,
							  MyQttTlsPostCheck      post_check,
							  axlPointer             user_data);

void               myqtt_tls_set_default_post_check     (MyQttCtx              * ctx, 
							  MyQttTlsPostCheck      post_check,
							  axlPointer             user_data);

void               myqtt_tls_set_failure_handler        (MyQttCtx                * ctx,
							  MyQttTlsFailureHandler   failure_handler,
							  axlPointer               user_data);

axl_bool           myqtt_tls_verify_cert                (MyQttConn * connection);

axlPointer         myqtt_tls_get_ssl_object             (MyQttConn * connection);

char             * myqtt_tls_get_peer_ssl_digest        (MyQttConn   * connection, 
							 MyQttDigestMethod   method);

char             * myqtt_tls_get_digest                 (MyQttDigestMethod   method,
							 const char         * string);

char             * myqtt_tls_get_digest_sized           (MyQttDigestMethod    method,
							 const char         * content,
							 int                  content_size);

void               myqtt_tls_cleanup                    (MyQttCtx * ctx);

#endif
