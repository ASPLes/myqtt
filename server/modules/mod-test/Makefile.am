EXTRA_DIST = mod-test.xml  

INCLUDES = -Wall -g -ansi -I../.. -I../../../lib/ -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(MYQTTD_VERSION)\" \
	   $(AXL_CFLAGS) $(MYQTT_CFLAGS) $(EXARG_CFLAGS)

lib_LTLIBRARIES      = mod-test.la
mod_test_la_SOURCES  = mod-test.c
mod_test_la_LDFLAGS  = -module -ldl

# reconfigure module installation directory
libdir = $(prefix)/lib/myqtt/modules

# configure site module installation
modconfdir   = $(sysconfdir)/myqtt/mods-available
modconf_DATA = mod-test.xml
