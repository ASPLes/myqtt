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
#include <myqtt-tls.h>

#define LOG_DOMAIN "myqtt-tls"

#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/* some keys to store creation handlers and its associate data */
#define CTX_CREATION      "tls:ctx-creation"
#define CTX_CREATION_DATA "tls:ctx-creation:data"
#define POST_CHECK        "tls:post-checks"
#define POST_CHECK_DATA   "tls:post-checks:data"
#define TLS_CTX           "tls:ctx"

/**
 * @internal Function that dumps all errors found on current ssl context.
 */
int myqtt_tls_log_ssl (MyQttCtx * ctx)
{
	char          log_buffer [512];
	unsigned long err;
	int           error_position;
	int           aux_position;
	
	while ((err = ERR_get_error()) != 0) {
		ERR_error_string_n (err, log_buffer, sizeof (log_buffer));
		myqtt_log (MYQTT_LEVEL_CRITICAL, "tls stack: %s (find reason(code) at openssl/ssl.h)", log_buffer);

		/* find error code position */
		error_position = 0;
		while (log_buffer[error_position] != ':' && log_buffer[error_position] != 0 && error_position < 511)
			error_position++;
		error_position++;
		aux_position = error_position;
		while (log_buffer[aux_position] != 0) {
			if (log_buffer[aux_position] == ':') {
				log_buffer[aux_position] = 0;
				break;
			}
			aux_position++;
		} /* end while */
		myqtt_log (MYQTT_LEVEL_CRITICAL, "    details, run: openssl errstr %s", log_buffer + error_position);
	}
	
	return (0);
}


typedef struct _MyQttTlsCtx {

	/* @internal Internal default handlers used to define the TLS
	 * profile support. */
	MyQttTlsCertificateFileLocator    tls_certificate_handler;
	MyQttTlsPrivateKeyFileLocator     tls_private_key_handler;

	/* default ctx creation */
	MyQttTlsCtxCreation               tls_default_ctx_creation;
	axlPointer                        tls_default_ctx_creation_user_data;

	/* default post check */
	MyQttTlsPostCheck                 tls_default_post_check;
	axlPointer                        tls_default_post_check_user_data;

	/* failure handler */
	MyQttTlsFailureHandler            failure_handler;
	axlPointer                        failure_handler_user_data;

} MyQttTlsCtx;

/** 
 * @internal Calls to the failure handler if it is defined with the
 * provided error message.
 */
void myqtt_tls_notify_failure_handler (MyQttCtx * ctx, 
					MyQttConn * conn, 
					const char * error_message)
{
	MyQttTlsCtx * tls_ctx;

	/* check parameters received */
	if (ctx == NULL)
		return;

	/* check if the tls ctx was created */
	tls_ctx = myqtt_ctx_get_data (ctx, TLS_CTX);
	if (tls_ctx == NULL)  {
		/* dump stack */
		myqtt_tls_log_ssl (ctx);
		return;
	}

	/* configure the handler */
	if (tls_ctx->failure_handler) 
		tls_ctx->failure_handler (conn, error_message, tls_ctx->failure_handler_user_data);
	else {
		/* dump stack if no failure handler */
		myqtt_tls_log_ssl (ctx);
	}
	return;
}



/** 
 * @brief Initialize TLS library.
 * 
 * All client applications using TLS profile must call to this
 * function in order to ensure TLS profile engine can be used.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @return axl_true if the TLS profile was initialized. Otherwise
 * axl_false is returned. If the function returns axl_false, TLS is
 * not available and any call to operate with the TLS api will fail.
 */
axl_bool      myqtt_tls_init (MyQttCtx * ctx)
{
	MyQttTlsCtx * tls_ctx;

	/* check context received */
	v_return_val_if_fail (ctx, axl_false);

	/* check if the tls ctx was created */
	tls_ctx = myqtt_ctx_get_data (ctx, TLS_CTX);
	if (tls_ctx != NULL) 
		return axl_true;

	/* create the tls context */
	tls_ctx = axl_new (MyQttTlsCtx, 1);
	myqtt_ctx_set_data_full (ctx,
				  /* key and value */
				  TLS_CTX, tls_ctx,
				  NULL, axl_free);

	/* init ssl ciphers and engines */
	SSL_library_init ();

	/* install cleanup */
	myqtt_ctx_install_cleanup (ctx, (axlDestroyFunc) myqtt_tls_cleanup);

	return axl_true;
}

/** 
 * @brief Allows to check if the provided connection has TLS
 * activated.
 *
 * @param conn The connection where the operation is taking place.
 *
 * @return axl_true in the case TLS is enabled on this connection
 * otherwsie axl_false is returned.
 */
axl_bool           myqtt_tls_is_on                      (MyQttConn            * conn)
{
	return (PTR_TO_INT (myqtt_conn_get_data (conn, "tls:status")));
}

/** 
 * @brief Allows to configure the SSL context creation function.
 *
 * See \ref MyQttTlsCtxCreation for more information.
 *
 * If you want to configure a global handler to be called for all
 * connections, you can use the default handler: \ref
 * myqtt_tls_set_default_ctx_creation.
 *
 * <i>NOTE: Using this function for the server side, will disable the
 * following handlers, (provided at the \ref myqtt_tls_accept_negotiation):
 * 
 *  - certificate_handler (\ref MyQttTlsCertificateFileLocator)
 *  - private_key_handler (\ref MyQttTlsPrivateKeyFileLocator)
 *
 * This is because providing a function to create the SSL context
 * (SSL_CTX) assumes the application layer on top of MyQtt Library
 * takes control over the SSL configuration process. This ensures
 * MyQtt Library will not do any additional configuration once
 * created the SSL context (SSL_CTX).
 *
 * </i>
 * 
 * @param connection The connection where TLS is going to be activated.
 *
 * @param ctx_creation The handler to be called once required a
 * SSL_CTX object.
 *
 * @param user_data User defined data, that is passed to the handler
 * provided (ctx_creation).
 */
void               myqtt_tls_set_ctx_creation           (MyQttConn     * connection,
							  MyQttTlsCtxCreation   ctx_creation, 
							  axlPointer             user_data)
{
	/* check parameters received */
	if (connection == NULL || ctx_creation == NULL)
		return;

	/* configure the handler */
	myqtt_conn_set_data (connection, CTX_CREATION,      ctx_creation);
	myqtt_conn_set_data (connection, CTX_CREATION_DATA, user_data);

	return;
}

/** 
 * @brief Allows to configure the default SSL context creation
 * function to be called when it is required a SSL_CTX object.
 *
 * See \ref MyQttTlsCtxCreation for more information.
 *
 * If you want to configure a per-connection handler you can use the
 * following: \ref myqtt_tls_set_ctx_creation.
 *
 * <i>NOTE: Using this function for the server side, will disable the
 * following handlers, (provided at the \ref myqtt_tls_accept_negotiation):
 * 
 *  - certificate_handler (\ref MyQttTlsCertificateFileLocator)
 *  - private_key_handler (\ref MyQttTlsPrivateKeyFileLocator)
 *
 * This is because providing a function to create the SSL context
 * (SSL_CTX) assumes the application layer on top of MyQtt Library
 * takes control over the SSL configuration process. This ensures
 * MyQtt Library will not do any additional configure operation once
 * created the SSL context (SSL_CTX).
 *
 * </i>
 * 
 * @param ctx The context where the operation will be performed.
 * 
 * @param ctx_creation The handler to be called once required a
 * SSL_CTX object.
 *
 * @param user_data User defined data, that is passed to the handler
 * provided (ctx_creation).
 */
void               myqtt_tls_set_default_ctx_creation   (MyQttCtx            * ctx, 
							  MyQttTlsCtxCreation   ctx_creation, 
							  axlPointer             user_data)
{
	MyQttTlsCtx * tls_ctx;

	/* check parameters received */
	if (ctx == NULL || ctx_creation == NULL)
		return;

	/* check if the tls ctx was created */
	tls_ctx = myqtt_ctx_get_data (ctx, TLS_CTX);
	if (tls_ctx == NULL) 
		return;

	/* configure the handler */
	tls_ctx->tls_default_ctx_creation           = ctx_creation;
	tls_ctx->tls_default_ctx_creation_user_data = user_data;

	return;
}

/** 
 * @brief Allows to configure a function that will be executed at the
 * end of the TLS process, before returning the connection to the
 * application level.
 *
 * See \ref MyQttTlsPostCheck for more information.
 *
 * If you want to configure a global handler to be called for all
 * connections, you can use the default handler: \ref
 * myqtt_tls_set_default_post_check
 * 
 * @param connection The connection to be post-checked.
 *
 * @param post_check The handler to be called once required to perform
 * the post-check. Passing a NULL value to this handler uninstall
 * previous check installed.
 *
 * @param user_data User defined data, that is passed to the handler
 * provided (post_check).
 */
void               myqtt_tls_set_post_check             (MyQttConn     * connection,
							  MyQttTlsPostCheck     post_check,
							  axlPointer             user_data)
{
	/* check parameters received */
	if (connection == NULL)
		return;

	if (post_check == NULL) {
		/* configure the handler */
		myqtt_conn_set_data (connection, POST_CHECK,      NULL);
		myqtt_conn_set_data (connection, POST_CHECK_DATA, NULL);
		return;
	}

	/* configure the handler */
	myqtt_conn_set_data (connection, POST_CHECK,      post_check);
	myqtt_conn_set_data (connection, POST_CHECK_DATA, user_data);

	return;
}

/** 
 * @brief Allows to configure a function that will be executed at the
 * end of the TLS process, before returning the connection to the
 * application level.
 *
 * See \ref MyQttTlsPostCheck for more information.
 *
 * If you want to configure a per-connection handler you can use: \ref
 * myqtt_tls_set_default_post_check
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param post_check The handler to be called once required to perform
 * the post-check. Passing a NULL value to this handler uninstall
 * previous check installed.
 *
 * @param user_data User defined data, that is passed to the handler
 * provided (post_check).
 */
void               myqtt_tls_set_default_post_check     (MyQttCtx            * ctx, 
							  MyQttTlsPostCheck     post_check,
							  axlPointer             user_data)
{
	MyQttTlsCtx * tls_ctx;

	/* check parameters received */
	if (ctx == NULL)
		return;

	/* check if the tls ctx was created */
	tls_ctx = myqtt_ctx_get_data (ctx, TLS_CTX);
	if (tls_ctx == NULL) 
		return;

	if (post_check == NULL) {
		/* uninstall handler */
		tls_ctx->tls_default_post_check           = NULL;
		tls_ctx->tls_default_post_check_user_data = NULL;
		return;
	}

	/* configure the handler */
	tls_ctx->tls_default_post_check           = post_check;
	tls_ctx->tls_default_post_check_user_data = user_data;

	return;
}

/** 
 * @brief Allows to configure a failure handler that will be called
 * when a failure is found at SSL level or during the handshake with
 * the particular function failing.
 *
 * @param ctx The context that will be configured with the failure handler.
 *
 * @param failure_handler The failure handler to be called when an
 * error is found.
 *
 * @param user_data Optional user pointer to be passed into the
 * function when the handler is called.
 */
void               myqtt_tls_set_failure_handler        (MyQttCtx               * ctx,
							  MyQttTlsFailureHandler   failure_handler,
							  axlPointer                user_data)
{
	MyQttTlsCtx * tls_ctx;

	/* check parameters received */
	if (ctx == NULL)
		return;

	/* check if the tls ctx was created */
	tls_ctx = myqtt_ctx_get_data (ctx, TLS_CTX);
	if (tls_ctx == NULL) 
		return;

	/* configure the handler */
	tls_ctx->failure_handler           = failure_handler;
	if (failure_handler)
		tls_ctx->failure_handler_user_data = user_data;
	else
		tls_ctx->failure_handler_user_data = NULL;

	return;
}

/** 
 * @internal
 *
 * @brief Default handlers used to actually read from underlying
 * transport while a connection is working under TLS.
 * 
 * @param connection The connection where the read operation will be performed.
 * @param buffer     The buffer where the data read will be returned
 * @param buffer_len Buffer size
 * 
 * @return How many bytes was read.
 */
int  myqtt_tls_ssl_read (MyQttConn * connection, unsigned char  * buffer, int  buffer_len)
{
	SSL         * ssl;
	MyQttMutex * mutex;
	MyQttCtx   * ctx = myqtt_conn_get_ctx (connection);
	int    res;
	int    ssl_err;

	/* get ssl object */
	ssl = myqtt_conn_get_data (connection, "ssl-data:ssl");
	if (ssl == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to find ssl object to read data");
		return 0;
	}

 retry:
	/* get and lock the mutex */
	mutex = myqtt_conn_get_data (connection, "ssl-data:mutex");
	if (mutex == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to find mutex to protect ssl object to read data");
		return 0;
	}
	myqtt_mutex_lock (mutex);

	/* read data */
	res = SSL_read (ssl, buffer, buffer_len);

	/* unlock the mutex */
	myqtt_mutex_unlock (mutex);

	/* get error returned */
	ssl_err = SSL_get_error(ssl, res);
	switch (ssl_err) {
	case SSL_ERROR_NONE:
		/* no error, return the number of bytes read */
		return res;
	case SSL_ERROR_WANT_WRITE:
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_X509_LOOKUP:
		myqtt_log (MYQTT_LEVEL_DEBUG, "SSL_read returned that isn't ready to read: retrying");
		return -2;
	case SSL_ERROR_SYSCALL:
		if(res < 0) { /* not EOF */
			if(errno == MYQTT_EINTR) {
				myqtt_log (MYQTT_LEVEL_DEBUG, "SSL read interrupted by a signal: retrying");
				goto retry;
			}
			myqtt_log (MYQTT_LEVEL_WARNING, "SSL_read (SSL_ERROR_SYSCALL)");
			return -1;
		}
		myqtt_log(MYQTT_LEVEL_DEBUG, "SSL socket closed on SSL_read (res=%d, ssl_err=%d, errno=%d)",
			    res, ssl_err, errno);
		return res;
	case SSL_ERROR_ZERO_RETURN: /* close_notify received */
		myqtt_log (MYQTT_LEVEL_DEBUG, "SSL closed on SSL_read");
		return res;
	case SSL_ERROR_SSL:
		myqtt_log (MYQTT_LEVEL_WARNING, "SSL_read function error (res=%d, ssl_err=%d, errno=%d)",
			    res, ssl_err, errno);
 		/* dump error stack */
		myqtt_tls_notify_failure_handler (ctx, connection, "SSL_read function error");
		
		return -1;
	default:
		/* nothing to handle */
		break;
	}
	myqtt_log (MYQTT_LEVEL_WARNING, "SSL_read/SSL_get_error returned %d", res);
	return -1;
}

/** 
 * @internal
 *
 * @brief Default handlers used to actually write to the underlying
 * transport while a connection is working under TLS.
 * 
 * @param connection The connection where the write operation will be performed.
 * @param buffer     The buffer containing data to be sent.
 * @param buffer_len The buffer size.
 * 
 * @return How many bytes was written.
 */
int  myqtt_tls_ssl_write (MyQttConn * connection, const unsigned char  * buffer, int  buffer_len)
{
	int           res;
	int           ssl_err;
	MyQttMutex * mutex;
#if defined(ENABLE_MYQTT_LOG) && ! defined(SHOW_FORMAT_BUGS)
	MyQttCtx   * ctx = myqtt_conn_get_ctx (connection);
#endif

	SSL * ssl = myqtt_conn_get_data (connection, "ssl-data:ssl");
	if (ssl == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to find ssl object to read data");
		return 0;
	}
	/* try to write */
 retry:
	/* get and lock the mutex */
	mutex = myqtt_conn_get_data (connection, "ssl-data:mutex");
	if (mutex == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to find mutex to protect ssl object to read data");
		return 0;
	}
	myqtt_mutex_lock (mutex);
	
	/* write data */
	res = SSL_write (ssl, buffer, buffer_len);

	/* unlock the mutex */
	myqtt_mutex_unlock (mutex);
	
	/* get error */
	ssl_err = SSL_get_error(ssl, res);
	myqtt_log (MYQTT_LEVEL_DEBUG, "%d = SSL_write ( %d), error = %d",
		    res, buffer_len, ssl_err);
	switch (ssl_err) {
	case SSL_ERROR_NONE:
		/* no error, nothing to report */
		return res;
	case SSL_ERROR_WANT_WRITE:
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_X509_LOOKUP:
		myqtt_log (MYQTT_LEVEL_DEBUG, "SSL_write returned that needs more data : retrying");
		return -2;
	case SSL_ERROR_SYSCALL:
		if(res < 0) { /* really an error */
			if(errno == MYQTT_EINTR) {
				myqtt_log (MYQTT_LEVEL_DEBUG, "SSL write interrupted by a signal: retrying");
				goto retry;
			}
			myqtt_log (MYQTT_LEVEL_CRITICAL, "SSL_write (ERROR_SYSCALL)");
			return -1;
		}
		break;
	case SSL_ERROR_ZERO_RETURN: /* close_notify received */
		myqtt_log (MYQTT_LEVEL_DEBUG, "SSL closed on SSL_write");
		return 0;
	case SSL_ERROR_SSL:
		myqtt_log (MYQTT_LEVEL_WARNING, "SSL_write ssl internal error (res=%d, ssl_err=%d, errno=%d)",
			    res, ssl_err, errno);
		return -1;
	default:
		/* nothing to handle */
		break;
	}

	myqtt_log (MYQTT_LEVEL_WARNING, "SSL_write/SSL_get_error returned %d", res);
	return -1;
}

/** 
 * @internal Internal function to release and free the mutex memory
 * allocated.
 * 
 * @param mutex The mutex to destroy.
 */
void __myqtt_tls_free_mutex (MyQttMutex * mutex)
{
	/* free mutex */
	myqtt_mutex_destroy (mutex);
	axl_free (mutex);
	return;
}

/** 
 * @internal Common function which sets needed data for the TLS
 * transport and default callbacks for read and write data.
 * 
 * @param connection The connection to configure
 * @param ssl The ssl object.
 * @param _ctx The ssl context
 */
void myqtt_tls_set_common_data (MyQttConn * connection, 
				 SSL* ssl, SSL_CTX * _ctx)
{
	MyQttMutex * mutex;
#if defined(ENABLE_MYQTT_LOG) && ! defined(SHOW_FORMAT_BUGS)
	MyQttCtx   * ctx = myqtt_conn_get_ctx (connection);
#endif

	/* set some necessary ssl data (setting destroy functions for
	 * both) */
	myqtt_conn_set_data_full (connection, "ssl-data:ssl", ssl, 
					 NULL, (axlDestroyFunc) SSL_free);
	myqtt_conn_set_data_full (connection, "ssl-data:ctx", _ctx, 
					 NULL, (axlDestroyFunc) SSL_CTX_free);

	/* set new handlers for read and write */
	myqtt_log (MYQTT_LEVEL_DEBUG, "change default handlers to be used for send/recv");

	/* create the mutex */
	mutex = axl_new (MyQttMutex, 1);
	myqtt_mutex_create (mutex);
	
	/* configure the mutex used by the connection to protect the ssl session */	
	myqtt_conn_set_data_full (connection, "ssl-data:mutex", mutex,
					 NULL, (axlDestroyFunc) __myqtt_tls_free_mutex);
	myqtt_conn_set_receive_handler (connection, myqtt_tls_ssl_read);
	myqtt_conn_set_send_handler    (connection, myqtt_tls_ssl_write);

	return;
}


/** 
 * @internal
 * @brief Invoke the specific TLS code to perform the handshake.
 * 
 * @param connection The connection where the TLS handshake will be performed.
 * 
 * @return axl_true if the handshake was successfully performed. Otherwise
 * axl_false is returned.
 */
int      myqtt_tls_invoke_tls_activation (MyQttConn * connection)
{
	/* get current context */
	MyQttCtx            * ctx = myqtt_conn_get_ctx (connection);
	SSL_CTX              * ssl_ctx;
	SSL                  * ssl;
	X509                 * server_cert;
	MyQttTlsCtxCreation   ctx_creation;
	axlPointer             ctx_creation_data;
	MyQttTlsPostCheck     post_check;
	axlPointer             post_check_data;
	int                    ssl_error;
	MyQttTlsCtx         * tls_ctx;

	/* check if the tls ctx was created */
	tls_ctx = myqtt_ctx_get_data (ctx, TLS_CTX);
	if (tls_ctx == NULL)  {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "unable to find tls context, unable to activate TLS. Did you call to init myqtt_tls module (myqtt_tls_init)?");
		return axl_false;
	}

	/* check if the connection have a ctx connection creation or
	 * the default ctx creation is configured */
	myqtt_log (MYQTT_LEVEL_DEBUG, "initializing TLS context");
	ctx_creation      = myqtt_conn_get_data (connection, CTX_CREATION);
	ctx_creation_data = myqtt_conn_get_data (connection, CTX_CREATION_DATA);
	if (ctx_creation == NULL) {
		/* get the default ctx creation */
		ctx_creation      = tls_ctx->tls_default_ctx_creation;
		ctx_creation_data = tls_ctx->tls_default_ctx_creation_user_data;
	} /* end if */

	if (ctx_creation == NULL) {
		/* fall back into the default implementation */
		ssl_ctx  = SSL_CTX_new (TLSv1_client_method ()); 
		myqtt_log (MYQTT_LEVEL_DEBUG, "ssl context SSL_CTX_new (TLSv1_client_method ()) returned = %p", ssl_ctx);
	} else {
		/* call to the default handler to create the SSL_CTX */
		ssl_ctx  = ctx_creation (connection, ctx_creation_data);
		myqtt_log (MYQTT_LEVEL_DEBUG, "ssl context ctx_creation (connection, ctx_creation_data) returned = %p", ssl_ctx);
	} /* end if */

	/* create and register the TLS method */
	if (ssl_ctx == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "error while creating TLS context");
		myqtt_tls_log_ssl (ctx);
		return axl_false;
	}

	/* create the tls transport */
	myqtt_log (MYQTT_LEVEL_DEBUG, "initializing TLS transport");
	ssl = SSL_new (ssl_ctx);       
	if (ssl == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "error while creating TLS transport, SSL_new (%p) returned NULL", ssl_ctx);
		return axl_false;
	}

	/* set the file descriptor */
	myqtt_log (MYQTT_LEVEL_DEBUG, "setting file descriptor");
	SSL_set_fd (ssl, myqtt_conn_get_socket (connection));

	/* configure read and write handlers and store default data to
	 * be used while sending and receiving data */
	myqtt_tls_set_common_data (connection, ssl, ssl_ctx);

	/* do the initial connect connect */
	myqtt_log (MYQTT_LEVEL_DEBUG, "connecting to remote TLS site");
	while (SSL_connect (ssl) <= 0) {
		
 		/* get ssl error */
  		ssl_error = SSL_get_error (ssl, -1);
 
		switch (ssl_error) {
		case SSL_ERROR_WANT_READ:
			myqtt_log (MYQTT_LEVEL_WARNING, "still not prepared to continue because read wanted");
			break;
		case SSL_ERROR_WANT_WRITE:
			myqtt_log (MYQTT_LEVEL_WARNING, "still not prepared to continue because write wanted");
			break;
		case SSL_ERROR_SYSCALL:
			myqtt_log (MYQTT_LEVEL_CRITICAL, "syscall error while doing TLS handshake, ssl error (code:%d)",
 				    ssl_error);
			
			/* now the TLS process have failed because we
			 * are in the middle of a tuning process we
			 * have to close the connection because is not
			 * possible to recover previous state */
			__myqtt_conn_set_not_connected (connection, "tls handshake failed",
							       MyQttProtocolError);

			myqtt_tls_notify_failure_handler (ctx, connection, "syscall error while doing TLS handshake, ssl error (SSL_ERROR_SYSCALL)");
			return axl_false;
		default:
			myqtt_log (MYQTT_LEVEL_CRITICAL, "there was an error with the TLS negotiation, ssl error (code:%d) : %s",
				    ssl_error, ERR_error_string (ssl_error, NULL));
			/* now the TLS process have failed, we have to
			 * restore how is read and written the data */
			myqtt_conn_set_default_io_handler (connection);
			return axl_false;
		}
	}
	myqtt_log (MYQTT_LEVEL_DEBUG, "seems SSL connect call have finished in a proper manner");

	/* check remote certificate (if it is present) */
	server_cert = SSL_get_peer_certificate (ssl);
	if (server_cert == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "server side didn't set a certificate for this session, these are bad news");
		/* myqtt_support_free (2, ssl, SSL_free, ctx, SSL_CTX_free); */
		return axl_false;
	}
	X509_free (server_cert);

	/* post SSL activation checkings */
	post_check      = myqtt_conn_get_data (connection, POST_CHECK);
	post_check_data = myqtt_conn_get_data (connection, POST_CHECK_DATA);
	if (post_check == NULL) {
		/* get the default ctx creation */
		post_check      = tls_ctx->tls_default_post_check;
		post_check_data = tls_ctx->tls_default_post_check_user_data;
	} /* end if */
	
	if (post_check != NULL) {
		/* post check function found, call it */
		if (! post_check (connection, post_check_data, ssl, ssl_ctx)) {
			/* found that the connection didn't pass post checks */
			__myqtt_conn_set_not_connected (connection, "post checks failed",
							       MyQttProtocolError);
			return axl_false;
		} /* end if */
	} /* end if */

	myqtt_log (MYQTT_LEVEL_DEBUG, "TLS transport negotiation finished");
	
	return axl_true;
}

/** 
 * @brief Allows to verify peer certificate after successfully
 * establish TLS session.
 *
 * This function is useful to check if certificate used by the remote
 * peer is valid.
 *
 * @param connection The connection where the certificate will be checked.
 *
 * @return axl_true If certificate verification status is ok,
 * otherwise axl_false is returned.
 */
axl_bool               myqtt_tls_verify_cert                (MyQttConn     * connection)
{
	SSL       * ssl;
	X509      * peer;
	char        peer_common_name[512];
	MyQttCtx * ctx;
	long        result;

	if (connection == NULL)
		return axl_false;

	/* get ctx from this connection */
	ctx = CONN_CTX (connection);

	/* check if connection has TLS enabled */
	if (! myqtt_tls_is_on (connection)) {
		myqtt_log (MYQTT_LEVEL_WARNING, "TLS wasn't enabled on this connection (id=%d), cert verify cannot succeed",
			    myqtt_conn_get_id (connection));
		return axl_false;
	} /* end if */

	/* get ssl object */
	ssl = myqtt_conn_get_data (connection, "ssl-data:ssl");
	if (! ssl) {
		myqtt_log (MYQTT_LEVEL_WARNING, "SSL object cannot found for this connection (id=%d), cert verify cannot succeed",
			    myqtt_conn_get_id (connection));
		return axl_false;
	} /* end if */

	/* check certificate */
	result = SSL_get_verify_result (ssl);
	if (result != X509_V_OK) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Certificate verify failed (SSL_get_verify_result failed = %ld)", result);
		/* dump stack if no failure handler */
		myqtt_tls_log_ssl (ctx);
		return axl_false;
	} /* end if */

	/*
	 * Check the common name
	 */
	peer = SSL_get_peer_certificate (ssl);
	if (! peer) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Certificate verify failed (SSL_get_verify_result failed)");
		/* dump stack if no failure handler */
		myqtt_tls_log_ssl (ctx);
		return axl_false;
	} /* end if */

	/* get certificate announced common name */
	X509_NAME_get_text_by_NID (X509_get_subject_name (peer), NID_commonName, peer_common_name, 512);

	if (! axl_cmp (peer_common_name, myqtt_conn_get_host (connection))) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "Certificate common name %s == serverName %s MISMATCH",
			    peer_common_name, myqtt_conn_get_host (connection));
		return axl_false;
	}

	/* verification ok */
	return axl_true;
}

/** 
 * @internal
 * @brief Default certificate locator function which returns the test
 * certificate used by the MyQtt Library.
 */
char  * myqtt_tls_default_certificate (MyQttConn * connection,
				        const char       * serverName)
{
	MyQttCtx   * ctx         = myqtt_conn_get_ctx (connection);
	char        * certificate;

	certificate = myqtt_support_find_data_file (ctx, "test-certificate.pem");

	myqtt_log (MYQTT_LEVEL_DEBUG, "getting default test certificate: %s (ctx: %p)", certificate, ctx);
	
	/* return certificate found */
	return certificate;
}

/** 
 * @internal
 *
 * @brief Default private key locator function which returns the test
 * private key used by the MyQtt Library.
 */
char  * myqtt_tls_default_private_key (MyQttConn * connection,
				        const char       * serverName)
{
	MyQttCtx   * ctx = myqtt_conn_get_ctx (connection);

	return myqtt_support_find_data_file (ctx, "test-private-key.pem");
}


typedef struct _MyQttTlsSyncResult {
	MyQttConn        * connection;
	MyQttStatus        status;
	char             * status_message;
} MyQttTlsSyncResult;




/** 
 * @brief Returns the SSL object associated to the given connection.
 *
 * Once a TLS negotiation has finished, the SSL object representing
 * the TLS session is stored on the connection. This function allows
 * to return that reference.
 *
 * @param connection The connection with a TLS session activated.
 * 
 * @return The SSL object reference or NULL if not defined. The
 * function returns NULL if TLS profile is not activated or was not
 * activated on the connection.
 */
axlPointer         myqtt_tls_get_ssl_object             (MyQttConn * connection)
{
	/* return the ssl object which is stored under the key:
	 * ssl-data:ssl */
	return myqtt_conn_get_data (connection, "ssl-data:ssl");
}

/** 
 * @brief Allows to return the certificate digest from the remote peer
 * given TLS session is activated (this is also called the certificate
 * fingerprint).
 * 
 * @param connection The connection where a TLS session was activated,
 * and a message digest is required using as input the certificate
 * provided by the remote peer.
 * 
 * @param method This is the digest method to use.
 * 
 * @return A newly allocated fingerprint or NULL if it fails. If NULL
 * is returned there is a TLS error (certificate not provided) or the
 * system is out of memory.
 */
char             * myqtt_tls_get_peer_ssl_digest        (MyQttConn           * connection, 
							 MyQttDigestMethod     method)
{
	/* variable declaration */
	int             iterator;
	unsigned int    message_size;
	unsigned char   message [EVP_MAX_MD_SIZE];
	char          * result;
	
	/* ssl variables */
	SSL          * ssl;
	X509         * peer_cert;
	const EVP_MD * digest_method = NULL;

	/* get ssl object */
	ssl = myqtt_tls_get_ssl_object (connection);
	if (ssl == NULL)
		return NULL;

	/* get remote peer */
	peer_cert = SSL_get_peer_certificate (ssl);
	if (peer_cert == NULL) {
		return NULL;
	}

	/* configure method digest */
	switch (method) {
	case MYQTT_SHA1:
		digest_method = EVP_sha1 ();
		break;
	case MYQTT_MD5:
		digest_method = EVP_md5 ();
		break;
	case MYQTT_DIGEST_NUM:
		/* do nothing */
		return NULL;
	}
	
	/* get the message digest and check */
	if (! X509_digest (peer_cert, digest_method, message, &message_size)) {
		return NULL;
	} 

	/* allocate enough memory for the result */
	result = axl_new (char, (message_size * 3) + 1);

	/* translate value returned into an octal representation */
	for (iterator = 0; iterator < message_size; iterator++) {
#if defined(AXL_OS_WIN32)  && ! defined(__GNUC__)
		sprintf_s (result + (iterator * 3), (message_size * 3), "%02X%s", 
			       message [iterator], 
			       (iterator + 1 != message_size) ? ":" : "");
#else
		sprintf (result + (iterator * 3), "%02X%s", 
			     message [iterator], 
			     (iterator + 1 != message_size) ? ":" : "");
#endif
	}

	return result;
}

/** 
 * @brief Allows to create a digest from the provided string.
 * 
 * @param string The string to digest.
 *
 * @param method The digest method to be used for the resulting
 * output.
 * 
 * @return A hash value that represents the string provided.
 */
char             * myqtt_tls_get_digest                 (MyQttDigestMethod   method,
							  const char         * string)
{
	/* use sized version */
	return myqtt_tls_get_digest_sized (method, string, strlen (string));
}

/** 
 * @brief Allows to create a digest from the provided string,
 * configuring the size of the string to be calculated. This function
 * performs the same action as \ref myqtt_tls_get_digest, but
 * allowing to configure the size of the input buffer to be used to
 * configure the resulting digest.
 * 
 * @param method The digest method to be used for the resulting
 * output.
 *
 * @param content The content to digest.
 *
 * @param content_size The amount of data to be taken from the input
 * provided.
 * 
 * @return A hash value that represents the string provided.
 */
char             * myqtt_tls_get_digest_sized           (MyQttDigestMethod   method,
							  const char         * content,
							  int                  content_size)
{
	char          * result = NULL;
	unsigned char   buffer[EVP_MAX_MD_SIZE];
	EVP_MD_CTX      mdctx;
	const EVP_MD  * md = NULL;
	int             iterator;
#ifdef __SSL_0_97__
	unsigned int   md_len;
#else
	unsigned int   md_len = EVP_MAX_MD_SIZE;
#endif
		
	if (content == NULL)
		return NULL;

	/* create the digest method */
	/* configure method digest */
	switch (method) {
	case MYQTT_SHA1:
		md = EVP_sha1 ();
		break;
	case MYQTT_MD5:
		md = EVP_md5 ();
		break;
	case MYQTT_DIGEST_NUM:
		/* do nothing */
		return NULL;
	}
	
	/* add all digest */
	/* OpenSSL_add_all_digests(); */
		
#ifdef __SSL_0_97__
	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, md, NULL);
	EVP_DigestUpdate(&mdctx, content, content_size);
	EVP_DigestFinal_ex(&mdctx, buffer, &md_len);
	EVP_MD_CTX_cleanup(&mdctx);
#else
	EVP_DigestInit (&mdctx, md);
	EVP_DigestUpdate (&mdctx, content, content_size);
	EVP_DigestFinal (&mdctx, buffer, &md_len);
#endif

	/* translate it into a octal format, allocate enough memory
	 * for the result */
	result = axl_new (char, (md_len * 3) + 1);

	/* translate value returned into an octal representation */
	for (iterator = 0; iterator < md_len; iterator++) {
#if defined(AXL_OS_WIN32) && ! defined(__GNUC__)
		sprintf_s (result + (iterator * 3), (md_len - iterator) * 3, "%02X%s", 
			   buffer [iterator], 
			   (iterator + 1 != md_len) ? ":" : "");
#else	
		sprintf (result + (iterator * 3), "%02X%s", 
			 buffer [iterator], 
			 (iterator + 1 != md_len) ? ":" : "");
#endif
	}
	
	/* return the digest */
	return result;
}

/** 
 * @internal Function used by the main module to cleanup the tls
 * module on exit (dealloc all memory used by openssl).
 */
void               myqtt_tls_cleanup (MyQttCtx * ctx)
{
	/* remove all cyphers */
	EVP_cleanup ();
	CRYPTO_cleanup_all_ex_data ();
	ERR_free_strings ();
/*	COMP_zlib_cleanup (); */
	return;
}

/* @} */

