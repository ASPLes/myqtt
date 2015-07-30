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
#include <myqttd.h>
#include <myqttd-ctx-private.h>

#if defined(DEFINE_MKSTEMP_PROTO)
int mkstemp(char *template);
#endif

/** 
 * \defgroup myqttd_support MyQttd Support : support functions and useful APIs
 */

/** 
 * \addtogroup myqttd_support
 * @{
 */


#define write_and_check(str, len) do {					\
	if (write (temp_file, str, len) != len) {			\
		error ("Unable to write expected string: %s", str);     \
		close (temp_file);					\
		axl_free (temp_name);					\
		axl_free (str_pid);					\
		return NULL;						\
	}								\
} while (0)

/** 
 * @brief Allows to get process backtrace (including all threads) of
 * the given process id.
 *
 * @param ctx The context where the operation is implemented.
 *
 * @param pid The process id for which the backtrace is requested. Use
 * getpid () to get current process id.
 *
 * @return A newly allocated string containing the path to the file
 * where the backtrace was generated or NULL if it fails.
 */
char          * myqttd_support_get_backtrace (MyQttdCtx * ctx, int pid)
{
#if defined(AXL_OS_UNIX)
	FILE               * file_handle;
	int                  temp_file;
	char               * temp_name;
	char               * str_pid;
	char               * command;
	int                  status;
	char               * backtrace_file = NULL;

	temp_name = axl_strdup ("/tmp/myqtt-backtrace.XXXXXX");
	temp_file = mkstemp (temp_name);
	if (temp_file == -1) {
		error ("Bad signal found but unable to create gdb commands file to feed gdb");
		return NULL;
	} /* end if */

	str_pid = axl_strdup_printf ("%d", getpid ());
	if (str_pid == NULL) {
		error ("Bad signal found but unable to get str pid version, memory failure");
		close (temp_file);
		return NULL;
	}
	
	/* write personalized gdb commands */
	write_and_check ("attach ", 7);
	write_and_check (str_pid, strlen (str_pid));

	axl_free (str_pid);
	str_pid = NULL;

	write_and_check ("\n", 1);
	write_and_check ("set pagination 0\n", 17);
	write_and_check ("thread apply all bt\n", 20);
	write_and_check ("quit\n", 5);
	
	/* close temp file */
	close (temp_file);
	
	/* build the command to get gdb output */
	while (1) {
		backtrace_file = axl_strdup_printf ("%s/myqtt-backtrace.%d.gdb", myqttd_runtime_datadir (ctx), time (NULL));
		file_handle    = fopen (backtrace_file, "w");
		if (file_handle == NULL) {
			msg ("Changing path because path %s is not allowed to the current uid=%d", backtrace_file, getuid ());
			axl_free (backtrace_file);
			backtrace_file = axl_strdup_printf ("%s/myqtt-backtrace.%d.gdb", myqttd_runtime_tmpdir (ctx), time (NULL));
		} else {
			fclose (file_handle);
			msg ("Checked that %s is writable/readable for the current usid=%d", backtrace_file, getuid ());
			break;
		} /* end if */

		/* check path again */
		file_handle    = fopen (backtrace_file, "w");
		if (file_handle == NULL) {
			error ("Failed to produce backtrace, alternative path %s is not allowed to the current uid=%d", backtrace_file, getuid ());
			axl_free (backtrace_file);
			return NULL;
		}
		fclose (file_handle);
		break; /* reached this point alternative path has worked */
	} /* end while */

	if (backtrace_file == NULL) {
		error ("Failed to produce backtrace, internal reference is NULL");
		return NULL;
	}

	/* place some system information */
	command  = axl_strdup_printf ("echo \"MyQttd backtrace at `hostname -f`, created at `date`\" > %s", backtrace_file);
	status   = system (command);
	msg ("Running: %s, exit status: %d", command, status);
	axl_free (command);

	/* get domain id */
	if (ctx->child == NULL) 
		command  = axl_strdup_printf ("echo \"Failure found at main process.\" >> %s", backtrace_file);
	else {
		/* get domain associated to child process */
		command   = axl_strdup_printf ("echo \"Failure found at child process.\" >> %s", backtrace_file);
	}
	status   = system (command);
	msg ("Running: %s, exit status: %d", command, status);
	axl_free (command);

	/* get place some pid information */
	command  = axl_strdup_printf ("echo -e 'Process that failed was %d. Here is the backtrace:\n--------------' >> %s", getpid (), backtrace_file);
	status   = system (command);
	msg ("Running: %s, exit status: %d", command, status);
	axl_free (command);
	
	/* get backtrace */
	command  = axl_strdup_printf ("gdb -x %s >> %s", temp_name, backtrace_file);
	status   = system (command);
	msg ("Running: %s, exit status: %d", command, status);

	/* remove gdb commands */
	unlink (temp_name);
	axl_free (temp_name);
	axl_free (command);

	/* return backtrace file created */
	return backtrace_file;

#elif defined(AXL_OS_WIN32)
	error ("Backtrace for Windows not implemented..");
	return NULL;
#endif			
}

axl_bool myqttd_support_smtp_send_receive_reply_and_check (MyQttdCtx     * ctx,
							   MYQTT_SOCKET    conn, 
							   char          * buffer, 
							   int             buffer_size, 
							   const char    * error_message)
{
	int read_bytes;

	/* clear buffer received */
	memset (buffer, 0, buffer_size);

	/* receive content */
	read_bytes = recv (conn, buffer, buffer_size, 0);
	if (read_bytes <= 0) {
		error ("Failed to receive reply SMTP content. %s", error_message);
		return axl_false;
	} /* end if */

	buffer[read_bytes - 1] = 0;

	if (! axl_memcmp (buffer, "250", 3) && ! axl_memcmp (buffer, "220", 3) && ! axl_memcmp (buffer, "354", 3)) {
		error ("Received negative SMTP reply: %s", buffer);
		myqtt_close_socket (conn);
		return axl_false;
	}

	msg ("Received afirmative SMTP content: %s", buffer);
	return axl_true;
}

/** 
 * @brief Allows to send a mail message through the provided smtp
 * server and port, with the provided content.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param mail_from The mail from value to configure on this mail
 * message. If NULL is provided, "myqttd@localdomain.local" will
 * be used.
 *
 * @param mail_to The destination address value to configure on this
 * mail message. This value is not optional and must be defined and
 * pointing to a right account.
 *
 * @param subject The message subject to be configured. This value is
 * optional, if not configured no subject will be placed.
 *
 * @param body The message body to be configured. This value is
 * optional, if not configured no subject will be placed.
 *
 * @param body_file Optional reference to a file that contains the
 * body of the message.
 *
 * @param smtp_server The location of the smtp server. If NULL is passed, localhost will be used.
 *
 * @param smtp_port The port location of the smtp server. If NULL is passed, 25 will be used.
 *
 * @return axl_true in the case the mail message was successfully sent
 * otherwise axl_false is returned.
 */
axl_bool        myqttd_support_smtp_send (MyQttdCtx * ctx, 
					      const char    * mail_from,
					      const char    * mail_to,
					      const char    * subject,
					      const char    * body,
					      const char    * body_file,
					      const char    * smtp_server,
					      const char    * smtp_port)
{
	MYQTT_SOCKET    conn;
	axlError      * err = NULL;
	char            buffer[1024];
	FILE          * body_ffile;
	int             bytes_read, bytes_sent;

	if (ctx == NULL || mail_to == NULL)
		return axl_false;

	/* set a default mail_from */
	if (mail_from == NULL)
		mail_from = "myqttd@localdomain.local";

	/* create connection */
	conn = myqtt_conn_sock_connect (MYQTTD_MYQTT_CTX (ctx), 
					smtp_server ? smtp_server : "localhost",
					smtp_port ? smtp_port : "25",
					NULL, &err);
	if (conn == -1) {
		error ("Unable to connect to smtp server %s:%s, error was: %s",
		       smtp_server ? smtp_server : "localhost",
		       smtp_port ? smtp_port : "25",
		       axl_error_get (err));
		axl_error_free (err);
	} /* end if */

	/* read greetings */
	if (! myqttd_support_smtp_send_receive_reply_and_check (ctx, conn, buffer, 1024, 
								    "Failed to receive initial SMTP greetings"))
		return axl_false;

	/* ok, now write content */
	if (send (conn, "mail from: ", 11, 0) != 11) {
		error ("Unable to send mail message, failed to send mail from content..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	if (send (conn, mail_from, strlen (mail_from), 0) != strlen (mail_from)) {
		error ("Unable to send mail message, failed to send mail from content (address)..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	/* send termination */
	if (send (conn, "\r\n", 2, 0) != 2) {
		error ("Unable to send mail message, failed to send mail from content (address)..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	/* read reply */
	if (! myqttd_support_smtp_send_receive_reply_and_check (ctx, conn, buffer, 1024, 
								    "Failed to receive mail from confirmation"))
		return axl_false;

	if (send (conn, "rcpt to: ", 9, 0) != 9) {
		error ("Unable to send mail message, failed to send rcpt to content..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	if (send (conn, mail_to, strlen (mail_to), 0) != strlen (mail_to)) {
		error ("Unable to send mail message, failed to send rcpt to content (address)..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	/* send termination */
	if (send (conn, "\r\n", 2, 0) != 2) {
		error ("Unable to send mail message, failed to send mail from content (address)..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	/* read reply */
	if (! myqttd_support_smtp_send_receive_reply_and_check (ctx, conn, buffer, 1024, 
								    "Failed to receive rcpt to confirmation"))
		return axl_false;

	/* init data section */
	if (send (conn, "data\n", 5, 0) != 5) {
		error ("Unable to send mail message, failed to send data section..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	/* read reply */
	msg ("SMTP: data command sent..");
	if (! myqttd_support_smtp_send_receive_reply_and_check (ctx, conn, buffer, 1024, 
								    "Failed to receive data start section confirmation"))
		return axl_false;

	/* send To: content */
	if (send (conn, "To: ", 4, 0) != 4) {
		error ("Unable to send mail message, failed to send To: content..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	if (send (conn, mail_to, strlen (mail_to), 0) != strlen (mail_to)) {
		error ("Unable to send mail message, failed to send rcpt to content (address)..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	/* send termination */
	if (send (conn, "\r\n", 2, 0) != 2) {
		error ("Unable to send mail message, failed to send mail from content (address)..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	/* send From: content */
	if (send (conn, "From: ", 6, 0) != 6) {
		error ("Unable to send mail message, failed to send From: content..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	if (send (conn, mail_from, strlen (mail_from), 0) != strlen (mail_from)) {
		error ("Unable to send mail message, failed to send rcpt to content (address)..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	/* send termination */
	if (send (conn, "\r\n", 2, 0) != 2) {
		error ("Unable to send mail message, failed to send mail from content (address)..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */
	
	/* check subject */
	if (subject) {
	        msg ("SMTP: subject command sent..");

		/* send subject content */
		if (send (conn, "Subject: ", 9, 0) != 9) {
			error ("Unable to send mail message, failed to send subject section..");
			myqtt_close_socket (conn);
			return axl_false;
		} /* end if */

		/* send the subject it self */
		if (send (conn, subject, strlen (subject), 0) != strlen (subject)) {
			error ("Unable to send mail message, failed to send subject content..");
			myqtt_close_socket (conn);
			return axl_false;
		} /* end if */


		/* send termination */
		if (send (conn, "\r\n", 2, 0) != 2) {
			error ("Unable to send mail message, failed to send mail from content (address)..");
			myqtt_close_socket (conn);
			return axl_false;
		} /* end if */
	}

	/* now send body */
	if (body) {
	        msg ("SMTP: body command sent..");

		if (send (conn, body, strlen (body), 0) != strlen (body)) {
			error ("Unable to send mail message, failed to send body content..");
			myqtt_close_socket (conn);
			return axl_false;
		} /* end if */

		/* send termination */
		if (send (conn, "\r\n", 2, 0) != 2) {
			error ("Unable to send mail message, failed to send mail from content (address)..");
			myqtt_close_socket (conn);
			return axl_false;
		} /* end if */
	}

	/* check for body from file */
	if (body_file) {
   	        msg ("SMTP: sending body file %s..", body_file);

		body_ffile = fopen (body_file, "r");
		if (body_ffile) {
			/* read and send content */
			bytes_read = fread (buffer, 1, 1024, body_ffile);
			while (bytes_read > 0) {

				/* send content */
				bytes_sent = send (conn, buffer, bytes_read, 0);
				if (bytes_sent != bytes_read) {
					error ("Unable to send mail message, found a failure while sending content from file (sent %d != requested %d)..",
					       bytes_sent, bytes_read);
					fclose (body_ffile);
					myqtt_close_socket (conn);
					return axl_false;
				} /* end if */

				/* read next content */
				bytes_read = fread (buffer, 1, 1024, body_ffile);
			} /* end while */
			fclose (body_ffile);

			/* send termination */
			if (send (conn, "\r\n", 2, 0) != 2) {
			        error ("Unable to send mail message, failed to send mail from content (address)..");
				myqtt_close_socket (conn);
				return axl_false;
			} /* end if */
		} /* end if */
	} /* end if */

	/* send termination */
	if (send (conn, ".\r\n", 3, 0) != 3) {
		error ("Unable to send mail message, failed to send termination message..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */

	/* read reply */
	msg ("(.) finished command sent, now wait for reply..");
	if (! myqttd_support_smtp_send_receive_reply_and_check (ctx, conn, buffer, 1024, 
								    "Failed to receive end message confirmation"))
		return axl_false;

	/* send termination */
	msg ("Now sending quit..");
	if (send (conn, "quit\r\n", 6, 0) != 6) {
		error ("Unable to send mail message, failed to send termination message..");
		myqtt_close_socket (conn);
		return axl_false;
	} /* end if */
	myqtt_close_socket (conn);

	return axl_true;
}

/** 
 * @brief Allows to send a SMTP message using the configuration found
 * on the provided smtp_conf declaration. This smtp_conf declaration
 * is found at the myqttd configuration file. See \ref
 * myqttd_smtp_notifications
 *
 * @param ctx The myqttd context where the operation will be
 * implemented.
 *
 * @param smtp_conf_id The string identifying the smtp configuration (id
 * declaration inside <smtp-server> node) or NULL. If NULL is used,
 * then the first smtp server with is-default=yes declared is used.
 *
 * @param subject Optional subject to be configured on mail body
 * message.
 * 
 * @param body The message body to be configured. If NULL is provided
 * no body will be sent.
 *
 * @param body_file Optional reference to a file that contains the
 * body of the message.
 *
 * @return axl_true if the mail message was submited or axl_false if
 * something failed.
 */
axl_bool        myqttd_support_simple_smtp_send (MyQttdCtx * ctx,
						     const char    * smtp_conf_id,
						     const char    * subject,
						     const char    * body,
						     const char    * body_file)
{
	axlNode * node;
	axlNode * default_node;

	if (ctx == NULL)
		return axl_false;

	/* find smtp mail notification conf */
	node         = axl_doc_get (myqttd_config_get (ctx), "/myqtt/global-settings/notify-failures/smtp-server");
	default_node = NULL;
	while (node) {
		/* check for declaration with the smtp conf requested */
		if (HAS_ATTR_VALUE (node, "id", smtp_conf_id))
			break;

		/* check for default node declaration */
		if (HAS_ATTR_VALUE (node, "is-default", "yes") && default_node == NULL)
			default_node = node;

		/* node not found, go next */
		node = axl_node_get_next_called (node, "smtp-server");
	} /* end while */

	/* set to default node found (if any) when node is null */
	if (node == NULL)
		node = default_node;

	/* check if the smtp configuration was found */
	if (node == NULL) {
		error ("Failed to send mail notification, unable to find default smtp configuration or smtp configuration associated to id '%s'",
		       smtp_conf_id ? smtp_conf_id : "NULL");
		return axl_false;
	}
	
	/* now use SMTP send values */
	return myqttd_support_smtp_send (ctx, ATTR_VALUE (node, "mail-from"), 
					 ATTR_VALUE (node, "mail-to"), subject, body, body_file, 
					 ATTR_VALUE (node, "server"), ATTR_VALUE (node, "port"));
}

/** 
 * @brief Allows to get what is the check mode that should be
 * followed. It is a pattern across myqtt implementation that the
 * check to select a domain or to validate a username/client_id to
 * consider them in a particular order, that is:
 *
 * - check mode 3: if username and client_id is defined, then check
 *   both at the same type (check_mode 3). This means that both values
 *   must be considered as a single unit.
 *
 * - check mode 2: if just client_id is defined (but not username)
 *   then then the function will report check mode 2.
 *
 * - check mode 1: and finally, if just username is defined, the
 *   function will report check mode 1.
 *
 * @param username Optional string representing the username
 *
 * @param client_id Optional string representing the client_id
 *
 * @return If nothing is defined, the function returns check mode 0.
 */
int             myqttd_support_check_mode (const char * username, const char * client_id)
{
	if (client_id != NULL && username != NULL && strlen (client_id) > 0 && strlen (username) > 0)
		return 3;
	else if (client_id != NULL && strlen (client_id) > 0)
		return 2;
	else if (username != NULL && strlen (username) > 0)
		return 1;

	/* report 0 username and client id is not defined */
	return 0;
}

/** 
 * @}
 */
