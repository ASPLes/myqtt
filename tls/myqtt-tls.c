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
#include <myqtt-conn-private.h>
#include <myqtt-ctx-private.h>
#include <myqtt-listener-private.h>

/** 
 * \defgroup myqtt_tls MyQtt SSL/TLS: support functions to create secured SSL/TLS MQTT connections and listeners
 */

/** 
 * \addtogroup myqtt_tls
 * @{
 */

#define LOG_DOMAIN "myqtt-tls"

#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/* some keys to store handlers and its associate data */
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
	 * support. */
	MyQttTlsCertificateFileLocator      tls_certificate_handler;
	MyQttTlsPrivateKeyFileLocator       tls_private_key_handler;
	MyQttTlsChainCertificateFileLocator tls_chain_handler;
	axlPointer                          tls_handler_data;

	/* default post check */
	MyQttTlsPostCheck                   tls_default_post_check;
	axlPointer                          tls_default_post_check_user_data;

	/* failure handler */
	MyQttTlsFailureHandler              failure_handler;
	axlPointer                          failure_handler_user_data;

} MyQttTlsCtx;

int __myqtt_tls_handle_error (MyQttConn * conn, int res, const char * label, axl_bool * needs_retry)
{
	int ssl_err;
	MyQttCtx * ctx;

	(*needs_retry) = axl_false;

	if (conn == NULL)
		return -1;
	ctx = conn->ctx;

	/* get error returned */
	ssl_err = SSL_get_error (conn->ssl, res);
	switch (ssl_err) {
	case SSL_ERROR_NONE:
		/* no error, return the number of bytes read */
	        /* myqtt_log (MYQTT_LEVEL_DEBUG, "%s, ssl_err=%d, perfect, no error reported, bytes read=%d", 
		   label, ssl_err, res); */
		return res;
	case SSL_ERROR_WANT_WRITE:
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_X509_LOOKUP:
	        myqtt_log (MYQTT_LEVEL_DEBUG, "%s, ssl_err=%d returned that isn't ready to read/write: you should retry", 
			   label, ssl_err);
		(*needs_retry) = axl_true;
		return -2;
	case SSL_ERROR_SYSCALL:
		if (res < 0) { /* not EOF */
			if (errno == MYQTT_EINTR) {
				myqtt_log (MYQTT_LEVEL_DEBUG, "%s interrupted by a signal: retrying", label);
				/* report to retry */
				return -2;
			}
			myqtt_log (MYQTT_LEVEL_WARNING, "SSL_read (SSL_ERROR_SYSCALL)");
			return -1;
		}
		myqtt_log (MYQTT_LEVEL_CRITICAL, "SSL socket closed on %s (res=%d, ssl_err=%d, errno=%d)",
			   label, res, ssl_err, errno);
		myqtt_tls_log_ssl (ctx);

		return res;
	case SSL_ERROR_ZERO_RETURN: /* close_notify received */
		myqtt_log (MYQTT_LEVEL_DEBUG, "SSL closed on %s", label);
		return res;
	case SSL_ERROR_SSL:
		myqtt_log (MYQTT_LEVEL_WARNING, "%s function error (received SSL_ERROR_SSL) (res=%d, ssl_err=%d, errno=%d)",
			   label, res, ssl_err, errno);
		myqtt_tls_log_ssl (ctx);
		return -1;
	default:
		/* nothing to handle */
		break;
	}
	myqtt_log (MYQTT_LEVEL_WARNING, "%s/SSL_get_error returned %d", label, res);
	return -1;
	
}


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
 * @brief Allows to configure the handler that will be used to let
 * user land code to define OpenSSL SSL_CTX object.
 *
 * By default, SSL_CTX (SSL Context) object is created by default
 * settings that works for most of the cases. In the case you want to
 * configure particular configurations that should be enabled on the
 * provided SSL_CTX that is going to be used by the client ---while
 * connecting--- or server ---while receiving a connection--- then use
 * this function to setup your creator handler.
 *
 * See \ref MyQttSslContextCreator for more information about this
 * handler.
 *
 * @param ctx The context that will be configured.
 *
 * @param context_creator The context creator function that will be used by the engine
 *
 * @param user_data A user defined reference passed in into the
 * context_creator handler.
 *
 */
void           myqtt_tls_set_ssl_context_creator (MyQttCtx                * ctx,
						  MyQttSslContextCreator    context_creator,
						  axlPointer                user_data)
{
	if (ctx == NULL)
		return;

	/* set handlers as indicated by the caller */
	ctx->context_creator      = context_creator;
	ctx->context_creator_data = user_data;
	return;
}

/** 
 * @brief Allows to set the serverName indication (SNI) that is going
 * to be used by the client connection created using the provided
 * connection options object.
 *
 * This function allows to configure the serverName indication
 * that is sent at TLS connection time.
 *
 * @param opts The connection options that will be configured.
 *
 * @param serverName the serverName to configure. Keep in mind that
 * the remote side may deny accepting the connection under the
 * serverName requested.
 */
void               myqtt_tls_opts_set_server_name        (MyQttConnOpts * opts,
							  const char    * serverName)
{
	char * temp;
	if (opts == NULL || serverName == NULL)
		return;
	/* configure value */
	temp = opts->serverName;
	opts->serverName = axl_strdup (serverName);
	axl_free (temp);

	return;
}

SSL_CTX * __myqtt_tls_conn_get_ssl_context (MyQttCtx * ctx, MyQttConn * conn, MyQttConnOpts * opts, axl_bool is_client)
{
	SSL_CTX * ptr;

	myqtt_log (MYQTT_LEVEL_DEBUG, "Attempting to create SSL_CTX for ctx=%p, conn=%p, opts=%p, is_client=%d",
		   ctx, conn, opts, is_client);

	/* call to user defined function if the context creator is defined */
	if (ctx && ctx->context_creator) {
		ptr =  ((MyQttSslContextCreator) ctx->context_creator) (ctx, conn, opts, is_client, ctx->context_creator_data);
		myqtt_log (MYQTT_LEVEL_DEBUG, "ctx->context_creator (ctx=%p, conn=%p, opts=%p, is_client=%d, ctx->context_creator_data=%s) = %p",
			   ctx, conn, opts, is_client, ctx->context_creator_data);
		return ptr;
	} /* end if */

	if (opts == NULL) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "No context creator and opts=NULL, reporting default SSL_CTX_new (TLSv1) for  for ctx=%p, conn=%p, opts=%p, is_client=%d",
			   ctx, conn, opts, is_client);

		/* printf ("**** REPORTING TLSv1 ****\n"); */
		return SSL_CTX_new (is_client ? TLSv1_client_method () : TLSv1_server_method ()); 
	} /* end if */

	myqtt_log (MYQTT_LEVEL_DEBUG, "Reporting default method according to opts->ssl_protocol=%d for  for ctx=%p, conn=%p, opts=%p, is_client=%d",
		   opts->ssl_protocol, ctx, conn, opts, is_client);

	switch (opts->ssl_protocol) {
	case MYQTT_METHOD_TLSV1:
		return SSL_CTX_new (is_client ? TLSv1_client_method () : TLSv1_server_method ()); 
#if defined(TLSv1_1_client_method)
	case MYQTT_METHOD_TLSV1_1:
		/* printf ("**** REPORTING TLSv1.1 ****\n"); */
		return SSL_CTX_new (is_client ? TLSv1_1_client_method () : TLSv1_1_server_method ()); 
#endif
	case MYQTT_METHOD_SSLV3:
		/* printf ("**** REPORTING SSLv3 ****\n"); */
		return SSL_CTX_new (is_client ? SSLv3_client_method () : SSLv3_server_method ()); 
	case MYQTT_METHOD_SSLV23:
		/* printf ("**** REPORTING SSLv23 ****\n"); */
		return SSL_CTX_new (is_client ? SSLv23_client_method () : SSLv23_server_method ()); 
	}

	/* reached this point, report default TLSv1 method */
	return SSL_CTX_new (is_client ? TLSv1_client_method () : TLSv1_server_method ()); 
}

MyQttCtx * __myqtt_tls_ssl_ctx_debug = NULL;

int __myqtt_tls_ssl_verify_callback (int ok, X509_STORE_CTX * store) {
	char   data[256];
	X509 * cert;
	int    depth;
	int    err;
	MyQttCtx * ctx = __myqtt_tls_ssl_ctx_debug;

	if (! ok) {
		cert  = X509_STORE_CTX_get_current_cert (store);
		depth = X509_STORE_CTX_get_error_depth (store);
		err   = X509_STORE_CTX_get_error (store);

		myqtt_log (MYQTT_LEVEL_CRITICAL, "CERTIFICATE: error at depth: %d", depth);

		X509_NAME_oneline (X509_get_issuer_name (cert), data, 256);
		myqtt_log (MYQTT_LEVEL_CRITICAL, "CERTIFICATE: issuer: %s", data);

		X509_NAME_oneline (X509_get_subject_name (cert), data, 256);
		myqtt_log (MYQTT_LEVEL_CRITICAL, "CERTIFICATE: subject: %s", data);

		myqtt_log (MYQTT_LEVEL_CRITICAL, "CERTIFICATE: error %d:%s", err, X509_verify_cert_error_string (err));
			    
	}
	return ok; /* return same value */
}

axl_bool __myqtt_conn_set_ssl_client_options (MyQttCtx * ctx, MyQttConn * conn, MyQttConnOpts * options)
{
	myqtt_log (MYQTT_LEVEL_DEBUG, "Checking to establish SSL options (%p)", options);

	if (options && options->ca_certificate) {
		myqtt_log ( MYQTT_LEVEL_DEBUG, "Setting CA certificate: %s", options->ca_certificate);
		if (SSL_CTX_load_verify_locations (conn->ssl_ctx, options->ca_certificate, NULL) != 1) {
			myqtt_log ( MYQTT_LEVEL_CRITICAL, "Failed to configure CA certificate (%s), SSL_CTX_load_verify_locations () failed", options->ca_certificate);
			return axl_false;
		} /* end if */
		
	} /* end if */

	/* enable default verification paths */
	if (SSL_CTX_set_default_verify_paths (conn->ssl_ctx) != 1) {
		myqtt_log ( MYQTT_LEVEL_CRITICAL, "Unable to configure default verification paths, SSL_CTX_set_default_verify_paths () failed");
		return axl_false;
	} /* end if */

	if (options && options->chain_certificate) {
		myqtt_log ( MYQTT_LEVEL_DEBUG, "Setting chain certificate: %s", options->chain_certificate);
		if (SSL_CTX_use_certificate_chain_file (conn->ssl_ctx, options->chain_certificate) != 1) {
			myqtt_log ( MYQTT_LEVEL_CRITICAL, "Failed to configure chain certificate (%s), SSL_CTX_use_certificate_chain_file () failed", options->chain_certificate);
			return axl_false;
		} /* end if */
	} /* end if */

	if (options && options->certificate) {
		myqtt_log ( MYQTT_LEVEL_DEBUG, "Setting certificate: %s", options->certificate);
		if (SSL_CTX_use_certificate_chain_file (conn->ssl_ctx, options->certificate) != 1) {
			myqtt_log ( MYQTT_LEVEL_CRITICAL, "Failed to configure client certificate (%s), SSL_CTX_use_certificate_file () failed", options->certificate);
			return axl_false;
		} /* end if */
	} /* end if */

	if (options && options->private_key) {
		myqtt_log ( MYQTT_LEVEL_DEBUG, "Setting private key: %s", options->private_key);
		if (SSL_CTX_use_PrivateKey_file (conn->ssl_ctx, options->private_key, SSL_FILETYPE_PEM) != 1) {
			myqtt_log ( MYQTT_LEVEL_CRITICAL, "Failed to configure private key (%s), SSL_CTX_use_PrivateKey_file () failed", options->private_key);
			return axl_false;
		} /* end if */
	} /* end if */

	if (options && options->private_key && options->certificate) {
		if (!SSL_CTX_check_private_key (conn->ssl_ctx)) {
			myqtt_log ( MYQTT_LEVEL_CRITICAL, "Certificate and private key do not matches, verification fails, SSL_CTX_check_private_key ()");
			return axl_false;
		} /* end if */
		myqtt_log ( MYQTT_LEVEL_DEBUG, "Certificate (%s) and private key (%s) matches", options->certificate, options->private_key);
	} /* end if */

	/* if no option and it is not disabled */
	if (options == NULL || ! options->disable_ssl_verify) {
		myqtt_log ( MYQTT_LEVEL_DEBUG, "Enabling certificate peer verification (options=%p, disable_ssl_verify=%d)",
			    options, options ? options->disable_ssl_verify : 0);
		/** really, really ugly hack to let
		 * __myqtt_conn_ssl_verify_callback to be able to get
		 * access to the context required to drop some logs */
		__myqtt_tls_ssl_ctx_debug = ctx;
		SSL_CTX_set_verify (conn->ssl_ctx, SSL_VERIFY_PEER, __myqtt_tls_ssl_verify_callback); 
		SSL_CTX_set_verify_depth (conn->ssl_ctx, 10); 
	} /* end if */

	return axl_true;
}

/** 
 * @internal Default connection receive until handshake is complete.
 */
int __myqtt_tls_receive (MyQttConn * conn, unsigned char * buffer, int buffer_size)
{
	int      res;
	axl_bool needs_retry;
	int      tries = 0;

	/* call to read content */
	while (tries < 50) {
	        res = SSL_read (conn->ssl, buffer, buffer_size);

		/* call to handle error */
		res = __myqtt_tls_handle_error (conn, res, "SSL_read", &needs_retry);
		
		if (! needs_retry)
		        break;

		/* next operation */
		tries++;
	}
	return res;
}

/** 
 * @internal Default connection send until handshake is complete.
 */
int __myqtt_tls_send (MyQttConn * conn, const unsigned char * buffer, int buffer_size)
{
	int         res;
	axl_bool    needs_retry;
	int         tries = 0;
	MyQttCtx  * ctx = conn->ctx;

	/* call to read content */
	while (tries < 50) {
	        res = SSL_write (conn->ssl, buffer, buffer_size);
		myqtt_log (MYQTT_LEVEL_DEBUG, "SSL: sent %d bytes (requested: %d)..", res, buffer_size); 

		/* call to handle error */
		res = __myqtt_tls_handle_error (conn, res, "SSL_write", &needs_retry);
		/* myqtt_log (MYQTT_LEVEL_DEBUG, "   SSL: after processing error, sent %d bytes (requested: %d)..",  res, buffer_size); */

		if (! needs_retry)
		        break;

		/* next operation */
		myqtt_sleep (tries * 10000);
		tries++;
	}
	return res;
}



axl_bool __myqtt_tls_session_setup (MyQttCtx * ctx, MyQttConn * conn, MyQttConnOpts * opts, axlPointer user_data)
{
	int        iterator;
	int        ssl_error;
	X509     * server_cert;
	int        d_timeout = myqtt_conn_get_connect_timeout (ctx);
	axlError * err = NULL;
	char     * serverName;

	if (! myqtt_tls_init (ctx)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to create TLS session, myqtt_tls_init() initialization failed");
		return axl_false;
	} /* end if */

	/* configure the socket created */
	conn->session = myqtt_conn_sock_connect_common (ctx, conn->host, conn->port, &d_timeout, conn->transport, &err);
	if (conn->session == -1) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to connect to  %s:%s, errno=%d (%s), %s", 
			   conn->host, conn->port, errno, myqtt_errno_get_last_error (),
			   axl_error_get (err));
		axl_error_free (err);
		return axl_false;
	} /* end if */

	/* found TLS connection request, enable it */
	conn->ssl_ctx  = __myqtt_tls_conn_get_ssl_context (ctx, conn, opts, axl_true);
	if (conn->ssl_ctx == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create SSL context (__myqtt_conn_get_ssl_context failed)");
		return axl_false;
	} /* end if */
	myqtt_conn_set_data_full (conn, "__my:co:ssl-ctx", conn->ssl_ctx, NULL, (axlDestroyFunc) SSL_CTX_free);

	/* check for client side SSL configuration */
	if (! __myqtt_conn_set_ssl_client_options (ctx, conn, opts)) {
		myqtt_log ( MYQTT_LEVEL_CRITICAL, "Unable to configure additional SSL options, unable to continue",
			    conn->ssl_ctx, conn->ssl);
		goto fail_ssl_connection;
	} /* end if */

	/* create context and check for result */
	conn->ssl      = SSL_new (conn->ssl_ctx);       
	if (conn->ssl)
		myqtt_conn_set_data_full (conn, "__my:co:ssl", conn->ssl, NULL, (axlDestroyFunc) SSL_free);

	/* configure here SNI indication or used suplied form by
	   calling API */
	serverName = (opts && opts->serverName) ? opts->serverName : conn->host;
	myqtt_log (MYQTT_LEVEL_DEBUG, "Setting SNI indication to serverName=%s", serverName);
	if (! SSL_set_tlsext_host_name (conn->ssl, serverName)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to configure SNI client indication (SSL_set_tlsext_ (conn->ssl_ctx=%p, conn->ssl=%p)",
			    conn->ssl_ctx, conn->ssl);
		goto fail_ssl_connection;
	}

	if (conn->ssl_ctx == NULL || conn->ssl == NULL) {
		myqtt_log ( MYQTT_LEVEL_CRITICAL, "Unable to create SSL context internal references are null (conn->ssl_ctx=%p, conn->ssl=%p)",
			    conn->ssl_ctx, conn->ssl);
	fail_ssl_connection:
		
		myqtt_conn_shutdown (conn);
		
		return axl_false;
	} /* end if */
	
	/* set socket */
	SSL_set_fd (conn->ssl, conn->session);
	
	/* do the initial connect connect */
	myqtt_log ( MYQTT_LEVEL_DEBUG, "connecting to remote TLS site");
	iterator = 0;
	while (SSL_connect (conn->ssl) <= 0) {
		
		/* get ssl error */
		ssl_error = SSL_get_error (conn->ssl, -1);
		
		switch (ssl_error) {
		case SSL_ERROR_WANT_READ:
			myqtt_log ( MYQTT_LEVEL_WARNING, "still not prepared to continue because read wanted, conn-id=%d (%p, session: %d), errno=%d",
				    conn->id, conn, conn->session, errno);
			break;
		case SSL_ERROR_WANT_WRITE:
			myqtt_log ( MYQTT_LEVEL_WARNING, "still not prepared to continue because write wanted, conn-id=%d (%p)",
				    conn->id, conn);
			break;
		case SSL_ERROR_SYSCALL:
			myqtt_log ( MYQTT_LEVEL_CRITICAL, "syscall error while doing TLS handshake, ssl error (code:%d), conn-id: %d (%p), errno: %d, session: %d",
				    ssl_error, conn->id, conn, errno, conn->session);
			myqtt_tls_log_ssl (ctx);
			myqtt_conn_shutdown (conn);
					
			return axl_false;
		default:
			myqtt_log ( MYQTT_LEVEL_CRITICAL, "there was an error with the TLS negotiation, ssl error (code:%d) : %s",
				    ssl_error, ERR_error_string (ssl_error, NULL));
			myqtt_tls_log_ssl (ctx);
			myqtt_conn_shutdown (conn);
					
			return axl_false;
		} /* end switch */
		
		/* try and limit max reconnect allowed */
		iterator++;
		
		if (iterator > 100) {
			myqtt_log ( MYQTT_LEVEL_CRITICAL, "Max retry calls=%d to SSL_connect reached, shutting down connection id=%d, errno=%d",
				    iterator, conn->id, errno);
				
			return axl_false;
		} /* end if */
		
		/* wait a bit before retry */
		myqtt_sleep (100000);
		
	} /* end while */
	
	myqtt_log ( MYQTT_LEVEL_DEBUG, "Client TLS handshake finished, configuring I/O handlers");
	
	/* check remote certificate (if it is present) */
	server_cert = SSL_get_peer_certificate (conn->ssl);
	if (server_cert == NULL) {
		myqtt_log ( MYQTT_LEVEL_CRITICAL, "server side didn't set a certificate for this session, these are bad news");
		
		return axl_false;
	}
	X509_free (server_cert);
	
	/* call to check post ssl checks after SSL finalization */
	if (conn->ctx && conn->ctx->post_ssl_check) {
		if (! ((MyQttSslPostCheck)conn->ctx->post_ssl_check) (conn->ctx, conn, conn->ssl_ctx, conn->ssl, conn->ctx->post_ssl_check_data)) {
			/* TLS post check failed */
			myqtt_log (MYQTT_LEVEL_CRITICAL, "TLS/SSL post check function failed, dropping connection");
			myqtt_conn_shutdown (conn);
			return axl_false;
		} /* end if */
	} /* end if */
	
	/* configure default handlers */
	conn->receive = __myqtt_tls_receive;
	conn->send    = __myqtt_tls_send;
	
	myqtt_log ( MYQTT_LEVEL_DEBUG, "TLS I/O handlers configured");
	conn->tls_on = axl_true;
	
	return axl_true;
}

/** @internal reference to track if SSL_library_init was called () **/
axl_bool __myqtt_tls_was_init = axl_false;


/** 
 * @brief Initialize TLS library.
 * 
 * All client applications using TLS must call to this
 * function in order to ensure TLS engine can be used.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @return axl_true if the TLS was initialized. Otherwise
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

	/* init ssl ciphers and engines (but only once even though we
	   have several contexts running on the same process) */
	if (! __myqtt_tls_was_init) {
		__myqtt_tls_was_init = axl_true;
		SSL_library_init ();

		/* install cleanup */
		atexit ((void (*)(void))myqtt_tls_cleanup);
	}

	return axl_true;
}

/** 
 * @brief Allows to create a new MQTT connection a MQTT broker/server
 * securing first the connection with TLS (MQTT over TLS).
 *
 * Please, see \ref myqtt_conn_new for more notes. You must call \ref myqtt_tls_init first before creating any connection.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param client_identifier The client identifier that uniquely
 * identifies this MQTT client from others.  It can be NULL to let
 * MQTT 3.1.1 servers to assign a default client identifier BUT
 * clean_session must be set to axl_true. This is done automatically
 * by the library (setting clean_session to axl_true when NULL is
 * provided).
 *
 * @param clean_session Flag to clean client session or to reuse the
 * existing one. If set to axl_false, you must provide a valid
 * client_identifier (otherwise the function will fail).
 *
 * @param keep_alive Keep alive configuration in seconds after which
 * the server/broker will close the connection if it doesn't detect
 * any activity. Setting 0 will disable keep alive mechanism.
 *
 * @param host The location of the MQTT server/broker
 *
 * @param port The port of the MQTT server/broker
 *
 * @param opts Optional connection options. See \ref myqtt_conn_opts_new
 *
 * @param on_connected Async notification handler that will be called
 * once the connection fails or it is completed. In the case this
 * handler is configured the caller will not be blocked. In the case
 * this parameter is NULL, the caller will be blocked until the
 * connection completes or fails.
 *
 * @param user_data User defined pointer that will be passed to the on_connected handler (in case it is defined).
 *
 * @return A reference to the newli created connection or NULL if
 * on_connected handler is provided. In both cases, the reference
 * returned (or received at the on_connected handler) must be checked
 * with \ref myqtt_conn_is_ok. 
 *
 * <b>About pending messages / queued messages </b>
 *
 * After successful connection with clean_session set to axl_false and
 * defined client identifier, the library will resend any queued or in
 * flight QoS1/QoS2 messages (as well as QoS0 if they were
 * stored). This is done in background without intefering the caller.
 *
 * If you need to get the number of queued messages that are going to
 * be sent use \ref myqtt_storage_queued_messages_offline. In the case
 * you need the number remaining during the process use \ref
 * myqtt_storage_queued_messages.
 *
 */
MyQttConn        * myqtt_tls_conn_new                   (MyQttCtx        * ctx,
							 const char      * client_identifier,
							 axl_bool          clean_session,
							 int               keep_alive,
							 const char      * host, 
							 const char      * port,
							 MyQttConnOpts   * opts,
							 MyQttConnNew      on_connected, 
							 axlPointer        user_data)
{
	MyQttNetTransport transport;
	
	/* get transport we have to use */
	transport = __myqtt_conn_detect_transport (ctx, host);

	/* call to create the connection */
	return myqtt_conn_new_full_common (ctx, client_identifier, clean_session, keep_alive, host, port, __myqtt_tls_session_setup, NULL, on_connected, transport, opts, user_data);
}

/** 
 * @brief Allows to create a new MQTT connection to a MQTT
 * broker/server securing first the connection with TLS (MQTT over
 * TLS), forcing IPv6 transport.
 *
 * You must call \ref myqtt_tls_init first before creating any connection.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param client_identifier The client identifier that uniquely
 * identifies this MQTT client from others.  It can be NULL to let
 * MQTT 3.1.1 servers to assign a default client identifier BUT
 * clean_session must be set to axl_true. This is done automatically
 * by the library (setting clean_session to axl_true when NULL is
 * provided).
 *
 * @param clean_session Flag to clean client session or to reuse the
 * existing one. If set to axl_false, you must provide a valid
 * client_identifier (otherwise the function will fail).
 *
 * @param keep_alive Keep alive configuration in seconds after which
 * the server/broker will close the connection if it doesn't detect
 * any activity. Setting 0 will disable keep alive mechanism.
 *
 * @param host The location of the MQTT server/broker
 *
 * @param port The port of the MQTT server/broker
 *
 * @param opts Optional connection options. See \ref myqtt_conn_opts_new
 *
 * @param on_connected Async notification handler that will be called
 * once the connection fails or it is completed. In the case this
 * handler is configured the caller will not be blocked. In the case
 * this parameter is NULL, the caller will be blocked until the
 * connection completes or fails.
 *
 * @param user_data User defined pointer that will be passed to the on_connected handler (in case it is defined).
 *
 * @return A reference to the newli created connection or NULL if
 * on_connected handler is provided. In both cases, the reference
 * returned (or received at the on_connected handler) must be checked
 * with \ref myqtt_conn_is_ok. 
 *
 * <b>About pending messages / queued messages </b>
 *
 * After successful connection with clean_session set to axl_false and
 * defined client identifier, the library will resend any queued or in
 * flight QoS1/QoS2 messages (as well as QoS0 if they were
 * stored). This is done in background without intefering the caller.
 *
 * If you need to get the number of queued messages that are going to
 * be sent use \ref myqtt_storage_queued_messages_offline. In the case
 * you need the number remaining during the process use \ref
 * myqtt_storage_queued_messages.
 *
 */
MyQttConn        * myqtt_tls_conn_new6                  (MyQttCtx       * ctx,
							 const char     * client_identifier,
							 axl_bool         clean_session,
							 int              keep_alive,
							 const char     * host, 
							 const char     * port,
							 MyQttConnOpts  * opts,
							 MyQttConnNew     on_connected, 
							 axlPointer       user_data)
{
	/* call to create the connection */
	return myqtt_conn_new_full_common (ctx, client_identifier, clean_session, keep_alive, host, port, __myqtt_tls_session_setup, NULL, on_connected, MYQTT_IPv6, opts, user_data);
}


/** 
 * @brief Allows to configure the set of functions that will help the
 * engine to find the appropriate certificate/key/chain according to
 * the serverName announced by the SNI indication.
 *
 * By default, certificates set \ref myqtt_tls_set_certificate or \ref
 * myqtt_tls_opts_set_ssl_certs are used. 
 *
 * However, in the case SNI indication is received, you can use this
 * function to install a set of handlers that are called to find the
 * right certificate according to the serverName. If these handlers
 * aren't configured and a SNI indication is received, the serverName
 * is attached to the connection to be used by \ref myqtt_conn_get_server_name
 *
 * @param ctx The context that is going to be configured.
 *
 * @param certificate_handler The certificate locator handler to find
 * the suitable file or content according to the serverName.
 *
 * @param private_key_handler The private key locator handler to find
 * the suitable file or content according to the serverName.
 *
 * @param chain_handler Optiona handler that allows to find the
 * suitable file or content according to the serverName to provide the
 * chain certificate.
 *
 * @param user_data User defined pointer that is passed to all
 * handlers configured at this function.
 */
void              myqtt_tls_listener_set_certificate_handlers (MyQttCtx                            * ctx,
							       MyQttTlsCertificateFileLocator        certificate_handler,
							       MyQttTlsPrivateKeyFileLocator         private_key_handler,
							       MyQttTlsChainCertificateFileLocator   chain_handler,
							       axlPointer                            user_data)
{
	MyQttTlsCtx * tls_ctx;

	if (ctx == NULL)
		return;

	/* ensure module initialization */
	myqtt_tls_init (ctx);

	/* check if the tls ctx was created */
	tls_ctx = myqtt_ctx_get_data (ctx, TLS_CTX);
	if (tls_ctx == NULL) 
		return;

	/* set up handlers */
	tls_ctx->tls_certificate_handler = certificate_handler;
	tls_ctx->tls_private_key_handler = private_key_handler;
	tls_ctx->tls_chain_handler       = chain_handler;
	tls_ctx->tls_handler_data        = user_data;

	return;
}

/** 
 * @brief Allows to disable peer ssl certificate verification. This is
 * not recommended for production enviroment. This affects in a
 * different manner to a listener connection and a client connection.
 *
 * For a client connection, by default, peer verification is enabled
 * and this function may help to disable it during development or
 * other reasons.
 *
 * In the case of the servers (created by using \ref
 * myqtt_listener_new for example) this is not required because by
 * default peer verification is disabled by default.
 *
 * @param opts The connection option to configure.
 *
 * @param verify axl_true to disable verification
 * otherwise, axl_false should be used. By default SSL verification
 * is enabled.
 *
 */
void myqtt_tls_opts_ssl_peer_verify (MyQttConnOpts * opts, axl_bool verify)
{
	if (opts == NULL)
		return;
	opts->disable_ssl_verify = ! verify;
	return;
}

/** 
 * @brief Allows to certificate, private key and optional chain
 * certificate and ca for on a particular options that can be used for
 * a client and a listener connection.
 *
 * @param opts The connection options where these settings will be
 * applied.
 *
 * @param certificate The certificate to use on the connection.
 *
 * @param private_key client_certificate private key.
 *
 * @param chain_certificate Optional chain certificate to use 
 *
 * @param ca_certificate Optional CA certificate to use during the
 * process.
 *
 * @return axl_true in the case all certificate files provided are
 * reachable.
 */
axl_bool        myqtt_tls_opts_set_ssl_certs    (MyQttConnOpts * opts, 
						 const char     * certificate,
						 const char     * private_key,
						 const char     * chain_certificate,
						 const char     * ca_certificate)
{
	if (opts == NULL)
		return axl_false;

	
	if (certificate && access (certificate, R_OK) != 0)
		return axl_false;
	if (private_key && access (private_key, R_OK) != 0)
		return axl_false;
	if (chain_certificate && access (chain_certificate, R_OK) != 0)
		return axl_false;
	if (ca_certificate && access (ca_certificate, R_OK) != 0)
		return axl_false;
	
	/* store certificate settings */
	opts->certificate        = axl_strdup (certificate);
	opts->private_key        = axl_strdup (private_key);
	opts->chain_certificate  = axl_strdup (chain_certificate);
	opts->ca_certificate     = axl_strdup (ca_certificate);

	return axl_true;
}

axl_bool __myqtt_tls_prepare_certificates (MyQttConn     * conn, 
					   MyQttConnOpts * opts, 
					   const char    * certificate, 
					   const char    * key, 
					   const char    * chain_certificate,
					   const char    * ca_certificate)
{
	MyQttCtx   * ctx = CONN_CTX (conn);
	const char * __ca_certificate = NULL;
	
	/* select the ca certificate from input or from options */
	if (ca_certificate)
		__ca_certificate = ca_certificate;
	if (! __ca_certificate && opts && opts->ca_certificate)
		__ca_certificate = opts->ca_certificate;

	if (__ca_certificate) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "Setting up CA certificate: %s", __ca_certificate);
		if (SSL_CTX_load_verify_locations (conn->ssl_ctx, __ca_certificate, NULL) != 1) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to configure CA certificate (%s), SSL_CTX_load_verify_locations () failed", 
				   __ca_certificate);

			/* dump error stack */
			myqtt_conn_shutdown (conn);

			return axl_false;
		} /* end if */
	} /* end if */
	
	/* enable default verification paths */
	if (SSL_CTX_set_default_verify_paths (conn->ssl_ctx) != 1) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to configure default verification paths, SSL_CTX_set_default_verify_paths () failed");

		/* dump error stack */
		myqtt_conn_shutdown (conn);
		return axl_false;
	} /* end if */
	
	/* configure chain certificate */
	if (chain_certificate) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "Setting up chain certificate: %s", chain_certificate);
		if (SSL_CTX_use_certificate_chain_file (conn->ssl_ctx, chain_certificate) != 1) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to configure chain certificate (%s), SSL_CTX_use_certificate_chain_file () failed", 
				   chain_certificate);

			/* dump error stack */
			myqtt_conn_shutdown (conn);

			return axl_false;
		} /* end if */
	} /* end if */
	
	myqtt_log (MYQTT_LEVEL_DEBUG, "Using certificate file: %s (with ssl context ref: %p)", certificate, conn->ssl_ctx);
	if (conn->ssl_ctx == NULL || SSL_CTX_use_certificate_chain_file (conn->ssl_ctx, certificate) != 1) {
		/* drop an error log */
		if (conn->ssl_ctx == NULL)
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to accept incoming connection, failed to create SSL context. Context creator returned NULL pointer");
		else 
			myqtt_log (MYQTT_LEVEL_CRITICAL, "there was an error while setting certificate file into the SSL context, unable to start TLS. Failure found at SSL_CTX_use_certificate_file function. Tried certificate file: %s", 
				   certificate);
		
		/* dump error stack */
		myqtt_conn_shutdown (conn);
		
		return axl_false;
	} /* end if */
	
	myqtt_log (MYQTT_LEVEL_DEBUG, "Using certificate key: %s", key);
	if (SSL_CTX_use_PrivateKey_file (conn->ssl_ctx, key, SSL_FILETYPE_PEM) != 1) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, 
			   "there was an error while setting private file into the SSl context, unable to start TLS. Failure found at SSL_CTX_use_PrivateKey_file function. Tried private file: %s", 
			   key);
		/* dump error stack */
		myqtt_conn_shutdown (conn);

		return axl_false;
	}
	
	/* check for private key and certificate file to match. */
	if (! SSL_CTX_check_private_key (conn->ssl_ctx)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, 
			   "seems that certificate file and private key doesn't match!, unable to start TLS. Failure found at SSL_CTX_check_private_key function. Used certificate %s, and key: %s",
			   certificate, key);
		/* dump error stack */
		myqtt_conn_shutdown (conn);
		
		return axl_false;
	} /* end if */
	
	if (opts != NULL && ! opts->disable_ssl_verify) {
		myqtt_log (MYQTT_LEVEL_DEBUG, "Enabling certificate client peer verification from server");
		/** really, really ugly hack to let
		 * __myqtt_tls_ssl_verify_callback to be able to get
		 * access to the context required to drop some logs */
		__myqtt_tls_ssl_ctx_debug = ctx;
		SSL_CTX_set_verify (conn->ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, __myqtt_tls_ssl_verify_callback); 
		SSL_CTX_set_verify_depth (conn->ssl_ctx, 5);
	} /* end if */

	return axl_true;
}


int __myqtt_tls_server_sni_callback (SSL * ssl, int *ad, void *arg)
{
	MyQttConn   * conn       = (MyQttConn *) arg;
	MyQttConn   * listener;
	MyQttCtx    * ctx        = CONN_CTX (conn);
	const char  * serverName = SSL_get_servername (ssl, TLSEXT_NAMETYPE_host_name);
	MyQttTlsCtx * tls_ctx;
	SSL_CTX     * old_context;

	/* additional variables */
	char        * certificate = NULL;
	char        * key         = NULL;
	char        * chain       = NULL;

	if (serverName) {
		/* notify on serverName notified to let application level
		   handling this: maybe he/she does not want to accept this
		   serverName.  */
		myqtt_log (MYQTT_LEVEL_DEBUG, "Received SNI server side notification with serverName=%s", serverName);

		
		/* now try to locate a certificate with the provided name */
		tls_ctx = myqtt_ctx_get_data (ctx, TLS_CTX);
		if (tls_ctx == NULL) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to accept SNI indication, TLS MyQtt api seems to be disabled, myqtt_ctx_get_data () reported NULL");
			return SSL_TLSEXT_ERR_NOACK;
		} /* end if */
		
		if (tls_ctx && tls_ctx->tls_certificate_handler) 
			certificate = tls_ctx->tls_certificate_handler (ctx, conn, serverName, tls_ctx->tls_handler_data);
		if (tls_ctx && tls_ctx->tls_private_key_handler) 
			key         = tls_ctx->tls_private_key_handler (ctx, conn, serverName, tls_ctx->tls_handler_data);
		if (tls_ctx && tls_ctx->tls_chain_handler) 
			chain       = tls_ctx->tls_chain_handler (ctx, conn, serverName, tls_ctx->tls_handler_data);

		if (certificate && key) {
			/* get reference to old context */
			old_context = conn->ssl_ctx;

			/* create new ssl context to switch to */
			listener       = myqtt_conn_get_listener (conn);
			conn->ssl_ctx  = __myqtt_tls_conn_get_ssl_context (ctx, conn, listener ? listener->opts : NULL, axl_false);
			if (conn->ssl_ctx == NULL) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create SSL context (__myqtt_conn_get_ssl_context failed)");
				conn->ssl_ctx = old_context;
				myqtt_conn_shutdown (conn);
				return SSL_TLSEXT_ERR_NOACK;
			} /* end if */

			if (! __myqtt_tls_prepare_certificates (conn, listener ? listener->opts : NULL, certificate, key, chain, NULL)) {
				myqtt_log (MYQTT_LEVEL_CRITICAL, "Prepare certificates failed (__myqtt_tls_prepare_certificates), skiping connection accept");
				conn->ssl_ctx = old_context;
				myqtt_conn_shutdown (conn);
				return SSL_TLSEXT_ERR_NOACK;
			} /* end if */

			/* everything went ok, update references */
			SSL_set_SSL_CTX (conn->ssl, conn->ssl_ctx);
			
			/* release previous context by setting the new */
			myqtt_conn_set_data_full (conn, "__my:co:ssl-ctx", conn->ssl_ctx, NULL, (axlDestroyFunc) SSL_CTX_free);

			axl_free (certificate);
			axl_free (key);
			axl_free (chain);
		} /* end if */

		/* for now, set the serverName for this connection */
		myqtt_log (MYQTT_LEVEL_DEBUG, "Calling to update connection's serverName to: %s", serverName);
		myqtt_conn_set_server_name (conn, serverName);

	} /* end if */
		

	return SSL_TLSEXT_ERR_OK;
}

/** 
 * @internal Function that prepares the TLS/SSL negotiation for every
 * incoming connection accepted.
 */
void __myqtt_tls_accept_connection (MyQttCtx * ctx, MyQttConn * listener, MyQttConn * conn, MyQttConnOpts * opts, axlPointer user_data)
{
	const char * certificateFile  = NULL;
	const char * privateKey       = NULL;
	const char * chainCertificate = NULL;
	const char * caCertificate    = NULL;
	int          ssl_error;
	int          result;

	if (conn->pending_ssl_accept) {
		/* SSL already configured but pending to fully accept
		   connection by doing the entire handshake */
		myqtt_log (MYQTT_LEVEL_DEBUG, "Received connect over a connection (id %d) with TLS handshake pending to be finished, processing..",
			    conn->id);
		
		/* get ssl error */
		ssl_error = SSL_accept (conn->ssl);
		if (ssl_error == -1) {
			/* get error */
			ssl_error = SSL_get_error (conn->ssl, -1);
			
			myqtt_log (MYQTT_LEVEL_WARNING, "accept function have failed (for listener side) ssl_error=%d : dumping error stack..", ssl_error);
 
			switch (ssl_error) {
			case SSL_ERROR_WANT_READ:
			        myqtt_log (MYQTT_LEVEL_WARNING, "still not prepared to continue because read wanted conn-id=%d (%p, session %d)",
					   conn->id, conn, conn->session);
				return;
			case SSL_ERROR_WANT_WRITE:
			        myqtt_log (MYQTT_LEVEL_WARNING, "still not prepared to continue because write wanted conn-id=%d (%p)",
					   conn->id, conn);
				return;
			default:
				break;
			} /* end switch */

			/* TLS-fication process have failed */
			myqtt_log (MYQTT_LEVEL_CRITICAL, "there was an error while accepting TLS connection");
			myqtt_tls_log_ssl (ctx);
			myqtt_conn_shutdown (conn);
			return;
		} /* end if */

		/* ssl accept */
		conn->pending_ssl_accept = axl_false;
		myqtt_conn_set_sock_block (conn->session, axl_false);

		result = SSL_get_verify_result (conn->ssl);

		myqtt_log (MYQTT_LEVEL_DEBUG, "Completed TLS operation from %s:%s (conn id %d, ssl veriry result: %d)",
			    conn->host, conn->port, conn->id, (int) result);

		/* configure default handlers */
		conn->receive = __myqtt_tls_receive;
		conn->send    = __myqtt_tls_send;

		/* call to check post ssl checks after SSL finalization */
		if (ctx && ctx->post_ssl_check) {
			if (! ((MyQttSslPostCheck) ctx->post_ssl_check) (ctx, conn, conn->ssl_ctx, conn->ssl, ctx->post_ssl_check_data)) {
				/* TLS post check failed */
				myqtt_log (MYQTT_LEVEL_CRITICAL, "TLS/SSL post check function failed, dropping connection");
				myqtt_conn_shutdown (conn);
				return;
			} /* end if */
		} /* end if */

		/* set this connection has TLS ok */

		/* reached this point, ensure tls is enabled on this
		 * session */
		conn->tls_on = axl_true;

		/* remove preread handler */
		conn->preread_handler = NULL;
		conn->preread_user_data = NULL;
		return;
	}

	/* overwrite connection options with listener's options: very TLS especific */
	if (listener->opts)
		opts = listener->opts;
	
	/* 1) GET FROM OPTIONS: detect here if we have certificates provided through options */
	myqtt_log (MYQTT_LEVEL_DEBUG, "Starting TLS process, options=%p, listener=%p", opts, listener);
	
	if (opts) {
		certificateFile = opts->certificate;
		privateKey      = opts->private_key;
	} /* end if */

	if (certificateFile == NULL || privateKey == NULL) {
		
		/* 2) GET FROM LISTENER: get references to currently configured certificate file */
		certificateFile = listener->certificate;
		privateKey      = listener->private_key;
		if (certificateFile == NULL || privateKey == NULL) {
			/* 3) GET FROM STORE: check if the
			 * certificate is already installed */
			/* find a certicate according to the user name
			   used or remote IP  */
		}
	} /* end if */
	
	/* check certificates and private key */
	if (certificateFile == NULL || privateKey == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to accept secure connection, certificate file %s and/or key file %s isn't defined",
			    certificateFile ? certificateFile : "<not defined>", 
			    privateKey ? privateKey : "<not defined>");
		myqtt_conn_shutdown (conn);
			
		return;
	} /* end if */
	
	/* init ssl ciphers and engines */
	if (! myqtt_tls_init (ctx)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Unable to accept TLS connection, myqtt_tls_init () failed");
		return;
	} /* end if */
	
	/* now configure chainCertificate */
	if (opts && opts->chain_certificate)
		chainCertificate = opts->chain_certificate;
	else if (listener->chain_certificate) 
		chainCertificate = listener->chain_certificate;
	
	/* create ssl context */
	conn->ssl_ctx  = __myqtt_tls_conn_get_ssl_context (ctx, conn, listener->opts, axl_false);
	if (conn->ssl_ctx == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to create SSL context (__myqtt_conn_get_ssl_context failed)");
		return;
	} /* end if */
	myqtt_conn_set_data_full (conn, "__my:co:ssl-ctx", conn->ssl_ctx, NULL, (axlDestroyFunc) SSL_CTX_free);

	/* configure SNI callback */
	SSL_CTX_set_tlsext_servername_callback (conn->ssl_ctx, __myqtt_tls_server_sni_callback);	
	SSL_CTX_set_tlsext_servername_arg      (conn->ssl_ctx, conn); 

	if (opts && opts->ca_certificate) 
		caCertificate = NULL;
	
	/* Configure ca certificate in the case it is defined */
	if (! __myqtt_tls_prepare_certificates (conn, opts, certificateFile, privateKey, chainCertificate, caCertificate)) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Prepare certificates failed (__myqtt_tls_prepare_certificates), skiping connection accept");
		return;
	} /* end if */

	/* create SSL context */
	conn->ssl = SSL_new (conn->ssl_ctx);       
	if (conn->ssl == NULL) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "error while creating TLS transport, SSL_new (%p) returned NULL", conn->ssl_ctx);
		myqtt_conn_shutdown (conn);
			
		return;
	} /* end if */

	/* configure release */
	myqtt_conn_set_data_full (conn, "__my:co:ssl", conn->ssl, NULL, (axlDestroyFunc) SSL_free);
	
	/* set the file descriptor */
	SSL_set_fd (conn->ssl, conn->session);
	
	/* don't complete here the operation but flag it as
	 * pending */
	conn->pending_ssl_accept = axl_true;
	myqtt_conn_set_sock_block (conn->session, axl_false);
	
	myqtt_log (MYQTT_LEVEL_DEBUG, "Prepared TLS session to be activated on next reads (conn id %d)", conn->id);

	return;
}

/** 
 * @brief Allows to start a MQTT server on the provided local host
 * address and port running secure TLS protocol (secure-mqtt).
 *
 * <b>Important note:</b> you must call to \ref myqtt_storage_set_path
 * to define the path first before creating any listener. This is
 * because creating a listener activates all server side code which
 * among other things includes the storage loading (client
 * subscriptions, offline publishing, etc). In the case direction,
 * once the storage path is loaded it cannot be changed after
 * restarting the particular context used in this operation (\ref MyQttCtx).
 *
 * This function works the same as \ref myqtt_listener_new but
 * providing TLS support.
 *
 * @param ctx The context where the operation takes place.
 *
 * @param host The local host address to list for incoming connections. 
 *
 * @param port The local port to listen on.
 *
 * @param opts Optional connection options to modify default behaviour.
 *
 * @param on_ready Optional on ready notification handler that gets
 * called when the listener is created or a failure was
 * found. Providing this handler makes this function to not block the
 * caller.
 *
 * @param user_data Optional user defined pointer that is passed into
 * the on_ready function (in the case the former is defined too).
 *
 * @return A newly created connection listener reference (\ref
 * MyQttConn). Use \ref myqtt_conn_is_ok to check listener was created
 * without errors.
 */
MyQttConn       * myqtt_tls_listener_new                (MyQttCtx             * ctx,
							 const char           * host, 
							 const char           * port, 
							 MyQttConnOpts        * opts,
							 MyQttListenerReady     on_ready, 
							 axlPointer             user_data)
{
	MyQttNetTransport transport;
	
	/* get transport we have to use */
	transport = __myqtt_conn_detect_transport (ctx, host);

	/* create listener */
	return __myqtt_listener_new_common (ctx, host, __myqtt_listener_get_port (port), axl_true, -1, opts, on_ready, transport, __myqtt_tls_accept_connection, NULL, user_data);
}

/** 
 * @brief Creates a new TCP/IPv6 MyQtt Listener accepting incoming
 * connections on the given <b>host:port</b> configuration running TLS
 * protocol (secure-mqtt).
 *
 * Take a look to \ref myqtt_listener_new for additional
 * information. This functions provides same features plus IPv6
 * support.
 *
 * <b>Important note:</b> you must call to \ref myqtt_storage_set_path
 * to define the path first before creating any listener. This is
 * because creating a listener activates all server side code which
 * among other things includes the storage loading (client
 * subscriptions, offline publishing, etc). In the case direction,
 * once the storage path is loaded it cannot be changed after
 * restarting the particular context used in this operation (\ref MyQttCtx).
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param host The host to listen on.
 *
 * @param port The port to listen on.
 *
 * @param opts Optional connection options to change default behviour.
 *
 * @param on_ready A optional callback to get a notification when
 * myqtt listener is ready to accept requests.
 *
 * @param user_data A user defined pointer to be passed in to
 * <i>on_ready</i> handler.
 *
 * @return The listener connection created (represented by a \ref
 * MyQttConn reference). You must use \ref
 * myqtt_conn_is_ok to check if the server was started.
 * 
 * See additional notes at \ref myqtt_listener_new
 */
MyQttConn       * myqtt_tls_listener_new6               (MyQttCtx             * ctx,
							 const char           * host, 
							 const char           * port, 
							 MyQttConnOpts        * opts,
							 MyQttListenerReady     on_ready, 
							 axlPointer             user_data)
{
	/* create listener */
	return __myqtt_listener_new_common (ctx, host, __myqtt_listener_get_port (port), axl_true, -1, opts, on_ready, MYQTT_IPv6, __myqtt_tls_accept_connection, NULL, user_data);
}

/** 
 * @brief Allows to configure the TLS certificate and key to be used
 * on the provided connection.
 *
 * @param listener The listener that is going to be configured with
 * the providing certificate and key.
 *
 * @param certificate The path to the public certificate file (PEM
 * format) to be used for every TLS connection received under the
 * provided listener.
 *
 * @param private_key The path to the key file (PEM format) to be used for
 * every TLS connection received under the provided listener.
 *
 * @param chain_file The path to additional chain certificates (PEM
 * format). You can safely pass here a NULL value.
 *
 * @return axl_true if the certificates were configured, otherwise
 * axl_false is returned.
 */
axl_bool           myqtt_tls_set_certificate (MyQttConn  * listener,
					      const char * certificate,
					      const char * private_key,
					      const char * chain_file)
{
	FILE     * handle;
	MyQttCtx * ctx;

	if (! listener || ! certificate || ! private_key)
		return axl_false;

	/* reference to certificate ctx */
	ctx = listener->ctx;
	
	/* check certificate file */
	handle = fopen (certificate, "r");
	if (! handle) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to open certificate file from %s", certificate);
		return axl_false;
	} /* end if */
	fclose (handle);

	/* check private file */
	handle = fopen (private_key, "r");
	if (! handle) {
		myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to open private key file from %s", private_key);
		return axl_false;
	} /* end if */
	fclose (handle);

	if (chain_file) {
		/* check private file */
		handle = fopen (chain_file, "r");
		if (! handle) {
			myqtt_log (MYQTT_LEVEL_CRITICAL, "Failed to open chain certificate file from %s", private_key);
			return axl_false;
		} /* end if */
		fclose (handle);
	} /* end if */

	/* copy certificates to be used */
	listener->certificate   = axl_strdup (certificate);
	myqtt_ctx_set_data_full (ctx, axl_strdup_printf ("%p", listener->certificate), listener->certificate,
				 axl_free, axl_free);
	listener->private_key   = axl_strdup (private_key);
	myqtt_ctx_set_data_full (ctx, axl_strdup_printf ("%p", listener->private_key), listener->private_key,
				 axl_free, axl_free);
	if (chain_file) {
		listener->chain_certificate = axl_strdup (chain_file);
		myqtt_ctx_set_data_full (ctx, axl_strdup_printf ("%p", listener->chain_certificate), listener->chain_certificate,
					 axl_free, axl_free);
	} /* end if */
	    
	myqtt_log (MYQTT_LEVEL_DEBUG, "Configured certificate: %s, key: %s, for conn id: %d",
		    listener->certificate, listener->private_key, listener->id);

	/* certificates configured */
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
	if (conn == NULL)
		return axl_false;
	return conn->tls_on;
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
 * function returns NULL if TLS is not activated or was not
 * activated on the connection.
 */
axlPointer         myqtt_tls_get_ssl_object             (MyQttConn * connection)
{
	/* return the ssl object which is stored under the key:
	 * ssl-data:ssl */
	if (connection == NULL)
		return NULL;
	return connection->ssl;
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
	if (peer_cert == NULL) 
		return NULL;

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

	/* release certificate */
	X509_free (peer_cert);

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

