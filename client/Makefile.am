EXTRA_DIST = 


if ENABLE_TLS_SUPPORT
INCLUDE_TLS_FLAGS=-DENABLE_TLS_SUPPORT $(TLS_CFLAGS) -I$(top_srcdir)/tls
INCLUDE_TLS_LIBS=$(TLS_LIBS) $(top_builddir)/tls/libmyqtt-tls-1.0.la 
endif

if ENABLE_WEBSOCKET_SUPPORT
INCLUDE_WEBSOCKET_FLAGS=-DENABLE_WEBSOCKET_SUPPORT $(NOPOLL_CFLAGS) -I$(top_srcdir)/web-socket
INCLUDE_WEBSOCKET_LIBS=$(NOPOLL_LIBS) $(top_builddir)/web-socket/libmyqtt-web-socket-1.0.la 
endif

INCLUDES = $(compiler_options) -DCOMPILATION_DATE=`date +%s` -D__COMPILING_MYQTTD__ -D_POSIX_C_SOURCE  \
	   -DMYQTT_VERSION=\"$(MYQTT_VERSION)\" -DVERSION=\"$(MYQTT_VERSION)\" -DAXL_VERSION=\"$(AXL_VERSION)\" -I$(top_srcdir)/lib \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" -DDEFINE_CHROOT_PROTO -DDEFINE_KILL_PROTO -DDEFINE_MKSTEMP_PROTO \
	   $(INCLUDE_TERMIOS) $(EXARG_FLAGS) \
	   $(AXL_CFLAGS)  -g -Wall -Werror -Wstrict-prototypes $(INCLUDE_TLS_FLAGS) $(INCLUDE_WEBSOCKET_FLAGS)

bin_PROGRAMS = myqtt-client 

# myqtt-client
myqtt_client_SOURCES = \
	main.c exarg.c exarg.h

myqtt_client_LDFLAGS = -Wl,-export-dynamic -ldl

myqtt_client_LDADD = $(AXL_LIBS) $(top_builddir)/lib/libmyqtt-1.0.la $(INCLUDE_TLS_LIBS) $(INCLUDE_WEBSOCKET_LIBS)

# copy exarg dependency to not force people to download a library with
# two files
exarg.h exarg.c: 
	test -e exarg.h || cp ../../libexarg/src/exarg.h .
	test -e exarg.c || cp ../../libexarg/src/exarg.c .

update-exarg:
	cp ../../libexarg/src/exarg.[ch] .



