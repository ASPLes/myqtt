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
#ifndef __MYQTTD_MODDEF_H__
#define __MYQTTD_MODDEF_H__

/** 
 * \defgroup myqttd_moddef MyQttd Module Def: Type and handler definitions for myqttd modules
 */

/**
 * \addtogroup myqttd_moddef
 * @{
 */

/** 
 * @brief Public definition for the init function that must implement
 * a myqttd module.
 *
 * The module must return axl_true to signal the modules was
 * properly initialized so myqttd can register it as usable.
 *
 * This handler is called only once at myqttd startup.
 *
 * @param ctx The myqttd context where the init operation is
 * taking place.
 * 
 * @return axl_true if the module is usable or axl_false if
 * not. Returning axl_false caused the module to be not loaded.
 */
typedef axl_bool  (*ModInitFunc)  (MyQttdCtx * ctx);

/** 
 * @brief Public definition for the close function that must implement
 * all operations required to unload and terminate a module.
 * 
 * The function doesn't receive and return any data.
 *
 * @param ctx The myqttd context where the close operation is
 * taking place.
 */
typedef void (*ModCloseFunc) (MyQttdCtx * ctx);

/** 
 * @brief A reference to the unload method that must implement all
 * unload code required in the case myqttd ask the module
 * to stop its function.
 * 
 * @param ctx The myqttd context where the close operation is
 * taking place.
 */
typedef void (*ModUnloadFunc) (MyQttdCtx * ctx);

/** 
 * @brief Public definition for the reconfiguration function that must
 * be implemented to receive notification if the myqttd server
 * configuration is reloaded (either because a signal was received or
 * because some module has called \ref myqttd_reload_config).
 *
 * @param ctx The myqttd context where the reconf operation is
 * taking place.
 */
typedef void (*ModReconfFunc) (MyQttdCtx * ctx);

/** 
 * @brief Public definition for the main entry point for all modules
 * developed for myqttd. 
 *
 * This structure contains pointers to all functions that may
 * implement a myqttd module.
 *
 */
typedef struct _MyQttdModDef {
	/** 
	 * @brief The module name. This name is used by myqttd to
	 * refer to the module.
	 */
	char         * mod_name;

	/** 
	 * @brief The module long description.
	 */
	char         * mod_description;

	/** 
	 * @brief A reference to the init function associated to the
	 * module.
	 */
	ModInitFunc    init;

	/** 
	 * @brief A reference to the close function associated to the
	 * module.
	 */
	ModCloseFunc   close;
	
	/** 
	 * @brief A reference to the reconf function associated to the
	 * module.
	 */
	ModReconfFunc  reconf;

	/** 
	 * @brief A reference to the unload function associated to the
	 * module.
	 */
	ModUnloadFunc  unload;
} MyQttdModDef;

/** 
 * @brief Allows to prepare de module with the myqttd context (and
 * the myqtt context associated).
 *
 * This macro must be called inside the module init, before any
 * operation is done. 
 * 
 * @param _ctx The context received by the module at the init functio.
 */
#define MYQTTD_MOD_PREPARE(_ctx) do{ctx = _ctx;}while(0)

#endif

/* @} */
