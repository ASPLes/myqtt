if ENABLE_TLS_SUPPORT
INCLUDE_TLS_SUPPORT = -DENABLE_TLS_SUPPORT -I$(top_srcdir)/tls
TLS_SUPPORT_LIBS    = $(top_builddir)/tls/libmyqtt-tls-1.0.la 
endif

EXTRA_DIST = mod-auth-xml.xml  

INCLUDES = -Wall -g -ansi -I../.. -I../../../lib/ $(INCLUDE_TLS_SUPPORT) -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(MYQTTD_VERSION)\" \
	   $(AXL_CFLAGS) $(MYQTT_CFLAGS) $(EXARG_CFLAGS)

lib_LTLIBRARIES      = mod-auth-xml.la
mod_auth_xml_la_SOURCES  = mod-auth-xml.c
mod_auth_xml_la_LDFLAGS  = -module -ldl $(TLS_SUPPORT_LIBS)

# reconfigure module installation directory
libdir = $(prefix)/lib/myqtt/modules

# configure site module installation
modconfdir   = $(sysconfdir)/myqtt/mods-available
modconf_DATA = mod-auth-xml.xml
