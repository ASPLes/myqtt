/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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

#include <myqttd.h>
#include <myqttd-ctx-private.h>

#if defined(ENABLE_TLS_SUPPORT)
#include <myqtt-tls.h>
#endif

#if defined(ENABLE_WEBSOCKET_SUPPORT)
#include <myqtt-web-socket.h>
#endif

/* mysql flags */
#include <mysql.h>


/* use this declarations to avoid c++ compilers to mangle exported
 * names. */
BEGIN_C_DECLS

MyQttdCtx * ctx = NULL;
axlDoc    * modconfig = NULL;

int      __mod_auth_mysql_get_user_id (MyQttConn * conn)
{
	int value;
	value = PTR_TO_INT (myqtt_conn_get_data (conn, "mod:mysql:auth:user_id"));
	if (value > 0)
		return value;
	return -1; /* return default value when no value was found: user_id == -1 */
}

axl_bool __mod_auth_mysql_check_input (MyQttdCtx * ctx, const char * input)
{
	int iterator = 0;
	if (input == NULL || strlen (input) == 0)
		return axl_true; /* empty string, clean string */
	while (input [iterator]) {
		/* block string terminator */
		if (input[iterator] == '\'' || 
		    /* block string terminator */
		    input[iterator] == '"' || 
		    /* block sql comment */
		    input[iterator] == '-' || 
		    /* block sentence separator */
		    input[iterator] == ';' || 
		    /* block 1=1 */
		    input[iterator] == '=' || 
		    /* block separator that allows ' and 1=1 ', ' or 1=1' */
		    input[iterator] == ' ')
			return axl_false;
		
		/* next position */
		iterator++;
	}

	/* reached this point, string has no invalid characters */
	return axl_true;
}

axlPointer __mod_auth_mysql_get_by_name (MyQttdCtx * ctx, MYSQL_RES * result, MYSQL_ROW row, const char * row_name)
{
	MYSQL_FIELD     * field;
	unsigned      int iterator;
	unsigned      int num_fields;

	/* set field seek to restart mysql_fetch_field calls */
	mysql_field_seek (result, 0);

	/* get number of fields */
	num_fields = mysql_num_fields (result);
	iterator   = 0;
	
	while (iterator < num_fields) {
		/* get field */
		field = mysql_fetch_field (result);
		if (field && axl_casecmp (row_name, field->name)) {
			/* field maches, report this row position */
			return row[iterator];
		} /* end if */

		/* next position */
		iterator++;
	} /* end while */

	/* no field with that name was found */
	return NULL;
}

/** 
 * @internal Function that creates a connection to the MySQL database
 * configured on the xml node.
 */ 
MYSQL * mod_auth_mysql_get_connection (MyQttdCtx      * ctx,
				       axlNode        * dsn_node, 
				       axlError      ** err)
{
	MYSQL   * conn;
	int       port = 0;
	int       reconnect = 1;

	if (ctx == NULL || dsn_node == NULL) {
		axl_error_report (err, -1, "Received null ctx, auth db node or sql query, failed to run SQL command");
		return NULL;
	} /* end if */

	/* create a mysql connection */
	conn = mysql_init (NULL);

	/* get port */
	if (HAS_ATTR (dsn_node, "port") && strlen (ATTR_VALUE (dsn_node, "port")) > 0) {
		/* get port configured by the user */
		port = atoi (ATTR_VALUE (dsn_node, "dbport"));
	}

	/* create a connection */
	if (mysql_real_connect (conn, 
				/* get host */
				ATTR_VALUE (dsn_node, "dbhost"), 
				/* get user */
				ATTR_VALUE (dsn_node, "dbuser"), 
				/* get password */
				ATTR_VALUE (dsn_node, "dbpassword"), 
				/* get database */
				ATTR_VALUE (dsn_node, "db"),
				/* port */
				port,
				NULL, 0) == NULL) {
		axl_error_report (err, mysql_errno (conn), "Mysql connect error: %s, failed to run SQL command", mysql_error (conn));
		return NULL;
	} /* end if */

	/* flag here to reconnect in case of lost connection */
	mysql_options (conn, MYSQL_OPT_RECONNECT, (const char *) &reconnect);

	return conn;
}

/** 
 * @brief Allows to check if the database connection is working.
 *
 * @param ctx The context where the operation will take place.
 * 
 * @return axl_true in the case it is working, otherwise axl_false is
 * returned.
 */
axl_bool mod_auth_mysql_check_conn (MyQttdCtx * ctx, axlNode * dsn_node)
{
	MYSQL      * conn;
	axlError   * err = NULL;

	/* get configuration node and check everything is working */
	conn = mod_auth_mysql_get_connection (ctx, dsn_node, &err);
	if (conn == NULL) {
		error ("Database connection is failing. Check settings and/or MySQL server status, error was: %s",
		       axl_error_get (err));
		axl_error_free (err);
		return axl_false;
	} /* end if */

	/* release connection */
	mysql_close (conn);
	/* mod_auth_mysql_release_connection (ctx, conn); */
	
	return axl_true;
}

char * __mod_auth_mysql_escape_query (const char * query)
{
	int iterator;
	char * complete_query = NULL;

	/* copy query */
	complete_query = axl_strdup (query);
	if (complete_query == NULL)
		return NULL;

	iterator       = 0;
	while (complete_query[iterator]) {
		if (complete_query[iterator] == '%')
			complete_query[iterator] = '#';
		iterator++;
	} /* end while */

	return complete_query;
}

/** 
 * @brief Allows to run a query when it is a single paramemeter. This
 * function is recommended when providing static strings or already
 * formated queries (to avoid problems with varadic string
 * implementations).
 *
 * @param ctx The context where the query will be executed
 *
 * @param query The query to run.
 *
 * @return A reference to the result or NULL if it fails.
 */
MYSQL_RES *     mod_auth_mysql_run_query_s   (MyQttdCtx * ctx, axlNode * dsn_node, const char  * query)
{
	int         iterator;
	axl_bool    non_query;
	MYSQL     * dbconn;
	char      * local_query;
	MYSQL_RES * result;
	axlError  * err = NULL;

	if (ctx == NULL || query == NULL)
		return NULL;

	/* make a local copy to handle it */
	local_query = axl_strdup (query);
	if (local_query == NULL)
		return NULL;

	/* get if we have a non query request */
	non_query = ! axl_stream_casecmp ("SELECT", local_query, 6);

	/* prepare query (replace # by %) */
	iterator = 0;
	while (local_query && local_query[iterator]) {
		if (local_query[iterator] == '#')
			local_query[iterator] = '%';
		/* next position */
		iterator++;
	} /* end if */

	/* get connection */
	dbconn = mod_auth_mysql_get_connection (ctx, dsn_node, &err);
	if (dbconn == NULL) {
		error ("Failed to acquire connection to run query (%s), error was: %s", local_query, axl_error_get (err));
		axl_error_free (err);

		/* release conn */
		axl_free (local_query);
		return NULL;
	} /* end if */

	/* now run query */
	if (mysql_query (dbconn, local_query)) {
		error ("Failed to run SQL query (%s), error was %u: %s", local_query, mysql_errno (dbconn), mysql_error (dbconn));

		/* release the connection */
		mysql_close (dbconn); 

		/* release conn */
		axl_free (local_query);
		return NULL;
	} /* end if */

	if (non_query) {
		/* release the connection */
		mysql_close (dbconn); 

		/* release conn */
		axl_free (local_query);

		/* report ok */
		return INT_TO_PTR (axl_true);
	} /* end if */

	/* return result */
	result = mysql_store_result (dbconn);
	
	/* release the connection */
	mysql_close (dbconn); 

	/* release conn */
	axl_free (local_query);

	return result;
}

/** 
 * @brief Allows to run a query and report the value found at the
 * first row on the provided column as a integer.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param col_name The column name where to get the result.
 *
 * @param query The query string to run.
 *
 * @param ... Additional arguments to the query string.
 *
 * @return -1 in case of errors or the value reported at the first
 * row of the query result at the column requested. Note that if the
 * result is defined but it has NULL or empty string, 0 is returned.
 */
long             __mod_auth_mysql_run_query_as_long (MyQttdCtx  * ctx,
						     axlNode    * dsn_node,
						     const char * query, 
						     ...)
{
	MYSQL_RES * result;
	char      * complete_query;
	va_list     args;
	MYSQL_ROW   row;
	long        int_result;

	/* open std args */
	va_start (args, query);

	/* create complete query */
	complete_query = axl_stream_strdup_printfv (query, args);
	if (complete_query == NULL) {
		complete_query = __mod_auth_mysql_escape_query (query);
		error ("Query failed, axl_stream_strdup_printfv reported error for query: %s\n", complete_query);
		error ("Returning -1 at __mod_auth_mysql_run_query_as_long\n");
		axl_free (complete_query);
		return -1;
	} /* end if */

	/* close std args */
	va_end (args);

	/* clear query */
	axl_stream_trim (complete_query);

	/* run query */
	result = mod_auth_mysql_run_query_s (ctx, dsn_node, complete_query);
	if (result == NULL) {
		/* get complete query */
		axl_free (complete_query);
		complete_query = __mod_auth_mysql_escape_query (query);
		error ("Query failed: %s\n", complete_query);
		axl_free (complete_query);
		return -1; 
	}
	axl_free (complete_query);

	/* get first row */
	row = mysql_fetch_row (result);
	if (row == NULL || row[0] == NULL || strlen (row[0]) == 0)
		return 0;

	/* get result */
	int_result = strtol (row[0], NULL, 10);

	/* release the result */
	if (PTR_TO_INT (result) != axl_true)
		mysql_free_result (result);

	return int_result;
}


void __mod_auth_mysql_run_query_for_test (MyQttdCtx * ctx, const char * query)
{
	/* get dsn database configuration node */
	MYSQL_RES * res;
	axlNode   * dsn_node;

	/* get dns node */
	dsn_node = axl_doc_get (modconfig, "/mod-auth-mysql/dsn");

	/* call to run and free result */
	res = mod_auth_mysql_run_query_s (ctx, dsn_node, query);
	if (PTR_TO_INT (res) == 1)
		return;

	mysql_free_result (res);

	return;
}

/** 
 * @brief Allows to run the provided query reporting the result.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param query The query to be run.
 *
 * @param ... Additional parameter for the query to be created.
 *
 * @return A reference to MYSQL_RES or NULL if it fails. The function
 * returns axl_true in the case of a NON query (UPDATE, DELETE, INSERT
 * ...) and there is no error.
 */
MYSQL_RES * mod_auth_mysql_run_query (MyQttdCtx * ctx, axlNode * dsn_node, const char * query, ...)
{
	MYSQL_RES * result;
	char      * complete_query;
	va_list     args;
	
	/* check context or query */
	if (ctx == NULL || query == NULL)
		return NULL;

	/* open std args */
	va_start (args, query);

	/* create complete query */
	complete_query = axl_stream_strdup_printfv (query, args);

	/* close std args */
	va_end (args);

	if (complete_query == NULL)  {
		/* escape query to be printed */
		complete_query = __mod_auth_mysql_escape_query (query);
			error ("Query failed to be created at mod_auth_mysql_run_query (axl_stream_strdup_printfv failed): %s\n", complete_query);
		axl_free (complete_query);
		return NULL;
	}

	/* clear query */
	axl_stream_trim (complete_query);

	if (HAS_ATTR_VALUE (dsn_node, "debug", "yes")) {
		msg ("DEBUG: Running query: %s", complete_query);
	} /* end if */

	/* report result */
	result = mod_auth_mysql_run_query_s (ctx, dsn_node, complete_query);

	axl_free (complete_query);
	return result;
}

/** 
 * @brief Allows to check if the table has the provided attribute
 *
 * @param ctx The context where the operation will take place.
 *
 * @param table_name The database table that will be checked.
 */
axl_bool mod_auth_mysql_attr_exists (MyQttdCtx * ctx, axlNode * dsn_node,
				     const char * table_name, const char * attr_name)
{
	MYSQL_RES * result;

	/* check attribute name */
	if (axl_cmp (attr_name, "usage")) {
		error ("Used unallowed attribute name %s", attr_name);
		return axl_false;
	} /* end if */

	if (! mod_auth_mysql_check_conn (ctx, dsn_node)) {
		error ("Unable to check if table exists, database connection is not working");
		return axl_false;
	}

	/* run query */
	result = mod_auth_mysql_run_query (ctx, dsn_node, "SELECT %s FROM %s LIMIT 1", attr_name, table_name);
	if (result == NULL) {
		/* don't report error, table doesn't exists */
		return axl_false;
	} /* end if */

	/* release the result */
	mysql_free_result (result);
	return axl_true;
}


/** 
 * @brief Allows to check if the table exists using context configuration.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param table_name The database table that will be checked.
 */
axl_bool mod_auth_mysql_table_exists (MyQttdCtx * ctx, axlNode * dsn_node, const char * table_name)
{
	MYSQL_RES * result;

	/* check attribute name */
	if (axl_cmp (table_name, "usage")) {
		error ("Used unallowed table name %s", table_name);
		return axl_false;
	} /* end if */

	if (! mod_auth_mysql_check_conn (ctx, dsn_node)) {
		error ("Unable to check if table exists, database connection is not working");
		return axl_false;
	}

	/* run query */
	result = mod_auth_mysql_run_query (ctx, dsn_node, "SELECT * FROM %s LIMIT 1", table_name);
	if (result == NULL) {
		/* don't report error, table doesn't exists */
		return axl_false;
	} /* end if */

	/* release the result */
	mysql_free_result (result);
	return axl_true;
}


/** 
 * @brief Allows to checks and creates the table provided in the case
 * it doesn't exists. 
 *
 * @param ctx The context where the operation will take place.
 *
 * @param table_name The table name to be checked to exist
 * 
 * @param  attr_name The attribute name to be created.
 *
 * @param attr_type The attribute type to associate. If you want to
 * support typical autoincrement index. use as type: "autoincrement int".
 *
 * The function allows to check and create an SQL table with the name
 * provided and the set of attributes provided. The list must be ended
 * by NULL.
 *
 * @return The function returns axl_true in the case everything
 * finished without any issue otherwise axl_false is returned.
 *
 * 
 */ 
axl_bool        mod_auth_mysql_ensure_table (MyQttdCtx  * ctx,
					     axlNode    * dsn_node,
					     const char * table_name,
					     const char * attr_name, const char * attr_type, 
					     ...)
{
	va_list   args;

	/* check if the mysql table exists */
	if (! mod_auth_mysql_table_exists (ctx, dsn_node, table_name)) {
		/* support for auto increments */
		if (axl_cmp (attr_type, "autoincrement int"))
			attr_type = "INT AUTO_INCREMENT PRIMARY KEY";

		/* create the table with the first column */
		if (! mod_auth_mysql_run_query (ctx, dsn_node, "CREATE TABLE %s (%s %s)", table_name, attr_name, attr_type)) {
			error ("Unable to create table %s, failed to ensure table exists", table_name);
			return axl_false;
		} /* end if */
	} /* end if */

	if (! mod_auth_mysql_attr_exists (ctx, dsn_node, table_name, attr_name)) {
		/* support for auto increments */
		
		if (axl_cmp (attr_type, "autoincrement int"))
			attr_type = "INT AUTO_INCREMENT PRIMARY KEY";

		/* create the table with the first column */
		if (! mod_auth_mysql_run_query (ctx, dsn_node, "ALTER TABLE %s ADD COLUMN %s %s", table_name, attr_name, attr_type)) {
			error ("Unable to update table %s to add attribute %s : %s, failed to ensure table exists",
			       table_name, attr_name, attr_type);
			return axl_false;
		} /* end if */
	} /* end if */

	va_start (args, attr_type);

	while (axl_true) {
		/* get attr name to create and stop if NULL is defined */
		attr_name  = va_arg (args, const char *);
		if (attr_name == NULL)
			break;
		/* get attr type */
		attr_type  = va_arg (args, const char *);

		/* though not supported, check for NULL values here */
		if (attr_type == NULL)
			break;

		/* support for auto increments */
		if (axl_cmp (attr_type, "autoincrement int"))
			attr_type = "INT AUTO_INCREMENT PRIMARY KEY";

		if (! mod_auth_mysql_attr_exists (ctx, dsn_node, table_name, attr_name)) {
			/* create the table with the first column */
			if (! mod_auth_mysql_run_query (ctx, dsn_node, "ALTER TABLE %s ADD COLUMN %s %s", table_name, attr_name, attr_type)) {
				error ("Unable to update table %s to add attribute %s : %s, failed to ensure table exists",
				       table_name, attr_name, attr_type);
				return axl_false;
			} /* end if */
		} /* end if */
	} /* end while */

	return axl_true;
}


/** Implementation for MyQttdUsersLoadDb **/
axlPointer __mod_auth_mysql_load (MyQttdCtx    * ctx,
				  MyQttdDomain * domain,
				  MyQttConn    * conn, 
				  const char   * path)
{
	/* for now, we don't load anything special here because init
	 * operation has done everything we needed */
	
	return INT_TO_PTR (1); /* report something so the caller do not detect an error here */
}

/** Implemenation for MyQttdUsersExists **/
axl_bool     __mod_auth_mysql_user_exists (MyQttdCtx    * ctx,
					   MyQttdDomain * domain,
					   axl_bool       domain_selected,
					   MyQttdUsers  * users,
					   MyQttConn    * conn, 
					   axlPointer     _backend,
					   const char   * client_id, 
					   const char   * user_name)
{
	/* for now, we are going to report that this user does not
	 * exists. This function is still not used */
	return axl_false;
}


const char * __mod_auth_mysql_get_protocol (MyQttConn * conn)
{
	noPollConn * wconn;

	wconn = myqtt_web_socket_get_conn (conn);
	if (wconn) {
		if (nopoll_conn_is_tls_on (wconn))
			return "mqtt-wss";
		return "mqtt-ws";
	} /* end if */

	if (myqtt_tls_is_on (conn)) 
		return "mqtt-tls";
	return "mqtt";
}


axl_bool __mod_auth_mysql_accept_user (MyQttdCtx * ctx, MyQttdDomain * domain, MyQttdUsers * users, MyQttConn * conn, int user_id, axlNode * dsn_node)
{
	char * query;
	
	
	if (user_id > 0) {
		/* save user_id into connection for later use */
		myqtt_conn_set_data (conn, "mod:mysql:auth:user_id", INT_TO_PTR (user_id));

		/* record here accepted connection from a given IP, user, etc */
		query = axl_strdup_printf ("INSERT INTO user_log (user_id, timestamp, action, ip, protocol, description) VALUES ('%d', '%d', 'login-ok', '%s', '%s', 'Login ok received')",
					   user_id, time (NULL), myqtt_conn_get_host (conn), __mod_auth_mysql_get_protocol (conn));
		mod_auth_mysql_run_query_s (ctx, dsn_node, query);
		axl_free (query);
		
	} /* end if */
	
	return axl_true; /* connection accepted */
}

axl_bool __mod_auth_mysql_reject_user (MyQttdCtx * ctx, MyQttdDomain * domain, axlNode * dsn_node, axl_bool domain_selected,
				       MyQttdUsers * users, MyQttConn * conn, int user_id, const char * user_name, const char * clientid, const char * reason)
{
	char * query;
	
	/* do not record anything here for now. When a login failure
	 * is detected, it is not clear to who user we have to attach
	 * it. The same happens with the domain. At the same time
	 * myqttd already records a login failure indication */
	if (domain_selected) {
		if (user_id > 0) {
			/* record here accepted connection from a given IP, user, etc */
			query = axl_strdup_printf ("INSERT INTO user_log (user_id, timestamp, action, ip, protocol, description) VALUES ('%d', '%d', 'login-failure', '%s', '%s', '%s')",
						   user_id, time (NULL), myqtt_conn_get_host (conn), __mod_auth_mysql_get_protocol (conn), reason);
			mod_auth_mysql_run_query_s (ctx, dsn_node, query);
			axl_free (query);

			/* report to the log */
			error ("Login failure for user-id=%d, clientid=%s, username=%s, ip=%s, protocol=%s : %s",
			       user_id,
			       clientid  ? clientid : "",
			       user_name ? user_name : "", myqtt_conn_get_host (conn), __mod_auth_mysql_get_protocol (conn), reason);
		} else {
			/* report to the log */
			error ("Login failure for clientid=%s, username=%s, ip=%s, protocol=%s : %s",
			       user_name ? user_name : "",
			       clientid ? clientid : "", myqtt_conn_get_host (conn), __mod_auth_mysql_get_protocol (conn), reason);
		} /* end if */
	}
	
	return axl_false; /* connection rejected */
}

axl_bool __mod_auth_mysql_protocol_allowed (MyQttdCtx * ctx, MYSQL_RES * res, MYSQL_ROW row, MyQttConn * conn)
{
	const char * protocol = __mod_auth_mysql_get_protocol (conn);

	if (axl_cmp (protocol, "mqtt"))
		return myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, res, row, "allow_mqtt"), NULL) > 0;
	if (axl_cmp (protocol, "mqtt-ws"))
		return myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, res, row, "allow_mqtt_ws"), NULL) > 0;
	if (axl_cmp (protocol, "mqtt-wss"))
		return myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, res, row, "allow_mqtt_wss"), NULL) > 0;
	if (axl_cmp (protocol, "mqtt-tls"))
		return myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, res, row, "allow_mqtt_tls"), NULL) > 0;

	/* never reached, unknown protocol */
	error ("Unknown protocol received: %s, failed to handle it", protocol);
	return axl_false; /* unknown protocol */
}


/** Implementation for MyQttdUsersAuthUser **/
axl_bool     __mod_auth_mysql_auth_user (MyQttdCtx    * ctx,
					 MyQttdDomain * domain,
					 axl_bool       domain_selected,
					 MyQttdUsers  * users,					 
					 MyQttConn    * conn,
					 axlPointer     _backend,
					 const char   * clientid, 
					 const char   * user_name,
					 const char   * password)
{
	int         check_mode = 0;
	MYSQL_RES * res;
	MYSQL_ROW   row;
	axlNode   * dsn_node;
	axl_bool    anonymous;
	int         default_acl;
	int         apply_message_quota;
	char      * digest_password;
	axl_bool    result;
	int         user_id;
	axl_bool    check_protocol_allowed;
	
	/* we ignore _backend because everything it is loaded through
	 * the dsn node */

	/* check type of checking we have to implement */
	check_mode = myqttd_support_check_mode (user_name, clientid);
	if (check_mode == 0) {
		/* report failure because username and client id is not defined */
		return axl_false;
	} /* end if */

	/* get dsn database configuration node */
	dsn_node = axl_doc_get (modconfig, "/mod-auth-mysql/dsn");

	/* check params */
	if (! __mod_auth_mysql_check_input (ctx, user_name)) 
		return axl_false;
	if (! __mod_auth_mysql_check_input (ctx, clientid)) 
		return axl_false;
	/* now need to check password input because it is md5d before checking */
	/* if (! __mod_auth_mysql_check_input (ctx, password)) 
	   return axl_false; */

	/* check if the domain exists in the database */
	res      = mod_auth_mysql_run_query (ctx, dsn_node, "SELECT * FROM domain WHERE is_active = '1' AND  name = '%s'", myqttd_domain_get_name (domain));
	if (res == NULL)
		return axl_false; /* failed to run query */
	
	row      = mysql_fetch_row (res);
	if (row == NULL || row[0] == NULL) {
		/* release result */
		mysql_free_result (res);
		return axl_false; /* empty query received */
	}

	/* get default domain acl */
	default_acl = myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, res, row, "default_acl"), NULL);
	myqtt_conn_set_data (conn, "mod:mysql:auth:default_acl", INT_TO_PTR (default_acl));

	/* get apply_message_quota */
	apply_message_quota = myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, res, row, "apply_message_quota"), NULL);
	myqtt_conn_set_data (conn, "mod:mysql:auth:message_quota", INT_TO_PTR (apply_message_quota));

	/* domain is defined in tables, let's continue, but first,
	 * check for anonymous connections accepted for this domain */
	anonymous = myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, res, row, "anonymous"), NULL);
	if (anonymous && domain_selected) {
		/* accept connection since anonymous is enabled */
		return __mod_auth_mysql_accept_user (ctx, domain, users, conn, -1, dsn_node); 
	} /* end if */
	mysql_free_result (res);

	switch (check_mode) {
	case 3:
		/* query database */
		res      = mod_auth_mysql_run_query (ctx, dsn_node,
						     "SELECT id, password, allow_mqtt, allow_mqtt_tls, allow_mqtt_ws, allow_mqtt_wss FROM user WHERE is_active = '1' AND require_auth = 1 AND domain_id = (SELECT id FROM domain WHERE is_active = '1' AND name = '%s') AND BINARY username = '%s' AND BINARY clientid = '%s'",
						     myqttd_domain_get_name (domain), user_name, clientid);
		if (res == NULL) {
			return __mod_auth_mysql_reject_user (ctx, domain, dsn_node, domain_selected, users, conn, -1, user_name, clientid, "Username/Clientid/Password combination unknown for this domain, SQL failure");
		} /* end if */
		
		/* get first row from result */
		row      = mysql_fetch_row (res);
		if (row == NULL || row[1] == NULL) {
			mysql_free_result (res);
			return __mod_auth_mysql_reject_user (ctx, domain, dsn_node, domain_selected, users, conn, -1, user_name, clientid, "Username/clientid combination unknown for this domain");
		} /* end if */

		/* user and client id (and password) */
		digest_password = myqtt_tls_get_digest (MYQTT_MD5, password);
		if (digest_password == NULL) 
			return __mod_auth_mysql_reject_user (ctx, domain, dsn_node, domain_selected, users, conn, -1, user_name, clientid, "Unable to encode password into MD5 for query");

		/* get result */
		result  = axl_cmp (digest_password, row[1]);

		if (HAS_ATTR_VALUE (dsn_node, "debug", "yes")) {
			/* report database password found and what was provided by the calling user */
			msg ("Checking password [%s] == [%s], result %d", digest_password, row[1], result);
		} /* end if */
		
		user_id = myqtt_support_strtod (row[0], NULL);
		axl_free (digest_password);
		if (result)
			check_protocol_allowed = __mod_auth_mysql_protocol_allowed (ctx, res, row, conn);
		mysql_free_result (res);
		
		if (result) {
			/* check protocol allowed */
			if (! check_protocol_allowed)
				return __mod_auth_mysql_reject_user (ctx, domain, dsn_node, axl_true /* domain_selected */, users, conn, user_id, user_name, clientid, "Login failure because protocol combination was not allowed");
			/* report accept user */
			return __mod_auth_mysql_accept_user (ctx, domain, users, conn, user_id, dsn_node);
		} /* end if */
		
		return __mod_auth_mysql_reject_user (ctx, domain, dsn_node, domain_selected, users, conn, user_id, user_name, clientid, "Password failure for client-id/username");
	case 2:
		/* query database */
		res      = mod_auth_mysql_run_query (ctx, dsn_node,
						     "SELECT clientid, id, allow_mqtt, allow_mqtt_tls, allow_mqtt_ws, allow_mqtt_wss FROM user WHERE is_active = '1' AND  require_auth = 0 AND domain_id = (SELECT id FROM domain WHERE is_active = '1' AND  name = '%s') AND BINARY clientid = '%s'",
						     myqttd_domain_get_name (domain), clientid);
		if (res == NULL) {
			return __mod_auth_mysql_reject_user (ctx, domain, dsn_node, domain_selected, users, conn, -1, NULL, clientid, "Clientid combination unknown for this domain, SQL failure");
		} /* end if */
			
		/* get first row from result */
		row  = mysql_fetch_row (res);
		if (row == NULL || row[0] == NULL) {
			mysql_free_result (res);
			return __mod_auth_mysql_reject_user (ctx, domain, dsn_node, domain_selected, users, conn, -1, NULL, clientid, "Username/clientid combination unknown for this domain");
		} /* end if */

		/* get result */
		result  = axl_cmp (clientid, row[0]);
		user_id = myqtt_support_strtod (row[1], NULL);
		if (result)
			check_protocol_allowed = __mod_auth_mysql_protocol_allowed (ctx, res, row, conn);
		mysql_free_result (res);
		
		if (result) {
			/* check protocol allowed */
			if (! check_protocol_allowed)
				return __mod_auth_mysql_reject_user (ctx, domain, dsn_node, axl_true /* domain_selected */, users, conn, user_id, NULL, clientid, "Login failure because protocol combination was not allowed");
			/* report accept user */
			return __mod_auth_mysql_accept_user (ctx, domain, users, conn, user_id, dsn_node);
		} /* end if */
		return __mod_auth_mysql_reject_user (ctx, domain, dsn_node, domain_selected, users, conn, -1, NULL, clientid, "Client-id unknown");
	case 1:
		/* case not supported */
		return __mod_auth_mysql_reject_user (ctx, domain, dsn_node, domain_selected, users, conn, -1, NULL, NULL, "Remove device only provided username");
		
	} /* end switch */
		
	/* user does not exists for this backend */
	return __mod_auth_mysql_reject_user (ctx, domain, dsn_node, domain_selected, users, conn, -1, NULL, NULL, "Remove device did not provided any valid credential");
}

/** MyQttdUsersUnloadDb **/
void __mod_auth_mysql_unload (MyQttdCtx * ctx, 
			    axlPointer  _backend)
{
	/* we do anything here because __mod_auth_mysql_load did
	 * anything special */
	return;
}

MyQttPublishCodes __mod_auth_mysql_on_publish_report (MyQttdCtx * ctx,       MyQttdDomain * domain, 
						      MyQttCtx  * myqtt_ctx, MyQttConn    * conn, 
						      MyQttMsg  * msg,       MyQttPublishCodes result) {

	/* report here when something was denied */
	return result;
}

MyQttPublishCodes __mod_auth_mysql_get_default_acl (MyQttdCtx * ctx, MyQttConn * conn)
{
	/* report value configured on the provided connection */
	int default_acl = PTR_TO_INT (myqtt_conn_get_data (conn, "mod:mysql:auth:default_acl"));
	if (default_acl > 0)
		return default_acl;
	
	return MYQTT_PUBLISH_OK; /* if nothing is configured, default allow */
}

/** 
 * @internal Do not use this function. It's just for debugging purposes 
 */
void __mod_auth_mysql_show_mysql_row (MYSQL_RES * acls, MYSQL_ROW row)
{
	MYSQL_FIELD  * field;
	int            iterator   = 0;
	int            num_fields;

	mysql_field_seek (acls, 0);

	num_fields = mysql_num_fields (acls);
	while (iterator < num_fields) {
		field = mysql_fetch_field (acls);
		printf (" row[%d] = %s (%s)\n", iterator, row[iterator], field ? field->name : "unknown");
		iterator++;
	}
	return;
}


MyQttPublishCodes __mod_auth_mysql_apply_acl (MyQttdCtx * ctx, MyQttdDomain * domain, MyQttCtx * myqtt_ctx,
					      MyQttConn * conn, MyQttMsg * msg, axl_bool apply_after, axl_bool is_domain_acl, MYSQL_RES * acls)
{
	MYSQL_ROW            row;
	axl_bool             local_apply_after;
	const char         * topic_filter;
	MyQttPublishCodes    action_if_matches;
	/* get default acl action to report it in case action_if_matches does not define any valid code: namely 0 */
	MyQttPublishCodes    default_acl_action  = __mod_auth_mysql_get_default_acl (ctx, conn);

	/* check acl received */
	if (acls == NULL)
		return MYQTT_PUBLISH_DUNNO;

	/* reset to the first position */
	mysql_data_seek (acls, 0);

	/* get the first row */
	row = mysql_fetch_row (acls);
	while (row) {

		if (is_domain_acl) {

			/* domain acl, get apply_after status */
			local_apply_after = myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, acls, row, "apply_after"), NULL);
			if (local_apply_after != apply_after) {
				/* do not apply acl here, go to the next */
				row = mysql_fetch_row (acls);
				continue;
			} /* end if */
		} /* end if */

		/* check topic to see if it matches with message topic */
		topic_filter = __mod_auth_mysql_get_by_name (ctx, acls, row, "topic_filter");
		if (myqtt_reader_topic_filter_match (myqtt_msg_get_topic (msg), topic_filter)) {

			/* get action if matches */
			action_if_matches = myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, acls, row, "action_if_matches"), NULL);
			if (action_if_matches == 0) {
				/* let domain default configuration decide */
				action_if_matches = default_acl_action;
			} /* end if */
			
			/* filter matches, apply option indicated */
			if (myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, acls, row, "w"), NULL))
				return action_if_matches;
			
			if (myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, acls, row, "publish"), NULL))
				return action_if_matches;
			
			if (myqtt_msg_get_qos (msg) == MYQTT_QOS_0 && myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, acls, row, "publish0"), NULL))
				return action_if_matches;
			
			if (myqtt_msg_get_qos (msg) == MYQTT_QOS_1 && myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, acls, row, "publish1"), NULL))
				return action_if_matches;
			
			if (myqtt_msg_get_qos (msg) == MYQTT_QOS_2 && myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, acls, row, "publish2"), NULL))
				return action_if_matches;
		} /* end if */
		

		/* get next row */
		row = mysql_fetch_row (acls);
	} /* end if */

	return MYQTT_PUBLISH_DUNNO; /* no acl reported nothing especial, so let's continue */
}

MyQttPublishCodes __mod_auth_mysql_on_publish_aux (MyQttdCtx * ctx,        MyQttdDomain * domain, 
						   MyQttCtx  * myqtt_ctx,  MyQttConn    * conn, 
						   MyQttMsg  * msg,        axlNode      * dsn_node,
						   MYSQL_RES * users_acls, MYSQL_RES    * domain_acls,
						   int         user_id)
{
	MyQttPublishCodes     result;

	/** GLOBAL: BEFORE: apply global acls before users' acls */
	result = __mod_auth_mysql_apply_acl (ctx, domain, myqtt_ctx, conn, msg,
					     axl_false /* apply after */, axl_true /* is_domain_acl */, domain_acls);
	if (result != MYQTT_PUBLISH_DUNNO)
		return __mod_auth_mysql_on_publish_report (ctx, domain, myqtt_ctx, conn, msg, result);

	/* USER: apply acls if defined for the provided user node */
	result = __mod_auth_mysql_apply_acl (ctx, domain, myqtt_ctx, conn, msg,
					     axl_false /* apply after */, axl_false /* is_domain_acl */, users_acls);
	if (result != MYQTT_PUBLISH_DUNNO)
		return __mod_auth_mysql_on_publish_report (ctx, domain, myqtt_ctx, conn, msg, result);

	/** GLOBAL: AFTER: apply global acls before users' acls */
	result = __mod_auth_mysql_apply_acl (ctx, domain, myqtt_ctx, conn, msg,
					     axl_true /* apply after */, axl_true /* is_domain_acl */, domain_acls);
	if (result != MYQTT_PUBLISH_DUNNO)
		return __mod_auth_mysql_on_publish_report (ctx, domain, myqtt_ctx, conn, msg, result);

	/* report publish is ok so return default deny policy */
	result = __mod_auth_mysql_get_default_acl (ctx, conn);
	return __mod_auth_mysql_on_publish_report (ctx, domain, myqtt_ctx, conn, msg, result);
}
	

MyQttPublishCodes __mod_auth_mysql_on_publish (MyQttdCtx * ctx,       MyQttdDomain * domain, 
					       MyQttCtx  * myqtt_ctx, MyQttConn    * conn, 
					       MyQttMsg  * msg,       axlPointer     user_data)
{
	int                   user_id     = __mod_auth_mysql_get_user_id (conn);
	axlNode             * dsn_node    = user_data;
	MYSQL_RES           * users_acls  = NULL;
	MYSQL_RES           * domain_acls = NULL;
	MyQttPublishCodes     result;
	int                   apply_message_quota;
	long                  current_day_usage;
	long                  current_month_usage;

	/* get acls for domain requested */
	domain_acls = mod_auth_mysql_run_query (ctx, dsn_node, "SELECT * FROM domain_acl WHERE is_active = '1' AND  domain_id = (SELECT id FROM domain WHERE is_active = '1' AND name = '%s')",
						myqttd_domain_get_name (domain));
	if (user_id > 0)  {
		/* get user acls if user is defined */
		users_acls  = mod_auth_mysql_run_query (ctx, dsn_node, "SELECT * FROM user_acl WHERE is_active = '1' AND user_id = '%d'", user_id);
	} /* end if */

	/** call aux function, get result and release resources */
	result = __mod_auth_mysql_on_publish_aux (ctx, domain, myqtt_ctx, conn, msg, dsn_node, users_acls, domain_acls, user_id);

	/* release memory */
	mysql_free_result (users_acls);
	mysql_free_result (domain_acls);

	if (result == MYQTT_PUBLISH_OK && user_id > 0) {
		/* check if record exists or not */
		if (! __mod_auth_mysql_run_query_as_long (ctx, dsn_node, "SELECT count(*) FROM user_msg_tracking WHERE user_id = '%d'", user_id)) {
			/* record does not exists, insert an empty one for this user */
			mod_auth_mysql_run_query (ctx, dsn_node, "INSERT INTO user_msg_tracking (current_day_usage, current_month_usage, user_id) VALUES (0, 0, '%d')", user_id);
		} /* end if */
		
		/* get current messages */
		current_day_usage   = __mod_auth_mysql_run_query_as_long (ctx, dsn_node, "SELECT current_day_usage FROM user_msg_tracking WHERE user_id = '%d'", user_id);
		if (current_day_usage >= 0)
			current_day_usage++;
		
		current_month_usage = __mod_auth_mysql_run_query_as_long (ctx, dsn_node, "SELECT current_month_usage FROM user_msg_tracking WHERE user_id = '%d'", user_id);
		if (current_month_usage >= 0)
			current_month_usage++;

		/* ok, operation ok, now track publish limits, it any */
		apply_message_quota = PTR_TO_INT (myqtt_conn_get_data (conn, "mod:mysql:auth:message_quota"));
		if (apply_message_quota) {
			
			/* limit here if quota has been reached */
			if (current_day_usage > myqttd_domain_get_day_message_quota (domain)) {
				/* report to the log */
				error ("Publish rejected for user-id=%d, clientid=%s, username=%s, ip=%s, protocol=%s : daily quota has been reached (%d messages)",
				       user_id,
				       myqtt_conn_get_client_id (conn)  ? myqtt_conn_get_client_id (conn) : "",
				       myqtt_conn_get_username (conn) ? myqtt_conn_get_username (conn) : "", myqtt_conn_get_host (conn), __mod_auth_mysql_get_protocol (conn), current_day_usage);
				return MYQTT_PUBLISH_DISCARD;
			} /* end if */

			/* month check*/
			if (current_month_usage > myqttd_domain_get_month_message_quota (domain)) {
				/* report to the log */
				error ("Publish rejected for user-id=%d, myqtt_conn_get_client_id (conn)=%s, username=%s, ip=%s, protocol=%s : monthly quota has been reached (%d messages)",
				       user_id,
				       myqtt_conn_get_client_id (conn)  ? myqtt_conn_get_client_id (conn) : "",
				       myqtt_conn_get_username (conn) ? myqtt_conn_get_username (conn) : "", myqtt_conn_get_host (conn), __mod_auth_mysql_get_protocol (conn), current_day_usage);
				return MYQTT_PUBLISH_DISCARD;
			} /* end if */
			
		} /* end if */

		/* reached this point, publish has not been limited by quota, update database database */
		mod_auth_mysql_run_query (ctx, dsn_node, "UPDATE user_msg_tracking SET current_day_usage = current_day_usage + 1, current_month_usage = current_month_usage + 1 WHERE user_id = '%d'", user_id);
	} /* end if */

	/* call to report data...nice and clean code! how beatiful! */
	return result;
}


/** 
 * @brief Init function, perform all the necessary code to register
 * handlers, configure MyQtt, and any other init task. The function
 * must return true to signal that the module was properly initialized
 * Otherwise, false must be returned.
 */
static int  mod_auth_mysql_init (MyQttdCtx * _ctx)
{
	char              * config;
	axlError          * err = NULL;
	axlNode           * dsn_node;
	MYSQL             * conn;
	int                 conns;
	
	/* configure the module */
	MYQTTD_MOD_PREPARE (_ctx);

	/* check if we have databases configured */
	config = myqtt_support_build_filename (myqttd_sysconfdir (ctx), "myqtt", "mysql", "mysql.xml", NULL);
	if (config == NULL) {
		error ("Failed to init mod-auth-mysql, unable to build configuration file path (allocation memory problem)\n");
		return axl_false; /* allocation failure */
	} /* end if */
	
	if (! myqtt_support_file_test (config, FILE_EXISTS)) {
		
		/* release */
		axl_free (config);
		
		/* file does not exists, try to find it through environment */
		config = myqtt_support_domain_find_data_file (MYQTTD_MYQTT_CTX (ctx), "myqtt-conf", "mysql.xml");
		if (! myqtt_support_file_test (config, FILE_EXISTS)) {
			error ("Unable to initialize mod-auth-mysql module because %s/myqtt/mysql.xml file was not found nor mysql through myqtt-conf declarations", myqttd_sysconfdir (ctx));
			return axl_false; 
		} /* end if */
	} /* end if */

	/* reached this point because found a file, try to load it */
	msg ("Found MySQL main configuration file at %s", config);
	
	/* load document */
	modconfig = axl_doc_parse_from_file (config, &err);
	if (modconfig == NULL) {
		error ("Failed to load document %s, error was: %s", config, axl_error_get (err));
		axl_free (config);
		axl_error_free (err);
		return axl_false; /* unable to init document, report
				   * failed to init module */
	} /* end if */
	axl_free (config);

	/* now, try to check connections  */
	dsn_node = axl_doc_get (modconfig, "/mod-auth-mysql/dsn");
	conns    = 0;
	while (dsn_node) {
		/* get connection from definition */
		conn     = mod_auth_mysql_get_connection (ctx, dsn_node, &err);
		if (conn == NULL) {
			error ("Failed to open mysql connection, error was: %s", axl_error_get (err));
			axl_error_free (err);
			return axl_false; /* unable to init mysql module */
		} /* end if */

		/* close connection */
		mysql_close (conn);

		/* call to ensure databases */

		/* domain table */
		mod_auth_mysql_ensure_table (ctx, dsn_node,
					     "domain",
					     "id", "autoincrement int",
					     "is_active", "int",
					     "default_acl", "int",
					     "name", "varchar(512)",
					     "anonymous", "int",
					     "apply_message_quota", "int",
					     "description", "text",
					     NULL);

		/* user table */
		mod_auth_mysql_ensure_table (ctx, dsn_node,
					     "user",
					     "id", "autoincrement int",
					     "is_active", "int",
					     "domain_id", "int",
					     "require_auth", "int",
					     "clientid", "varchar(512)",
					     "username", "varchar(512)",
					     "password", "varchar(512)",
					     /* protocol enforcement */
					     "allow_mqtt", "int",
					     "allow_mqtt_tls", "int",
					     "allow_mqtt_ws", "int",
					     "allow_mqtt_wss", "int",
					     /* description */
					     "description", "text",
					     NULL);

		/* user table */
		mod_auth_mysql_ensure_table (ctx, dsn_node,
					     "user_msg_tracking",
					     "id", "autoincrement int",
					     "user_id", "int",
					     /* current day usage */
					     "current_day_usage", "int", 
					     /* current month usage */
					     "current_month_usage", "int",
					     NULL);

		/* user log */
		mod_auth_mysql_ensure_table (ctx, dsn_node,
					     "user_log",
					     "id", "autoincrement int",
					     "user_id", "int",
					     "timestamp", "int",
					     "action", "varchar(128)",
					     "ip", "varchar(128)",
					     "protocol", "varchar(128)", 
					     "description", "text",
					     NULL);
		
		/* domain_acl table */
		mod_auth_mysql_ensure_table (ctx, dsn_node,
					     "domain_acl",
					     "id", "autoincrement int",
					     "is_active", "int",
					     "domain_id", "int",
					     "topic_filter", "text",
					     "apply_after", "int",
					     "r", "int",
					     "w", "int",
					     "publish", "int",
					     "subscribe", "int",
					     "publish0", "int",
					     "publish1", "int",
					     "publish2", "int",
					     "ping", "int",
					     "description", "text",
					     "action_if_matches", "int",
					     NULL);

		/* user_acl table */
		mod_auth_mysql_ensure_table (ctx, dsn_node,
					     "user_acl",
					     "id", "autoincrement int",
					     "is_active", "int",
					     "user_id", "int",
					     "topic_filter", "text",
					     "r", "int",
					     "w", "int",
					     "publish", "int",
					     "subscribe", "int",
					     "publish0", "int",
					     "publish1", "int",
					     "publish2", "int",
					     "ping", "int",
					     "description", "text",
					     "action_if_matches", "int",
					     NULL);
					     
		
		/* get next node */
		dsn_node = axl_node_get_next_called (dsn_node, "dsn");
		conns++;
	} /* end if */

	if (conns == 0) {
		error ("Unable to initialize MySQL mod-auth-mysql, no connections defined was found");
		return axl_false;
	} /* end if */

	/* reached this point, all connections are working */
	msg ("MySQL initial settings ok, registering backend");

	/* install handlers to implement auth based on a simple xml
	   backend */
	if (! myqttd_users_register_backend (ctx, 
					     "mod-auth-myqtt",
					     __mod_auth_mysql_load,
					     __mod_auth_mysql_user_exists,
					     __mod_auth_mysql_auth_user,
					     __mod_auth_mysql_unload,
					     /* for future usage */
					     NULL, NULL, NULL, NULL)) {
		error ("Failed to install mod-auth-mysql authentication handlers..");
		return axl_false;
	} /* end if */

	/* register an on publish (write, publish, publish0, publish1, publish2) */
	dsn_node = axl_doc_get (modconfig, "/mod-auth-mysql/dsn");
	myqttd_ctx_add_on_publish (ctx, __mod_auth_mysql_on_publish, dsn_node);

	/* register on subscribe */
	/* myqttd_ctx_add_on_subscribe (ctx, __mod_auth_mysql_on_subscribe, NULL);  */
	
	msg ("mod-auth-mysql started a ready\n");	
	return axl_true;
}

/** 
 * @brief Close function called once the myqttd server wants to
 * unload the module or it is being closed. All resource deallocation
 * and stop operation required must be done here.
 */
static void mod_auth_mysql_close (MyQttdCtx * ctx)
{
	axlDoc * doc = modconfig;

	if (doc == NULL)
		return;
	modconfig = NULL;
	axl_doc_free (doc);

	/* finish thread and library */
	mysql_thread_end ();
	mysql_library_end ();
	
	/* for now nothing */
	return;
}

/** 
 * @brief The reconf function is used by myqttd to notify to all
 * its modules loaded that a reconfiguration signal was received and
 * modules that could have configuration and run time change support,
 * should reread its files. It is an optional handler.
 */
static void mod_auth_mysql_reconf (MyQttdCtx * ctx) {
	/* for now nothing */
	return;
}

/** 
 * @brief Public entry point for the module to be loaded. This is the
 * symbol the myqttd will lookup to load the rest of items.
 */
MyQttdModDef module_def = {
	"mod-auth-mysql",
	"Authentication backend support for auth-mysql",
	mod_auth_mysql_init,
	mod_auth_mysql_close,
	mod_auth_mysql_reconf,
	NULL,
};

END_C_DECLS


/** 
 * \page myqttd_mod_auth_mysql mod-auth-mysql Authentication and Acls plugin supported on MySQL server for MyQttd broker
 *
 * 
 * 
 *
 */
