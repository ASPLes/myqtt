/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2015 Advanced Software Production Line, S.L.
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

#ifndef __MYQTT_H__
#define __MYQTT_H__

/**
 * \addtogroup myqtt
 * @{
 */

/* define default socket pool size for the MYQTT_IO_WAIT_SELECT
 * method. If you change this value, you must change the
 * following. This value must be synchronized with FD_SETSIZE. This
 * has been tested on windows to work properly, but under GNU/Linux,
 * the GNUC library just rejects providing this value, no matter where
 * you place them. The only solutions are:
 *
 * [1] Modify /usr/include/bits/typesizes.h the macro __FD_SETSIZE and
 *     update the following values: FD_SETSIZE and MYQTT_FD_SETSIZE.
 *
 * [2] Use better mechanism like poll or epoll which are also available 
 *     in the platform that is giving problems.
 * 
 * [3] The last soluction could be the one provided by you. Please report
 *     any solution you may find.
 **/
#ifndef MYQTT_FD_SETSIZE
#define MYQTT_FD_SETSIZE 1024
#endif
#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif

/** 
 * @brief MQTT maximum message size.
 */
#define MYQTT_MAX_MSG_SIZE (268435455)

/* External header includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Axl library headers */
#include <axl.h>

/* Direct portable mapping definitions */
#if defined(AXL_OS_UNIX)

/* Portable definitions while using MyQtt Library */
#define MYQTT_EINTR           EINTR
#define MYQTT_EWOULDBLOCK     EWOULDBLOCK
#define MYQTT_EINPROGRESS     EINPROGRESS
#define MYQTT_EAGAIN          EAGAIN
#define MYQTT_SOCKET          int
#define MYQTT_INVALID_SOCKET  -1
#define MYQTT_SOCKET_ERROR    -1
#define myqtt_close_socket(s) do {if ( s >= 0) {close (s);}} while (0)
#define myqtt_getpid          getpid
#define myqtt_sscanf          sscanf
#define myqtt_is_disconnected (errno == EPIPE)
#define MYQTT_FILE_SEPARATOR "/"

#endif /* end defined(AXL_OS_UNIX) */

#if defined(AXL_OS_WIN32)

/* additional includes for the windows platform */

/* _WIN32_WINNT note: If the application including the header defines
 * the _WIN32_WINNT, it must include the bit defined by the value
 * 0x501 (formerly 0x400). */
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x501

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <time.h>

#define MYQTT_EINTR           WSAEINTR
#define MYQTT_EWOULDBLOCK     WSAEWOULDBLOCK
#define MYQTT_EINPROGRESS     WSAEINPROGRESS
#define MYQTT_EAGAIN          WSAEWOULDBLOCK
#define SHUT_RDWR              SD_BOTH
#define SHUT_WR                SD_SEND
#define MYQTT_SOCKET          SOCKET
#define MYQTT_INVALID_SOCKET  INVALID_SOCKET
#define MYQTT_SOCKET_ERROR    SOCKET_ERROR
#define myqtt_close_socket(s) do {if ( s >= 0) {closesocket (s);}} while (0)
#define myqtt_getpid          _getpid
#define myqtt_sscanf          sscanf
#define uint16_t               u_short
#define myqtt_is_disconnected ((errno == WSAESHUTDOWN) || (errno == WSAECONNABORTED) || (errno == WSAECONNRESET))
#define MYQTT_FILE_SEPARATOR "\\"
#define inet_ntop myqtt_win32_inet_ntop

/* no link support windows */
#define S_ISLNK(m) (0)

#endif /* end defined(AXL_OS_WINDOWS) */

#if defined(AXL_OS_UNIX)
#include <sys/types.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#endif

/* additional headers for poll support */
#if defined(MYQTT_HAVE_POLL)
#include <sys/poll.h>
#endif

/* additional headers for linux epoll support */
#if defined(MYQTT_HAVE_EPOLL)
#include <sys/epoll.h>
#endif

/* Check gnu extensions, providing an alias to disable its precence
 * when no available. */
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8)
#  define GNUC_EXTENSION __extension__
#else
#  define GNUC_EXTENSION
#endif

/* define minimal support for int64 constants */
#ifndef _MSC_VER 
#  define INT64_CONSTANT(val) (GNUC_EXTENSION (val##LL))
#else /* _MSC_VER */
#  define INT64_CONSTANT(val) (val##i64)
#endif

/* check for missing definition for S_ISDIR */
#ifndef S_ISDIR
#  ifdef _S_ISDIR
#    define S_ISDIR(x) _S_ISDIR(x)
#  else
#    ifdef S_IFDIR
#      ifndef S_IFMT
#        ifdef _S_IFMT
#          define S_IFMT _S_IFMT
#        endif
#      endif
#       ifdef S_IFMT
#         define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#       endif
#    endif
#  endif
#endif

/* check for missing definition for S_ISREG */
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
# define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif 

/** 
 * @brief Allows to check the reference provided, and returning the
 * return value provided.
 * @param ref The reference to be checke for NULL.
 * @param return_value The return value to return in case of NULL reference.
 */
#define MYQTT_CHECK_REF(ref, return_value) do { \
	if (ref == NULL) {   		         \
             return return_value;                \
	}                                        \
} while (0)

/** 
 * @brief Allows to check the reference provided, returning the return
 * value provided, also releasing a second reference with a custom
 * free function.
 * @param ref The reference to be checke for NULL.
 * @param return_value The return value to return in case of NULL reference.
 * @param ref2 Second reference to be released
 * @param free2_func Function to be used to release the second reference.
 */
#define MYQTT_CHECK_REF2(ref, return_value, ref2, free2_func) do { \
        if (ref == NULL) {                                          \
               free2_func (ref);                                    \
	       return return_value;                                 \
	}                                                           \
} while (0)

BEGIN_C_DECLS

/* Internal includes and external includes for MyQtt API
 * consumers. */
#include <myqtt-types.h>
#include <myqtt-support.h>
#include <myqtt-handlers.h>
#include <myqtt-hash.h>
#include <myqtt-ctx.h>
#include <myqtt-thread.h>
#include <myqtt-thread-pool.h>
#include <myqtt-conn.h>
#include <myqtt-listener.h>
#include <myqtt-io.h>
#include <myqtt-reader.h>
#include <myqtt-errno.h>
#include <myqtt-sequencer.h>
#include <myqtt-msg.h>
#include <myqtt-storage.h>

END_C_DECLS

#if defined(AXL_OS_WIN32)
#include <myqtt-win32.h>
#endif

#include <errno.h>

#if defined(AXL_OS_WIN32)
/* errno redefinition for windows platform. this declaration must
 * follow the previous include. */
#ifdef  errno
#undef  errno
#endif
#define errno (WSAGetLastError())
#endif

/* console debug support:
 *
 * If enabled, the log reporting is activated as usual. If log is
 * stripped from myqtt building all instructions are removed.
 */
#if defined(ENABLE_MYQTT_LOG)
# define myqtt_log(l, m, ...)   do{_myqtt_log  (ctx, __AXL_FILE__, __AXL_LINE__, l, m, ##__VA_ARGS__);}while(0)
# define myqtt_log2(l, m, ...)   do{_myqtt_log2  (ctx, __AXL_FILE__, __AXL_LINE__, l, m, ##__VA_ARGS__);}while(0)
#else
# if defined(AXL_OS_WIN32) && !( defined(__GNUC__) || _MSC_VER >= 1400)
/* default case where '...' is not supported but log is still
 * disabled */
#   define myqtt_log _myqtt_log
#   define myqtt_log2 _myqtt_log2
# else
#   define myqtt_log(l, m, ...) /* nothing */
#   define myqtt_log2(l, m, message, ...) /* nothing */
# endif
#endif

/** 
 * @internal The following definition allows to find printf like wrong
 * argument passing to myqtt_log function. To activate the depuration
 * just add the following header after this comment.
 *
 * #define SHOW_FORMAT_BUGS (1)
 */
#if defined(SHOW_FORMAT_BUGS)
# undef  myqtt_log
# define myqtt_log(l, m, ...)   do{printf (m, ##__VA_ARGS__);}while(0)
#endif

/** 
 * @internal Allows to check a condition and return if it is not meet.
 * 
 * @param expr The expresion to check.
 */
#define v_return_if_fail(expr) \
if (!(expr)) {return;}

/** 
 * @internal Allows to check a condition and return the given value if it
 * is not meet.
 * 
 * @param expr The expresion to check.
 *
 * @param val The value to return if the expression is not meet.
 */
#define v_return_val_if_fail(expr, val) \
if (!(expr)) { return val;}

/** 
 * @internal Allows to check a condition and return if it is not
 * meet. It also provides a way to log an error message.
 * 
 * @param expr The expresion to check.
 *
 * @param msg The message to log in the case a failure is found.
 */
#define v_return_if_fail_msg(expr,msg) \
if (!(expr)) {myqtt_log (MYQTT_LEVEL_CRITICAL, "%s: %s", __AXL_PRETTY_FUNCTION__, msg); return;}

/** 
 * @internal Allows to check a condition and return the given value if
 * it is not meet. It also provides a way to log an error message.
 * 
 * @param expr The expresion to check.
 *
 * @param val The value to return if the expression is not meet.
 *
 * @param msg The message to log in the case a failure is found.
 */
#define v_return_val_if_fail_msg(expr, val, msg) \
if (!(expr)) { myqtt_log (MYQTT_LEVEL_CRITICAL, "%s: %s", __AXL_PRETTY_FUNCTION__, msg); return val;}


BEGIN_C_DECLS

axl_bool myqtt_init_ctx             (MyQttCtx * ctx);

axl_bool myqtt_init_check           (MyQttCtx * ctx);

void     myqtt_exit_ctx             (MyQttCtx * ctx, 
				     axl_bool    free_ctx);

axl_bool myqtt_is_exiting           (MyQttCtx * ctx);

axl_bool myqtt_log_is_enabled       (MyQttCtx * ctx);

axl_bool myqtt_log2_is_enabled      (MyQttCtx * ctx);

void     myqtt_log_enable           (MyQttCtx * ctx, 
				      axl_bool    status);

void     myqtt_log2_enable          (MyQttCtx * ctx, 
				      axl_bool    status);

axl_bool myqtt_color_log_is_enabled (MyQttCtx * ctx);

void     myqtt_color_log_enable     (MyQttCtx * ctx, 
				      axl_bool    status);

axl_bool myqtt_log_is_enabled_acquire_mutex (MyQttCtx * ctx);

void     myqtt_log_acquire_mutex            (MyQttCtx * ctx, 
					      axl_bool    status);

void     myqtt_log_set_handler      (MyQttCtx         * ctx,
				     MyQttLogHandler    handler,
				     axlPointer         user_data);

void     myqtt_log_set_prepare_log  (MyQttCtx         * ctx,
				      axl_bool            prepare_string);

MyQttLogHandler myqtt_log_get_handler (MyQttCtx      * ctx);

void     myqtt_log_filter_level     (MyQttCtx * ctx, const char * filter_string);

axl_bool    myqtt_log_filter_is_enabled (MyQttCtx * ctx);

/**
 * @brief Allowed items to use for \ref myqtt_conf_get.
 */
typedef enum {
	/** 
	 * @brief Gets/sets current soft limit to be used by the library,
	 * regarding the number of connections handled. Soft limit
	 * means it is can be moved to hard limit.
	 *
	 * To configure this value, use the integer parameter at \ref myqtt_conf_set. Example:
	 * \code
	 * myqtt_conf_set (MYQTT_SOFT_SOCK_LIMIT, 4096, NULL);
	 * \endcode
	 */
	MYQTT_SOFT_SOCK_LIMIT = 1,
	/** 
	 * @brief Gets/sets current hard limit to be used by the
	 * library, regarding the number of connections handled. Hard
	 * limit means it is not possible to exceed it.
	 *
	 * To configure this value, use the integer parameter at \ref myqtt_conf_set. Example:
	 * \code
	 * myqtt_conf_set (MYQTT_HARD_SOCK_LIMIT, 4096, NULL);
	 * \endcode
	 */
	MYQTT_HARD_SOCK_LIMIT = 2,
	/** 
	 * @brief Gets/sets current backlog configuration for listener
	 * connections.
	 *
	 * Once a listener is activated, the backlog is the number of
	 * complete connections (with the finished tcp three-way
	 * handshake), that are ready to be accepted by the
	 * application. The default value is 5.
	 *
	 * Once a listener is activated, and its backlog is
	 * configured, it can't be changed. In the case you configure
	 * this value, you must set it (\ref myqtt_conf_set) after
	 * calling to the family of functions to create myqtt
	 * listeners (\ref myqtt_listener_new).
	 *
	 * To configure this value, use the integer parameter at \ref myqtt_conf_set. Example:
	 * \code
	 * myqtt_conf_set (MYQTT_LISTENER_BACKLOG, 64, NULL);
	 * \endcode
	 */
	MYQTT_LISTENER_BACKLOG = 3,
	/** 
	 * @brief Allows to skip thread pool waiting on myqtt ctx finalization.
	 *
	 * By default, when myqtt context is finished by calling \ref
	 * myqtt_exit_ctx, the function waits for all threads running
	 * the in thread pool to finish. However, under some
	 * conditions, this may cause a dead-lock problem especially
	 * when blocking operations are triggered from threads inside the
	 * pool at the time the exit operation happens.
	 *
	 * This parameter allows to signal this myqtt context to not
	 * wait for threads running in the thread pool.
	 *
	 * To set the value to make myqtt ctx exit to not wait for
	 * threads in the pool to finish use:
	 *
	 * \code
	 * myqtt_conf_set (ctx, MYQTT_SKIP_THREAD_POOL_WAIT, axl_true, NULL);
	 * \endcode
	 */
	MYQTT_SKIP_THREAD_POOL_WAIT = 6
} MyQttConfItem;

axl_bool  myqtt_conf_get             (MyQttCtx      * ctx,
				       MyQttConfItem   item, 
				       int            * value);

axl_bool  myqtt_conf_set             (MyQttCtx      * ctx,
				       MyQttConfItem   item, 
				       int              value, 
				       const char     * str_value);

void     _myqtt_log                 (MyQttCtx        * ctx,
				      const       char * file,
				      int                line,
				      MyQttDebugLevel   level, 
				      const char       * message,
				      ...);

void     _myqtt_log2                (MyQttCtx        * ctx,
				      const       char * file,
				      int                line,
				      MyQttDebugLevel   level, 
				      const char       * message, 
				      ...);

int    myqtt_get_bit (unsigned char byte, int position);

void   myqtt_set_bit     (unsigned char * buffer, int position);

void   myqtt_show_byte (MyQttCtx * ctx, unsigned char byte, const char * label);

unsigned char * myqtt_int2bin (int a, unsigned char *buffer, int buf_size);

void   myqtt_int2bin_print (MyQttCtx * ctx, int value);

int    myqtt_get_8bit  (const unsigned char * buffer);

int    myqtt_get_16bit (const unsigned char * buffer);

void   myqtt_set_16bit (int value, unsigned char * buffer);

void   myqtt_set_32bit (int value, unsigned char * buffer);

int    myqtt_get_32bit (const unsigned  char * buffer);

void   myqtt_sleep (long microseconds);

#if defined(__COMPILING_MYQTT__) && defined(__GNUC__)
/* makes gcc happy, by prototyping functions which aren't exported
 * while compiling with -ansi. Really uggly hack, please report
 * any idea to solve this issue. */
int  setenv  (const char *name, const char *value, int overwrite);
#endif

int    myqtt_mkdir (const char * path, int mode);

END_C_DECLS

/* @} */
#endif
