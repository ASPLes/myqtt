EXTRA_DIST = myqttd-ctx-private.h

# profile a flag to enable the source to code to react agaisnt the
# pcre inclusion.
if ENABLE_PCRE_SUPPORT
INCLUDE_PCRE_SUPPORT=-DENABLE_PCRE_SUPPORT
endif

if ENABLE_TERMIOS
INCLUDE_TERMIOS=-DENABLE_TERMIOS
endif

INCLUDES = $(compiler_options) -DCOMPILATION_DATE=`date +%s` -D__COMPILING_MYQTTD__ -D_POSIX_C_SOURCE  \
	   -DVERSION=\"$(MYQTT_VERSION)\" -DAXL_VERSION=\"$(AXL_VERSION)\" -I$(top_srcdir)/lib \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" -DDEFINE_CHROOT_PROTO -DDEFINE_KILL_PROTO -DDEFINE_MKSTEMP_PROTO \
	   -DPIDFILE=\""$(statusdir)/myqttd.pid"\" \
	   -DMYQTTD_RUNTIME_DATADIR=\""$(runtimedatadir)"\" \
	   -DMYQTTD_DATADIR=\""$(datadir)"\" $(INCLUDE_PCRE_SUPPORT) $(PCRE_CFLAGS) $(INCLUDE_TERMIOS) $(EXARG_FLAGS) \
	   -D__MYQTTD_ENABLE_DEBUG_CODE__ \
	   $(AXL_CFLAGS)  -g -Wall -Werror -Wstrict-prototypes 

bin_PROGRAMS = myqttd


myqttd_SOURCES = \
	main-common.c main-common.h main.c

myqttd_LDFLAGS = -Wl,-export-dynamic -ldl

myqttd_LDADD = $(AXL_LIBS) libmyqttd.la

lib_LTLIBRARIES  = libmyqttd.la

libmyqttdinclude_HEADERS    = \
	exarg.h \
	myqttd-types.h \
	myqttd.h \
	myqttd-handers.h \
	myqttd-config.h \
	myqttd-support.h \
	myqttd-ctx.h \
	myqttd-expr.h \
	myqttd-support.h \
	myqttd-signal.h \
	myqttd-moddef.h \
	myqttd-run.h \
	myqttd-module.h \
	myqttd-log.h \
	myqttd-conn-mgr.h \
	myqttd-loop.h \
	myqttd-process.h \
	myqttd-child.h

libmyqttd_la_SOURCES = \
	exarg.c \
	myqttd-config.c \
	myqttd-support.c \
	myqttd-ctx.c \
	myqttd-expr.c \
	myqttd-support.c \
	myqttd-signal.c \
	myqttd-run.c \
	myqttd-module.c \
	myqttd-log.c \
	myqttd-conn-mgr.c \
	myqttd-loop.c \
	myqttd-process.c \
	myqttd-child.c

libmyqttd_la_LIBADD = $(AXL_LIBS)  $(PCRE_LIBS)
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
