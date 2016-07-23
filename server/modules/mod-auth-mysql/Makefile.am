EXTRA_DIST = mod-auth-mysql.xml  

INCLUDES = -Wall -g -ansi -I../.. -I../../../lib/ -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(MYQTTD_VERSION)\" \
	   $(AXL_CFLAGS) $(MYQTT_CFLAGS) $(EXARG_CFLAGS) $(MYSQL_CFLAGS)

lib_LTLIBRARIES      = mod-auth-mysql.la
mod_auth_mysql_la_SOURCES  = mod-auth-mysql.c 
mod_auth_mysql_la_LDFLAGS  = -module -ldl $(MYSQL_LIBS)

# reconfigure module installation directory
libdir = $(prefix)/lib/myqtt/modules

# configure site module installation
modconfdir   = $(sysconfdir)/myqtt/mods-available
modconf_DATA = mod-auth-mysql.xml
