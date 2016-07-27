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
		if (input[iterator] == '\'' || input[iterator] == '-' || input[iterator] == ';')
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



axl_bool __mod_auth_mysql_accept_user (MyQttdCtx * ctx, MyQttdDomain * domain, MyQttdUsers * users, MyQttConn * conn, int user_id)
{
	if (user_id > 0) {
		/* save user_id into connection for later use */
		myqtt_conn_set_data (conn, "mod:mysql:auth:user_id", INT_TO_PTR (user_id));
	} /* end if */
	
	/* record here accepted connection from a given IP, user, etc */
	
	return axl_true; /* connection accepted */
}

axl_bool __mod_auth_mysql_reject_user (MyQttdCtx * ctx, MyQttdDomain * domain, MyQttdUsers * users, MyQttConn * conn, const char * reason)
{
	/* record here rejected connection from a given ip, user, etc */
	return axl_false; /* connection rejected */
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
	char      * digest_password;
	axl_bool    result;
	int         user_id;
	
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
	if (! __mod_auth_mysql_check_input (ctx, password)) 
		return axl_false;

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

	/* domain is defined in tables, let's continue, but first,
	 * check for anonymous connections accepted for this domain */
	anonymous = myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, res, row, "anonymous"), NULL);
	if (anonymous && domain_selected) {
		/* accept connection since anonymous is enabled */
		return __mod_auth_mysql_accept_user (ctx, domain, users, conn, -1); 
	} /* end if */
	mysql_free_result (res);

	switch (check_mode) {
	case 3:
		/* query database */
		res      = mod_auth_mysql_run_query (ctx, dsn_node,
						     "SELECT id, password from user WHERE is_active = '1' AND require_auth = 1 AND domain_id = (SELECT id FROM domain WHERE is_active = '1' AND name = '%s') AND BINARY username = '%s' AND BINARY clientid = '%s'",
						     myqttd_domain_get_name (domain), user_name, clientid);
		if (res == NULL) {
			return __mod_auth_mysql_reject_user (ctx, domain, users, conn, "Username/Clientid/Password combination unknown for this domain, SQL failure");
		} /* end if */
		
		/* get first row from result */
		row      = mysql_fetch_row (res);
		if (row == NULL || row[1] == NULL) {
			mysql_free_result (res);
			return __mod_auth_mysql_reject_user (ctx, domain, users, conn, "Username/clientid combination unknown for this domain");
		} /* end if */

		/* user and client id (and password) */
		digest_password = myqtt_tls_get_digest (MYQTT_MD5, password);
		if (digest_password == NULL) 
			return __mod_auth_mysql_reject_user (ctx, domain, users, conn, "Unable to encode password into MD5 for query");

		/* get result */
		result  = axl_cmp (digest_password, row[1]);
		user_id = myqtt_support_strtod (row[0], NULL);
		axl_free (digest_password);
		mysql_free_result (res);
		
		if (result)
			return __mod_auth_mysql_accept_user (ctx, domain, users, conn, user_id);
		return __mod_auth_mysql_reject_user (ctx, domain, users, conn, "Password failure for client-id/username");
	case 2:
		/* query database */
		res      = mod_auth_mysql_run_query (ctx, dsn_node,
						     "SELECT clientid, id from user WHERE is_active = '1' AND  require_auth = 0 AND domain_id = (SELECT id FROM domain WHERE is_active = '1' AND  name = '%s') AND BINARY clientid = '%s'",
						     myqttd_domain_get_name (domain), clientid);
		if (res == NULL) {
			return __mod_auth_mysql_reject_user (ctx, domain, users, conn, "Clientid combination unknown for this domain, SQL failure");
		} /* end if */
			
		/* get first row from result */
		row  = mysql_fetch_row (res);
		if (row == NULL || row[0] == NULL) {
			mysql_free_result (res);
			return __mod_auth_mysql_reject_user (ctx, domain, users, conn, "Username/clientid combination unknown for this domain");
		} /* end if */

		/* get result */
		result  = axl_cmp (clientid, row[0]);
		user_id = myqtt_support_strtod (row[1], NULL);
		mysql_free_result (res);
		
		if (result)
			return __mod_auth_mysql_accept_user (ctx, domain, users, conn, user_id);
		return __mod_auth_mysql_reject_user (ctx, domain, users, conn, "Client-id unknown");
	case 1:
		/* case not supported */
		return __mod_auth_mysql_reject_user (ctx, domain, users, conn, "Remove device only provided username");
		
	} /* end switch */
		
	/* user does not exists for this backend */
	return __mod_auth_mysql_reject_user (ctx, domain, users, conn, "Remove device did not provided any valid credential");
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

MyQttPublishCodes __mod_auth_mysql_apply_acl (MyQttdCtx * ctx, MyQttdDomain * domain, MyQttCtx * myqtt_ctx,
					      MyQttConn * conn, MyQttMsg * msg, axl_bool apply_after, axl_bool is_domain_acl, MYSQL_RES * acls)
{
	MYSQL_ROW    row;
	axl_bool     local_apply_after;
	const char * topic_filter;

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
			/* filter matches, apply option indicated */
			if (myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, acls, row, "w"), NULL))
				return MYQTT_PUBLISH_OK;
			if (myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, acls, row, "publish"), NULL))
				return MYQTT_PUBLISH_OK;
			if (myqtt_msg_get_qos (msg) == MYQTT_QOS_0 && myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, acls, row, "publish0"), NULL))
				return MYQTT_PUBLISH_OK;
			if (myqtt_msg_get_qos (msg) == MYQTT_QOS_1 && myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, acls, row, "publish1"), NULL))
				return MYQTT_PUBLISH_OK;
			if (myqtt_msg_get_qos (msg) == MYQTT_QOS_2 && myqtt_support_strtod (__mod_auth_mysql_get_by_name (ctx, acls, row, "publish2"), NULL))
				return MYQTT_PUBLISH_OK;
		} /* end if */
		

		/* get next row */
		row = mysql_fetch_row (acls);
	} /* end if */

	return MYQTT_PUBLISH_DUNNO; /* no acl reported nothing especial, so let's continue */
}

MyQttPublishCodes __mod_auth_mysql_get_default_acl (MyQttdCtx * ctx, MyQttConn * conn)
{
	/* report value configured on the provided connection */
	int default_acl = PTR_TO_INT (myqtt_conn_get_data (conn, "mod:mysql:auth:default_acl"));
	if (default_acl > 0)
		return default_acl;
	
	return MYQTT_PUBLISH_OK; /* if nothing is configured, default allow */
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
	if (config == NULL)
		return axl_false; /* allocation failure */
	
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
					     "description", "text");

		/* user table */
		mod_auth_mysql_ensure_table (ctx, dsn_node,
					     "user",
					     "id", "autoincrement int",
					     "is_active", "int",
					     "domain_id", "int",
					     "user_id", "int", 
					     "require_auth", "int",
					     "clientid", "varchar(512)",
					     "username", "varchar(512)",
					     "password", "varchar(512)",
					     "description", "text");

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
					     "description", "text");

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
					     "description", "text");
					     
		
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
