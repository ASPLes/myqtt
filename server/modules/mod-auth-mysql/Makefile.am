if ENABLE_TLS_SUPPORT
INCLUDE_TLS_FLAGS=-DENABLE_TLS_SUPPORT $(TLS_CFLAGS) -I$(top_srcdir)/tls
INCLUDE_TLS_LIBS=$(TLS_LIBS) $(top_builddir)/tls/libmyqtt-tls-1.0.la 
endif

if ENABLE_WEBSOCKET_SUPPORT
INCLUDE_WEBSOCKET_FLAGS=-DENABLE_WEBSOCKET_SUPPORT $(NOPOLL_CFLAGS) -I$(top_srcdir)/web-socket
INCLUDE_WEBSOCKET_LIBS=$(NOPOLL_LIBS) $(top_builddir)/web-socket/libmyqtt-web-socket-1.0.la 
endif

EXTRA_DIST = mod-auth-mysql.xml  

INCLUDES = -Wall -g -ansi -I../.. -I../../../lib/ -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(MYQTTD_VERSION)\" \
	   $(AXL_CFLAGS) $(MYQTT_CFLAGS) $(EXARG_CFLAGS) $(MYSQL_CFLAGS) $(INCLUDE_TLS_FLAGS) $(INCLUDE_WEBSOCKET_FLAGS)

lib_LTLIBRARIES      = mod-auth-mysql.la
mod_auth_mysql_la_SOURCES  = mod-auth-mysql.c 
mod_auth_mysql_la_LDFLAGS  = -module -ldl $(MYSQL_LIBS) $(INCLUDE_TLS_LIBS) $(INCLUDE_WEBSOCKET_LIBS)

# reconfigure module installation directory
libdir = $(prefix)/lib/myqtt/modules

# configure site module installation
modconfdir   = $(sysconfdir)/myqtt/mods-available
modconf_DATA = mod-auth-mysql.xml

# configure site module installation
configdir   = $(sysconfdir)/myqtt/mysql
config_DATA = mysql.example.xml
