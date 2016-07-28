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
#ifndef __MYQTTD_CONFIG_H__
#define __MYQTTD_CONFIG_H__

#include <myqttd.h>

axl_bool        myqttd_config_load     (MyQttdCtx  * ctx, 
					const char * config);

void            myqttd_config_cleanup  (MyQttdCtx * ctx);

axlDoc        * myqttd_config_get      (MyQttdCtx * ctx);

axl_bool        myqttd_config_set      (MyQttdCtx * ctx,
					const char    * path,
					const char    * attr_name,
					const char    * attr_value);

/** 
 * @brief Allows to get the node associated on a particular path on
 * inside myqttd configuration.
 *
 * @param path The path that will be used to search the node inside
 * the myqttd configuration file (myqttd.conf).
 *
 * @return A reference to the node pointing to the path or NULL if
 * nothing is found.
 *
 * <b>Examples:</b>
 *
 * To know if a particular variable is enabled at the myqttd
 * configuration file use something like:
 * \code
 * value = myqttd_config_is_attr_positive (ctx, MYQTTD_CONFIG_PATH ("/myqttd/global-settings/close-conn-on-start-failure"), "value");
 * \endcode
 */
#define MYQTTD_CONFIG_PATH(path) axl_doc_get (myqttd_config_get (ctx), path)

void            myqttd_config_ensure_attr (MyQttdCtx * ctx, axlNode * node, const char * attr_name);

axl_bool        myqttd_config_is_attr_positive (MyQttdCtx * ctx,
						axlNode       * node,
						const char    * attr_name);

axl_bool        myqttd_config_is_attr_negative (MyQttdCtx * ctx,
						axlNode       * node,
						const char    * attr_name);

axl_bool        myqttd_config_exists_attr (MyQttdCtx     * ctx,
					   const char    * path,
					   const char    * attr_name);

int             myqttd_config_get_number (MyQttdCtx * ctx, 
					  const char    * path,
					  const char    * attr_name);

/*** private API ***/
axlDoc * __myqttd_config_load_from_file (MyQttdCtx * ctx, const char * config);

#endif
