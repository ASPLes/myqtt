EXTRA_DIST = mod-status.xml  status.example.conf

INCLUDES = -Wall -g -ansi -I../.. -I../../../lib/ -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(MYQTTD_VERSION)\" $(NOPOLL_CFLAGS) \
	   $(AXL_CFLAGS) $(MYQTT_CFLAGS) $(EXARG_CFLAGS)

lib_LTLIBRARIES      = mod-status.la
mod_status_la_SOURCES  = mod-status.c
mod_status_la_LDFLAGS  = -module -ldl $(NOPOLL_LIBS) 

# reconfigure module installation directory
libdir = $(prefix)/lib/myqtt/modules

etcdir = $(sysconfdir)/myqtt/status
etc_DATA = status.example.conf

# configure site module installation
modconfdir   = $(sysconfdir)/myqtt/mods-available
modconf_DATA = mod-status.xml
