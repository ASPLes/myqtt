EXTRA_DIST = mod-ssl.xml  ssl.example.conf

INCLUDES = -Wall -g -ansi -I../.. -I../../../lib/ -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(MYQTTD_VERSION)\" $(TLS_CFLAGS) -I$(top_builddir)/tls \
	   $(AXL_CFLAGS) $(MYQTT_CFLAGS) $(EXARG_CFLAGS)

lib_LTLIBRARIES      = mod-ssl.la
mod_ssl_la_SOURCES  = mod-ssl.c
mod_ssl_la_LDFLAGS  = -module -ldl $(TLS_LIBS) $(top_builddir)/tls/libmyqtt-tls-1.0.la 

# reconfigure module installation directory
libdir = $(prefix)/lib/myqtt/modules

etcdir = $(sysconfdir)/myqtt/ssl
etc_DATA = ssl.example.conf

# configure site module installation
modconfdir   = $(sysconfdir)/myqtt/mods-available
modconf_DATA = mod-ssl.xml
