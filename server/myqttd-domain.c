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

#include <myqttd-domain.h>
#include <myqttd-ctx-private.h>


void myqtt_domain_free (axlPointer _domain)
{
	MyQttdDomain * domain = _domain;

	/* release memory */
 	axl_free (domain->name);
 	axl_free (domain->storage_path);
	axl_free (domain->users_db);
	axl_free (domain);

	return;
}

/** 
 * @internal Initialize domain module on the provided server MyQttdCtx
 *
 * @param ctx The context where the operation takes place.
 *
 * @return axl_true in the case the module was initialized, othewise
 * axl_false is returned.
 */
axl_bool   myqttd_domain_init (MyQttdCtx  * ctx)
{
	
	/* init module */
	ctx->domains = myqtt_hash_new_full (axl_hash_string, 
					    axl_hash_equal_string,
					    NULL, /* name is a reference to the domain */
					    myqtt_domain_free);
	return axl_true;
}

/** 
 * @brief Allows to add a new domain with the provide initial data.
 *
 * @param ctx The context where the domain will be added.
 *
 * @param storage_path The storage path used by this domain to hold
 * messages in transit.
 *
 * @param user_db The storage path where user databse configuration is
 * found.
 */
axl_bool   myqttd_domain_add  (MyQttdCtx  * ctx, 
			       const char * name, 
			       const char * storage_path, 
			       const char * user_db)
{
	MyQttdDomain * domain;

	/* check input parameters */
	if (ctx == NULL || name == NULL || storage_path == NULL || user_db == NULL)
		return axl_false;

	/* ensure first the is no domain with the same name */
	if (myqtt_hash_exists (ctx->domains, (axlPointer) name)) {
		error ("Unable to add domain %s because it is already registered", name);
		return axl_false; /* unable to add domain because it does exists */
	} /* end if */

	/* it does not exists, add it */
	domain = axl_new (MyQttdDomain, 1);
	if (domain == NULL)
		return axl_false; /* failed to allocate memory for domain, unable to add it */

	/* copy content */
	domain->name         = axl_strdup (name);
	domain->storage_path = axl_strdup (storage_path);
	domain->users_db     = axl_strdup (user_db);

	/* add it into the domain hashes */
	myqtt_hash_insert (ctx->domains, domain->name, domain);
	
	/* report added */
	return axl_true;
}

/** 
 * @brief Allows to find the corresponding domain for the given
 * username, client id and serverName.
 *
 * @param ctx The context where the operation takes place.
 *
 * @param username The user name to lookfor (optional)
 *
 * @param client_id The client id provided by the remote connecting device.
 *
 * @param server_name Optional server name indication to locate the
 * domain.
 *
 * @return The domain reference object or NULL if it failed to find a valid domain.
 * 
 */
MyQttdDomain    * myqttd_domain_find_by_indications (MyQttdCtx  * ctx,
						     const char * username,
						     const char * client_id,
						     const char * server_Name)
{
	return NULL;
}

/** 
 * @brief Allows to authenticate the provided username, password and
 * client_id on the provided domain.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param domain The domain where the auth operation will take place.
 *
 * @param username The username to check.
 *
 * @param password The password to check.
 *
 * @param client_id The client id to check.
 *
 * @return axl_true if the authentication was ok, otherwise, axl_false
 * is returned.
 */
axl_bool          myqttd_domain_do_auth (MyQttdCtx    * ctx,
					 MyQttdDomain * domain,
					 const char   * username, 
					 const char   * password,
					 const char   * client_id)
{
	return axl_false;
}

/** 
 * @internal Cleanups the domain module for this context.
 */
void myqttd_domain_cleanup (MyQttdCtx * ctx)
{
	/* release domains */
	myqtt_hash_clear (ctx->domains);
	ctx->domains = NULL;

	return;
}

