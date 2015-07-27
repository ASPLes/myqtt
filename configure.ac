dnl  MyQtt: A high performance open source MQTT implementation
dnl  Copyright (C) 2014 Advanced Software Production Line, S.L.
dnl
dnl  This program is free software; you can redistribute it and/or
dnl  modify it under the terms of the GNU Lesser General Public License
dnl  as published by the Free Software Foundation; either version 2.1
dnl  of the License, or (at your option) any later version.
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl  Lesser General Public License for more details.
dnl
dnl  You should have received a copy of the GNU Lesser General Public
dnl  License along with this program; if not, write to the Free
dnl  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
dnl  02111-1307 USA
dnl  
dnl  You may find a copy of the license under this software is released
dnl  at COPYING file. This is LGPL software: you are welcome to develop
dnl  proprietary applications using this library without any royalty or
dnl  fee but returning back any change, improvement or addition in the
dnl  form of source code, project image, documentation patches, etc.
dnl
dnl  For commercial support on build MQTT enabled solutions contact us:
dnl          
dnl      Postal address:
dnl         Advanced Software Production Line, S.L.
dnl         C/ Antonio Suarez Nº 10, 
dnl         Edificio Alius A, Despacho 102
dnl         Alcalá de Henares 28802 (Madrid)
dnl         Spain
dnl
dnl      Email address:
dnl         info@aspl.es - http://www.aspl.es/mqtt
dnl                        http://www.aspl.es/myqtt

AC_INIT(lib/myqtt.h)

configure_dir=`dirname "$0"`
MYQTT_VERSION=`cat $configure_dir/VERSION`
AC_SUBST(MYQTT_VERSION)

AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE(myqtt, $MYQTT_VERSION)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL

# compiler option
compiler_options=""
STRICT_PROTOTYPES=""
if test "$compiler" = "gcc" ; then
   compiler_options="-Wstrict-prototypes -Wall -Werror -g -fstack-protector-all -Wstack-protector"
   echo "Detected gcc compiler: $compiler, adding options: $compiler_options"
fi
AC_SUBST(compiler_options)
# ansi option
ansi_option=""
if test "$compiler" = "gcc" ; then
    ansi_option="-ansi"
    echo "Detected gcc compiler: $compiler, adding options: $ansi_option"
fi
AC_SUBST(ansi_option)

AC_CHECK_PROG(PKG_CONFIG, pkg-config, "yes", "no")
if test "$PKG_CONFIG" = "no" ; then
  AC_MSG_ERROR([You need to install pkg-config to compile MyQtt. See: http://pkgconfig.freedesktop.org/releases/])
fi

dnl
dnl Thread detection support mostly taken from the apache project 2.2.3.
dnl
dnl MYQTT_PTHREADS_TRY_RUN(actions-if-success)
dnl
dnl Try running a program which uses pthreads, executing the
dnl actions-if-success commands on success.
dnl
AC_DEFUN([MYQTT_PTHREADS_TRY_RUN], [
AC_TRY_RUN( [
#include <pthread.h>
#include <stddef.h>

void *thread_routine(void *data) {
    return data;
}

int main() {
    pthread_t thd;
    pthread_mutexattr_t mattr;
    pthread_once_t once_init = PTHREAD_ONCE_INIT;
    int data = 1;
    pthread_mutexattr_init(&mattr);
    return pthread_create(&thd, NULL, thread_routine, &data);
} ], [myqtt_p_t_r=yes], [myqtt_p_t_r=no], [myqtt_p_t_r=no])

if test $myqtt_p_t_r = yes; then
  $1
fi

])dnl


dnl
dnl MYQTT_PTHREADS_CHECK()
dnl
dnl Try to find a way to enable POSIX threads.  Sets the 
dnl pthreads_working variable to "yes" on success.
dnl
AC_DEFUN([MYQTT_PTHREADS_CHECK],[

AC_CACHE_CHECK([for CFLAGS needed for pthreads], [myqtt_cv_pthreads_cflags],
[myqtt_ptc_cflags=$CFLAGS
 for flag in -kthread -pthread -pthreads -mt -mthreads -Kthread -threads; do 
    CFLAGS=$myqtt_ptc_cflags
    test "x$flag" != "xnone" && CFLAGS="$CFLAGS $flag"
    MYQTT_PTHREADS_TRY_RUN([
      myqtt_cv_pthreads_cflags="$flag"
      break
    ])
 done
 CFLAGS=$myqtt_ptc_cflags
])

# The CFLAGS may or may not be sufficient to ensure that libmyqtt
# depends on the pthreads library: some versions of libtool
# drop -pthread when passed on the link line; some versions of
# gcc ignore -pthread when linking a shared object.  So always
# try and add the relevant library to LIBS too.

AC_CACHE_CHECK([for LIBS needed for pthreads], [myqtt_cv_pthreads_lib], [
  myqtt_ptc_libs=$LIBS
  for lib in -lpthread -lpthreads -lc_r; do
    LIBS="$myqtt_ptc_libs $lib"
    MYQTT_PTHREADS_TRY_RUN([
      myqtt_cv_pthreads_lib=$lib
      break
    ])
  done
  LIBS=$myqtt_ptc_libs
])

if test "$pthreads_working" = "yes"; then
  threads_result="POSIX Threads found"
else
  threads_result="POSIX Threads not found"
fi
])dnl

dnl call to detect thread activation support
MYQTT_PTHREADS_CHECK

PTHREAD_CFLAGS="$myqtt_cv_pthreads_cflags"
PTHREAD_LIBS="$myqtt_cv_pthreads_lib"
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)

dnl check mosquitto client library
AC_CHECK_HEADER(mosquitto.h, enable_mosquitto=yes, enable_mosquitto=no)
AM_CONDITIONAL(ENABLE_MOSQUITTO, test "x$enable_mosquitto" = "xyes")

dnl check for poll support
AC_CHECK_HEADER(sys/poll.h, enable_poll=yes, enable_poll=no)
AM_CONDITIONAL(ENABLE_POLL_SUPPORT, test "x$enable_poll" = "xyes")

dnl Check for the Linux epoll interface; epoll* may be available in libc
dnl with Linux kernels 2.6.X
AC_CACHE_CHECK([for epoll(2) support], [enable_cv_epoll],
[AC_TRY_RUN([
#include <sys/epoll.h>
#include <unistd.h>

int main()
{
    return epoll_create(5) == -1;
}], [enable_cv_epoll=yes], [enable_cv_epoll=no], [enable_cv_epoll=no])])
AM_CONDITIONAL(ENABLE_EPOLL_SUPPORT, test "x$enable_cv_epoll" = "xyes")

dnl select the best I/O platform
if test x$enable_cv_epoll = xyes ; then
   default_platform="epoll"
elif test x$enable_poll = xyes ; then
   default_platform="poll"
else 
   default_platform="select"
fi
AM_CONDITIONAL(DEFAULT_EPOLL, test "x$default_platform" = "xepoll")
AM_CONDITIONAL(DEFAULT_POLL, test "x$default_platform" = "xpoll")


dnl check for myqtt client tool dependencies
AC_ARG_ENABLE(myqtt-client, [  --enable-myqtt-client    Enable myqtt tool building [default=yes]], enable_myqtt_client="$enableval", enable_myqtt_client=yes)
AM_CONDITIONAL(ENABLE_MYQTT_CLIENT, test "x$enable_myqtt_client" = "xyes")

dnl check for doxygen documentation
AC_ARG_ENABLE(myqtt-doc, [  --enable-myqtt-doc       Enable building MyQtt documentation (doxygen required) [default=yes]], 
	      enable_myqtt_doc="$enableval", 
	      enable_myqtt_doc=yes)
if test x$enable_myqtt_doc = xyes ; then
        AC_CHECK_PROG(DOXYGEN, doxygen, "yes", "no")
fi
AM_CONDITIONAL(ENABLE_AF_ARCH_DOC, test "x$DOXYGEN" = "xyes")

dnl LibAxl library support.
PKG_CHECK_MODULES(AXL, axl >= 0.6.4)
AC_SUBST(AXL_CFLAGS)
AC_SUBST(AXL_LIBS)

dnl check for py-myqtt support
AC_ARG_ENABLE(py-myqtt,   [   --disable-py-myqtt       Makes MyQtt Library to be build without PyMyQtt: python bindings support],
              enable_py_myqtt_support="$enableval",
	      enable_py_myqtt_support=yes)
if test "$enable_py_myqtt_support" = "yes" ; then
   AM_PATH_PYTHON([2.3], enable_py_myqtt_support="yes", enable_py_myqtt_support="no")

   # find python-config
   PYTHON_CONFIG=`./get-python-config`

   # get site_packages
   python_dir_check=`eval ./check-python-dir.py ${pythondir}`

   # get python includes 
   PYTHON_INCLUDES=`$PYTHON_CONFIG --includes 2>/dev/null`
   old_CPPFLAGS=$CPPFLAGS
   CPPFLAGS="$PYTHON_INCLUDES"
   echo "Python includes: $PYTHON_INCLUDES"
   AC_SUBST(PYTHON_INCLUDES)

   AC_TRY_COMPILE([#include <Python.h>],
          [Py_InitModule3(NULL, NULL, NULL);],
          enable_py_myqtt_support="yes",
          enable_py_myqtt_support="no")
   AC_MSG_RESULT([Python development headers status: $enable_py_myqtt_support])

   if test "$enable_py_myqtt_support" = "yes" ; then

      # get python flags and libs
      PYTHON_CFLAGS=`$PYTHON_CONFIG --cflags 2>/dev/null | sed -r 's/\-arch (\d|\w)+ //g'`
      PYTHON_LIBS=`$PYTHON_CONFIG --ldflags 2>/dev/null`
      AC_SUBST(PYTHON_CFLAGS)
      AC_SUBST(PYTHON_LIBS)

      dnl get site dinamic library extension
      SHARED_EXTENSION=$shrext_cmds
      AC_SUBST(SHARED_EXTENSION)

      # check python_install_dir and pythondir maches
      python_dir_temp=`eval echo ${pythondir}`
      # check python_install_dir and pythondir maches
      if test "$python_dir_check" != "ok" ; then
          AC_MSG_ERROR([ERROR: pythondir variable is poiting to an unusable location: $python_dir_temp. Error detected: $python_dir_check. Fix your autoconf install or disable python building, for that, pass --disable-py-myqtt to configure. This usually indicates a problem with automake package which provides the _AM_PATH_PYTHON_ macro.])
      fi
      echo "Python install dir OK: $python_dir_temp"
   fi
fi
AM_CONDITIONAL(ENABLE_PY_MYQTT_SUPPORT, test "x$enable_py_myqtt_support" = "xyes")

dnl check for console log
AC_ARG_ENABLE(py-myqtt-log, [  --enable-py-myqtt-log       Enable building PyMyQtt console debug log support [default=yes]], 
	      enable_py_myqtt_log="$enableval", 
	      enable_py_myqtt_log=yes)
AM_CONDITIONAL(ENABLE_PY_MYQTT_LOG, test "x$enable_py_myqtt_log" = "xyes")


dnl check for console log
AC_ARG_ENABLE(myqtt-log, [  --enable-myqtt-log       Enable building MyQtt console debug log support [default=yes]], 
	      enable_myqtt_log="$enableval", 
	      enable_myqtt_log=yes)
AM_CONDITIONAL(ENABLE_MYQTT_LOG, test "x$enable_myqtt_log" = "xyes")

dnl check for tls building
AC_ARG_ENABLE(tls-support, [  --disable-tls-support     Makes buidling MyQtt TLS support (OpenSSL required)], 
	      enable_tls_support="$enableval", 
	      enable_tls_support=yes)
if test x$enable_tls_support = xyes ; then
   dnl check header needed
   AC_CHECK_HEADER(openssl/ssl.h,,enable_tls_support=no)
   if test x$enable_tls_support = xno ; then
      AC_MSG_WARN([Cannot OpenSSL installation, disabling it. This will disable TLS support. ]) 
      enable_tls_support=no
   else
      dnl configure additional TLS flags
      TLS_LIBS="-lssl -lcrypto"
      AC_SUBST(TLS_LIBS)
   fi
fi
AM_CONDITIONAL(ENABLE_TLS_SUPPORT, test "x$enable_tls_support" = "xyes")

dnl check for websocket support (through noPoll)
AC_ARG_ENABLE(websocket-support, [  --disable-websocket-support    Makes MyQtt to be built with WebSocket extension library], 
	      enable_websocket_support="$enableval", 
	      enable_websocket_support=yes)
if test "$enable_websocket_support" != "no" ; then
    PKG_CHECK_MODULES(NOPOLL, nopoll,	[enable_websocket_support=yes], [enable_websocket_support=no])
    AC_SUBST(NOPOLL_CFLAGS)
    AC_SUBST(NOPOLL_LIBS)
fi
AM_CONDITIONAL(ENABLE_WEBSOCKET_SUPPORT, test "x$enable_websocket_support" = "xyes")


dnl general libries subsitution
dnl AC_SUBST(LIBRARIES_CFLAGS)
dnl AC_SUBST(LIBRARIES_LIBS)

dnl get current platform and add especific flags
case $host in
  *-*-beos*)
    echo "Found BEOS platform: $host.."
    ;;
  *-*-cygwin*)
    echo "Found cygwin platform: $host.."
    ;;
  *-*-mingw*)
    echo "Found mingw platform: $host.."
    ADDITIONAL_LIBS=-lws2_32
    ;;
  *)
    echo "Found platform: $host.."
    ;;
esac
AC_SUBST(ADDITIONAL_LIBS)

# check additional flags for exarg
AC_TRY_LINK([#define _GNU_SOURCE
#include <stdio.h>], 
[
  char * result;
  return vasprintf (&result, "This is a test: %d", NULL);
], [have_vasprintf=yes],[have_vasprintf=no])
echo "Checking vasprintf support: $have_vasprintf"

dnl support for vasprintf
case $have_vasprintf in
yes)
	EXARG_FLAGS="-DHAVE_VASPRINTF $EXARG_FLAGS"
	echo "vasprintf support found"
     ;;
esac

dnl check for windows platform
case $host in
*-*-mingw*)
	EXARG_FLAGS="-DOS_WIN32 $EXARG_FLAGS"
	echo "windows platform found"
	;;	
*)
	echo "platform found: $host"
    ;;
esac
AC_SUBST(EXARG_FLAGS)	

AC_OUTPUT([
Makefile
lib/Makefile
tls/Makefile
web-socket/Makefile
server/Makefile
server/modules/Makefile
server/modules/mod-auth-xml/Makefile
server/modules/mod-auth-xml/mod-auth-xml.xml
server/modules/mod-ssl/Makefile
server/modules/mod-ssl/mod-ssl.xml
server/modules/mod-web-socket/Makefile
server/modules/mod-web-socket/mod-web-socket.xml
server/modules/mod-status/Makefile
server/modules/mod-status/mod-status.xml
server/modules/mod-test/Makefile
server/modules/mod-test/mod-test.xml
client/Makefile
py-myqtt/Makefile
py-myqtt/test/Makefile
py-myqtt/doc/Makefile
py-myqtt/doc/conf.py
test/Makefile
doc/Makefile
doc/myqtt.doxygen
myqtt-1.0.pc
myqtt-tls-1.0.pc
myqtt-web-socket-1.0.pc
])

echo "------------------------------------------"
echo "--     MYQTT LIBRARY 1.0 SETTINGS       --"
echo "------------------------------------------"
echo "   Installation prefix:            [$prefix]"
echo "      select(2) support:           [yes]"
echo "      poll(2) support:             [$enable_poll]"
echo "      epoll(2) support:            [$enable_cv_epoll]"
echo "      default:                     [$default_platform]"
echo "      debug log support:           [$enable_myqtt_log]"
echo "      pthread cflags=$PTHREAD_CFLAGS, libs=$PTHREAD_LIBS"
echo "      additional libs=$ADDITIONAL_LIBS"
if test x$enable_myqtt_log = xyes ; then
echo "      NOTE: To disable log reporting use: "
echo "            --disable-myqtt-log"               
fi
echo
echo "   Axl installation: "
echo "      cflags: $AXL_CFLAGS"
echo "      libs:   $AXL_LIBS"
echo

echo "   Build libMyQtt:                    enabled"
echo "   Build MyQttD server:               enabled"

if test x$enable_myqtt_client = xyes ; then
   echo "   Build myqtt-client tool:           enabled"
else
   echo "   Build myqtt-client tool:           disabled"
   echo 
   echo "     NOTE: To enable myqtt-client tool use "
   echo "           --enable-myqtt-client option."
fi
if test x$enable_tls_support = xyes ; then
   echo "   Build TLS support:                 enabled"
   echo "     (libmyqtt-tls-1.0)"
else
   echo "   Build TLS support:                 disabled"
   echo 
   echo "     NOTE: To enable TLS support you have to"
   echo "     install OpenSSL developement headers. "
   echo "     Once installed run again configure process "
   echo "     using --enable-tls-support option."
fi
if test x$enable_websocket_support = xyes ; then
   echo "   Build WebSocket extension library: enabled"
   echo "     (libmyqtt-websocket-1.0)"
else
   echo "   Build WebSocket extension library: disabled"
   echo 
   echo "     NOTE: To enable WebSocket support you need to have libnopoll"
   echo "           installed on your system and use --enable-websocket-support option."
fi
echo
if test x$enable_py_myqtt_support = xyes ; then
   echo "   Build PyMyQtt support:                   enabled"
   echo "     python install dir: ${python_install_dir}"
   echo "     python dir: ${pythondir}"
   echo "     python cflags:      ${PYTHON_CFLAGS}"
   echo "     python libs:        ${PYTHON_LIBS}"
else
   echo "   Build PyMyQtt support:                   disabled"
   echo 
   echo "     NOTE: To enable PyMyQtt support you have to install python development headers and "
   echo "           use --enable-py-myqtt option."
fi
if test x$DOXYGEN = xyes ; then
   echo "   Build MyQtt doc:                   enabled"
else	    
   echo "   Build MyQtt doc:                   disabled"
   echo  
   echo "     NOTE: To enable MyQtt documentation you will"
   echo "     have to install doxygen tool. Then run again"
   echo "     configure process."
fi

echo
echo "------------------------------------------"
echo "--            Let's MQTT!               --"
echo "--                                      --"
echo "--     NOW TYPE: make; make install     --"
echo "------------------------------------------"
