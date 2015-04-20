EXTRA_DIST = mod-web-socket.xml  

INCLUDES = -Wall -g -ansi -I../.. -I../../../lib/ -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(MYQTTD_VERSION)\" $(NOPOLL_CFLAGS) -I$(top_builddir)/web-socket \
	   $(AXL_CFLAGS) $(MYQTT_CFLAGS) $(EXARG_CFLAGS)

lib_LTLIBRARIES      = mod-web-socket.la
mod_web_socket_la_SOURCES  = mod-web-socket.c
mod_web_socket_la_LDFLAGS  = -module -ldl $(NOPOLL_LIBS) $(top_builddir)/web-socket/libmyqtt-web-socket-1.0.la 

# reconfigure module installation directory
libdir = $(prefix)/lib/myqtt/modules

etcdir = $(sysconfdir)/myqtt/web-socket
etc_DATA = web-socket.example.conf

# configure site module installation
modconfdir   = $(sysconfdir)/myqtt/mods-available
modconf_DATA = mod-web-socket.xml
