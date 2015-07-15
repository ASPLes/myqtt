SUBDIRS = modules

EXTRA_DIST = myqttd-ctx-private.h \
	test_01.conf \
	test_02.conf \
	test_10.conf \
	test_12.conf \
	test_15.conf

etcdir = $(sysconfdir)/myqtt
etc_DATA = myqtt.example.conf

# profile a flag to enable the source to code to react agaisnt the
# pcre inclusion.
if ENABLE_PCRE_SUPPORT
INCLUDE_PCRE_SUPPORT=-DENABLE_PCRE_SUPPORT
endif

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
	   -DPIDFILE=\""$(statusdir)/myqttd.pid"\" \
	   -DMYQTTD_RUNTIME_DATADIR=\""$(runtimedatadir)"\" \
	   -DMYQTTD_DATADIR=\""$(datadir)"\" $(INCLUDE_PCRE_SUPPORT) $(PCRE_CFLAGS) $(INCLUDE_TERMIOS) $(EXARG_FLAGS) \
	   -D__MYQTTD_ENABLE_DEBUG_CODE__ \
	   $(AXL_CFLAGS)  -g -Wall -Werror -Wstrict-prototypes $(INCLUDE_TLS_FLAGS) $(INCLUDE_WEBSOCKET_FLAGS)

bin_PROGRAMS = myqttd 

noinst_PROGRAMS = test_01

# myqttd
myqttd_SOURCES = \
	main-common.c main-common.h main.c

myqttd_LDFLAGS = -Wl,-export-dynamic -ldl

myqttd_LDADD = $(AXL_LIBS) libmyqttd.la $(top_builddir)/lib/libmyqtt-1.0.la 

# test_01
test_01_SOURCES = test_01.c test_01.h

test_01_LDFLAGS = -Wl,-export-dynamic -ldl

test_01_LDADD = $(AXL_LIBS) $(INCLUDE_TLS_LIBS) $(INCLUDE_WEBSOCKET_LIBS) libmyqttd.la $(top_builddir)/lib/libmyqtt-1.0.la 

lib_LTLIBRARIES  = libmyqttd.la

libmyqttdinclude_HEADERS    = \
	exarg.h \
	myqttd-types.h \
	myqttd.h \
	myqttd-handlers.h \
	myqttd-config.h \
	myqttd-support.h \
	myqttd-ctx.h \
	myqttd-expr.h \
	myqttd-signal.h \
	myqttd-moddef.h \
	myqttd-run.h \
	myqttd-module.h \
	myqttd-log.h \
	myqttd-conn-mgr.h \
	myqttd-loop.h \
	myqttd-process.h \
	myqttd-child.h \
	myqttd-domain.h \
	myqttd-users.h

libmyqttd_la_SOURCES = \
	exarg.c \
	myqttd.c \
	myqttd-config.c \
	myqttd-support.c \
	myqttd-ctx.c \
	myqttd-expr.c \
	myqttd-signal.c \
	myqttd-run.c \
	myqttd-module.c \
	myqttd-log.c \
	myqttd-conn-mgr.c \
	myqttd-loop.c \
	myqttd-process.c \
	myqttd-child.c \
	myqttd-domain.c \
	myqttd-users.c

libmyqttd_la_LIBADD = $(AXL_LIBS)  $(PCRE_LIBS) $(INCLUDE_TLS_LIBS)
libmyqttd_la_LDFLAGS = -Wl,-export-dynamic -ldl -no-undefined -export-symbols-regex '^(myqttd|__myqttd|exarg).*'

libmyqttdincludedir = $(includedir)/myqttd

all-local: update-def

libmyqttd.def: update-def

update-def:
	echo "EXPORTS" > libmyqttd.def
	cat .libs/libmyqttd.exp | grep -v __myqttd >> libmyqttd.def

# copy exarg dependency to not force people to download a library with
# two files
exarg.h exarg.c: 
	test -e exarg.h || cp ../../libexarg/src/exarg.h .
	test -e exarg.c || cp ../../libexarg/src/exarg.c .

update-exarg:
	cp ../../libexarg/src/exarg.[ch] .

# create directory and set permissions
install-exec-hook:
	@echo "Creating shared dir ${DESTDIR}${runtimedatadir}/myqttd.."
	install -d -m 700 ${DESTDIR}${runtimedatadir}/myqttd

