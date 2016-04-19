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

/** 
 * \addtogroup myqttd
 * @{
 */

#ifndef __MYQTTD_H__
#define __MYQTTD_H__

/* system includes */
#include <stdarg.h>
#include <sys/types.h>
#include <dirent.h>

/* XML support */
#include <axl.h>

/* MQTT support ;-) */
#include <myqtt.h>

/* local includes */
#include <myqttd-types.h>
#include <myqttd-handlers.h>
#include <myqttd-ctx.h>
#include <myqttd-support.h>
#include <myqttd-signal.h>
#include <myqttd-moddef.h>
#include <myqttd-config.h>
#include <myqttd-run.h>
#include <myqttd-module.h>
#include <myqttd-log.h>
#include <myqttd-conn-mgr.h>
#include <myqttd-loop.h>
#include <myqttd-child.h>
#include <myqttd-process.h>
#include <myqttd-domain.h>
#include <myqttd-users.h>

axl_bool  myqttd_log_enabled      (MyQttdCtx * ctx);

void      myqttd_log_enable       (MyQttdCtx * ctx, 
				       int  value);

axl_bool  myqttd_log2_enabled     (MyQttdCtx * ctx);

void      myqttd_log2_enable      (MyQttdCtx * ctx,
				       int  value);

axl_bool  myqttd_log3_enabled     (MyQttdCtx * ctx);

void      myqttd_log3_enable      (MyQttdCtx * ctx,
				       int  value);

void      myqttd_color_log_enable (MyQttdCtx * ctx,
				       int             value);

/** 
 * Drop an error msg to the console stderr.
 *
 * To drop an error message use:
 * \code
 *   error ("unable to open file: %s", file);
 * \endcode
 * 
 * @param m The error message to output.
 */
#define error(m,...) do{myqttd_error (ctx, axl_false, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  myqttd_error (MyQttdCtx * ctx, axl_bool ignore_debug, const char * file, int line, const char * format, ...);

/** 
 * Drop an error msg to the console stderr without taking into
 * consideration debug configuration.
 *
 * To drop an error message use:
 * \code
 *   abort_error ("unable to open file: %s", file);
 * \endcode
 * 
 * @param m The error message to output.
 */
#define abort_error(m,...) do{myqttd_error (ctx, axl_true, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)

/** 
 * Drop a msg to the console stdout.
 *
 * To drop a message use:
 * \code
 *   msg ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The console message to output.
 */
#define msg(m,...)   do{myqttd_msg (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  myqttd_msg   (MyQttdCtx * ctx, const char * file, int line, const char * format, ...);

/** 
 * Drop a second level msg to the console stdout.
 *
 * To drop a message use:
 * \code
 *   msg2 ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The console message to output.
 */
#define msg2(m,...)   do{myqttd_msg2 (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  myqttd_msg2   (MyQttdCtx * ctx, const char * file, int line, const char * format, ...);



/** 
 * Drop a warning msg to the console stdout.
 *
 * To drop a message use:
 * \code
 *   wrn ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The warning message to output.
 */
#define wrn(m,...)   do{myqttd_wrn (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  myqttd_wrn   (MyQttdCtx * ctx, const char * file, int line, const char * format, ...);

/** 
 * Drops to the console stdout a warning, placing the content prefixed
 * with the file and the line that caused the message, without
 * introducing a new line.
 *
 * To drop a message use:
 * \code
 *   wrn_sl ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The warning message to output.
 */
#define wrn_sl(m,...)   do{myqttd_wrn_sl (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  myqttd_wrn_sl   (MyQttdCtx * ctx, const char * file, int line, const char * format, ...);

/** 
 * Reports an access message, a message that is sent to the access log
 * file. The message must contain access to the server information.
 *
 * To drop a message use:
 * \code
 *   access ("module loaded: %s", module);
 * \endcode
 * 
 * @param m The console message to output.
 */
#define tbc_access(m,...)   do{myqttd_access (ctx, __AXL_FILE__, __AXL_LINE__, m, ##__VA_ARGS__);}while(0)
void  myqttd_access   (MyQttdCtx * ctx, const char * file, int line, const char * format, ...);

/** 
 * @internal The following definition allows to find printf like wrong
 * argument passing to nopoll_log function. To activate the depuration
 * just add the following header after this comment.
 *
 * #define SHOW_FORMAT_BUGS (1)
 */
#if defined(SHOW_FORMAT_BUGS)
# undef error
# undef msg
# undef msg2
# undef wrn
# undef wrn_sl
# undef tbc_access
#define error(m,...) do{printf (m, ##__VA_ARGS__);}while(0)
#define msg(m,...)   do{printf (m, ##__VA_ARGS__);}while(0)
#define msg2(m,...)   do{printf (m, ##__VA_ARGS__);}while(0)
#define wrn(m,...)   do{printf (m, ##__VA_ARGS__);}while(0)
#define wrn_sl(m,...)   do{printf (m, ##__VA_ARGS__);}while(0)
#define tbc_access(m,...)   do{printf (m, ##__VA_ARGS__);}while(0)
#endif

axl_bool myqttd_init                (MyQttdCtx  * ctx, 
				     MyQttCtx   * myqtt_ctx,
				     const char * config);

void     myqttd_exit                (MyQttdCtx * ctx,
				     axl_bool        free_ctx,
				     axl_bool        free_myqtt_ctx);

void     myqttd_reload_config       (MyQttdCtx * ctx, int value);

axl_bool myqttd_file_test_v         (const char    * format, 
				     MyQttFileTest   test, ...);

axl_bool myqttd_create_dir          (const char * path);

axl_bool myqttd_unlink              (const char * path);

long     myqttd_last_modification   (const char * file);

axl_bool myqttd_file_is_fullpath    (const char * file);

char   * myqttd_base_dir            (const char * path);

char   * myqttd_file_name           (const char * path);

typedef enum {
	DISABLE_STDIN_ECHO = 1 << 0,
} MyqttdIoFlags;

char *   myqttd_io_get (char * prompt, MyqttdIoFlags flags);

/* enviroment configuration */
const char    * myqttd_sysconfdir      (MyQttdCtx * ctx);

const char    * myqttd_datadir         (MyQttdCtx * ctx);

const char    * myqttd_runtime_datadir (MyQttdCtx * ctx);

const char    * myqttd_runtime_tmpdir  (MyQttdCtx * ctx);

axl_bool        myqttd_is_num          (const char * value);

int             myqttd_get_system_id   (MyQttdCtx     * ctx,
					const char    * value, 
					axl_bool        get_user);

axl_bool        myqttd_change_fd_owner (MyQttdCtx     * ctx,
					const char    * file_name,
					const char    * user,
					const char    * group);

axl_bool        myqttd_change_fd_perms (MyQttdCtx     * ctx,
					const char    * file_name,
					const char    * mode);

void            myqttd_sleep           (MyQttdCtx * ctx,
					long        microseconds);

const char    * myqttd_ensure_str      (const char * string);

#endif

/* @} */
