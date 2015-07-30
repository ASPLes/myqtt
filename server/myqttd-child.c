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
#include <myqttd-child.h>
#include <syslog.h>

/* local private include */
#include <myqttd-ctx-private.h>

/** 
 * @internal Creates a object that will represents a child object and
 * initialize internal data structures for its function.
 */
MyQttdChild * myqttd_child_new (MyQttdCtx * ctx)
{
	MyQttdChild * result;
	char            * temp_dir;
	struct timeval    now;

	result        = axl_new (MyQttdChild, 1);
	/* check allocation */
	if (result == NULL)
		return NULL;

	/* get current time */
	gettimeofday (&now, NULL);

	/* create socket path: socket used to transfer file descriptors from parent to child */
	result->socket_control_path = axl_strdup_printf ("%s%s%s%s%p%d%d.myqttd",
							 myqttd_runtime_datadir (ctx),
							 MYQTT_FILE_SEPARATOR,
							 "myqttd",
							 MYQTT_FILE_SEPARATOR,
							 result, now.tv_sec, now.tv_usec);
	/* check result */
	if (result->socket_control_path == NULL) {
		axl_free (result);
		return NULL;
	} /* end if */

	/* now check check base dir for socket control path exists */
	temp_dir = myqttd_base_dir (result->socket_control_path);
	if (temp_dir && ! myqtt_support_file_test (temp_dir, FILE_EXISTS)) {
		/* base directory having child socket control do not exists */
		wrn ("run time directory %s do not exists, creating..", temp_dir);
		if (! myqttd_create_dir (temp_dir)) {
			/* call to finish references */
			myqttd_child_unref (result);

			error ("Unable to create directory to hold child socket connection, unable to create child process..");
			axl_free (temp_dir);
			return NULL;
		} /* end if */
	} /* end if */
	/* free temporal directory */
	axl_free (temp_dir);
	
	/* set context */
	result->ctx   = ctx;

	/* set default reference counting */
	result->ref_count = 1;
	myqtt_mutex_create (&result->mutex);

	return result;
}

/** 
 * @brief Allows to a acquire a reference to the provided object.
 *
 * @param child The child object to get a reference.
 *
 * @return axl_true if the reference was acquired otherwise, axl_false
 * is returned.
 */
axl_bool              myqttd_child_ref (MyQttdChild * child)
{
	if (child == NULL || child->ref_count == 0)
		return axl_false;

	/* get mutex */
	myqtt_mutex_lock (&child->mutex);

	child->ref_count++;

	/* release */
	myqtt_mutex_unlock (&child->mutex);

	return axl_true;
}


/** 
 * @brief Allows to decrease reference counting and deallocating
 * resources when reference counting reaches 0.
 *
 * @param child A reference to the child to decrease reference counting.
 */
void              myqttd_child_unref (MyQttdChild * child)
{
	if (child == NULL || child->ref_count == 0)
		return;

	/* get mutex */
	myqtt_mutex_lock (&child->mutex);

	child->ref_count--;

	if (child->ref_count != 0) {
		/* release */
		myqtt_mutex_unlock (&child->mutex);
		return;
	}

#if defined(AXL_OS_UNIX)
	/* unlink (child->socket_control_path);*/
	axl_free (child->socket_control_path);
	child->socket_control_path = NULL;
	myqtt_close_socket (child->child_connection);
	axl_freev (child->init_string_items);
#endif

	/* finish child conn loop */
	myqttd_loop_close (child->child_conn_loop, axl_true);

	/* release server name */
	axl_free (child->serverName);

	/* destroy mutex */
	myqtt_mutex_destroy (&child->mutex);

	axl_free (child);

	return;
}

/** 
 * @brief Allows to get the serverName string included in the domain configuration that triggered this child.
 *
 * Every child process created by myqttd is because there is a domain
 * configuration that instructs it. In this context, this function
 * allows to get the serverName value of the provided domain
 * configuration that triggered the creation of this child. This
 * function is useful to get a reference to the serverName
 * configuration that triggered this child to allow additional
 * configurations that may be necessary during child initialization or
 * for its later processing.
 *
 * @param ctx A reference to the myqttd context that is running a
 * child.
 *
 * @return A reference to the serverName expression configured in the
 * domain or NULL if it fails. 
 */
const char      * myqttd_child_get_serverName (MyQttdCtx * ctx)
{
	if (ctx == NULL || ctx->child == NULL)
		return NULL;
	return ctx->child->serverName;
}

#define MYQTTD_CHILD_CONF_LOG(logw,logr,level) do {			\
	log_descriptor = atoi (logw);                                   \
	if (log_descriptor > 0) {                                       \
		myqttd_log_configure (ctx, level, log_descriptor);  \
		myqtt_close_socket (atoi (logr));                      \
	}                                                               \
} while(0)

axl_bool __myqttd_child_post_init_openlogs (MyQttdCtx  * ctx, 
						char          ** items)
{
	int log_descriptor;

	if (ctx->use_syslog) {
		openlog ("myqttd", LOG_PID, LOG_DAEMON);
		return axl_true;
	} /* end if */

	/* configure general_log */
	MYQTTD_CHILD_CONF_LOG (items[2], items[1], LOG_REPORT_GENERAL);

	/* configure error_log */
	MYQTTD_CHILD_CONF_LOG (items[4], items[3], LOG_REPORT_ERROR);

	/* configure access_log */
	MYQTTD_CHILD_CONF_LOG (items[6], items[5], LOG_REPORT_ACCESS);

	/* configure myqtt_log */
	MYQTTD_CHILD_CONF_LOG (items[8], items[7], LOG_REPORT_MYQTT);

	return axl_true;
}

/** 
 * @internal Function used to recover child status from the provided
 * init string.
 */
axl_bool          myqttd_child_build_from_init_string (MyQttdCtx * ctx, 
							   const char    * socket_control_path)
{
	MyQttdChild   * child;
	char                child_init_string[4096];
	char                child_init_length[30];
	int                 iterator;
	int                 size;
	axl_bool            found;
	char             ** items;

	/* create empty child object */
	child = axl_new (MyQttdChild, 1);

	/* set socket control path */
	child->socket_control_path = axl_strdup (socket_control_path);

	/* connect to the socket control path */
	if (! __myqttd_process_create_parent_connection (child)) {
		error ("CHILD: unable to create parent control connection, finishing child");
		return axl_false;
	}

	/* get the amount of bytes to read from parent */
	iterator = 0;
	found    = axl_false;
	while (iterator < 30 && recv (child->child_connection, child_init_length + iterator, 1, 0) == 1) {
		if (child_init_length[iterator] == '\n') {
			found = axl_true;
			break;
		} /* end if */
		
		/* next position */
		iterator++;
	}

	if (! found) {
		error ("CHILD: expected to find init child string length indication termination (\\n) but not found, finishing child");
		return axl_false;
	}
	child_init_length[iterator + 1] = 0;
	size = atoi (child_init_length);
	msg ("CHILD: init child string length is: %d", size);

	if (size > 4095) {
		error ("CHILD: unable to process child init string, found length indicator bigger than 4095");
		return axl_false;
	}

	/* now receive child init string */
	size = recv (child->child_connection, child_init_string, size, 0);
	if (size > 0)
		child_init_string[size] = 0;
	else {
		error ("CHILD: received empty or wrong init child string from parent, finishing child");
		return axl_false;
	}

	msg ("CHILD: starting child with init string: %s", child_init_string);

	/* build items */
	child->init_string_items = axl_split (child_init_string, 1, ";_;");
	if (child->init_string_items == NULL)
		return axl_false;

	/* set child on context */
	ctx->child = child;
	child->ctx = ctx;

	/* get a reference to the serverName this child represents */
	items = axl_split (child->init_string_items[11], 1, ";-;");
	if (items == NULL)
		return axl_false;
	child->serverName = axl_strdup (items[5]);
	axl_freev (items);
	msg ("CHILD: domain serverName for this child: %s", child->serverName);

	/* set default reference counting */
	child->ref_count = 1;
	myqtt_mutex_create (&child->mutex);	

	/* open logs */
	if (! __myqttd_child_post_init_openlogs (ctx, child->init_string_items)) 
		return axl_false;

	/* return child structure properly recovered */
	return axl_true;
}

axl_bool __myqttd_child_post_init_register_conn (MyQttdCtx * ctx, const char * conn_socket, char * conn_status)
{
	MyQttConn * conn;

	msg ("CHILD: restoring connection to be handled at child, socket: %s, connection status: %s", conn_socket, conn_status);
	if (! (conn = __myqttd_process_handle_connection_received (ctx, ctx->child, atoi (conn_socket), conn_status + 1))) 
		return axl_false;

	return axl_true;
} 

/** 
 * @internal Function used to complete child startup.
 */
axl_bool          myqttd_child_post_init (MyQttdCtx * ctx)
{
	MyQttdChild     * child;
	int               len;
	axlPointer        def = NULL;

	/*** NOTE: indexes used for child->init_string_items[X] are defined inside
	 * myqttd_process_create_child.c, around line 1392 ***/
	msg ("CHILD: doing post init (recovering first connection)");

	/* get child reference */
	child = ctx->child;

	/* define domain */
	msg ("Setting id for child %s", child->init_string_items[10] ? child->init_string_items[10] : "<not defined>");
	if (def == NULL) {
		error ("Unable to find domain associated to id %d, unable to complete post init", atoi (child->init_string_items[10]));
		return axl_false;
	}

	/* create loop to watch child->child_connection */
	child->child_conn_loop = myqttd_loop_create (ctx);
	myqttd_loop_watch_descriptor (child->child_conn_loop, child->child_connection, 
					  myqttd_process_parent_notify, child, NULL);
	msg ("CHILD: started socket watch on (child_connection socket: %d)", child->child_connection);


	/* check if we have to restore the connection or skip this
	 * step */
	len = strlen (child->init_string_items[11]);
	msg ("CHILD: checking to skip connection store, conn_status=%s, last='%c'", 
	     child->init_string_items[11], child->init_string_items[11][len - 1]);
	if (child->init_string_items[11][len - 1] == '1') {
		msg ("CHILD: skiping connection restoring as indicated in conn_status (last position == '1'): %s", child->init_string_items[11]);
	} else {
		/* register connection handled now by child  */
		msg ("CHILD: restoring connection conn_socket=%s, conn_status=%s", child->init_string_items[0], child->init_string_items[11]);
		if (! __myqttd_child_post_init_register_conn (ctx, /* conn_socket */ child->init_string_items[0],
								  /* conn_status */ child->init_string_items[11])) {
			error ("CHILD: failed to register starting connection at child process, finishing..");
			return axl_false;
		} /* end if */
	} /* end if */
	msg ("CHILD: post init phase done, child running (myqtt.ctx refs: %d)", myqtt_ctx_ref_count (child->ctx->myqtt_ctx));
	return axl_true;
}

