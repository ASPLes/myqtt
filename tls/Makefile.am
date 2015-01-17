EXTRA_DIST = libmyqtt-tls-1.0.def

if ENABLE_MYQTT_LOG
INCLUDE_MYQTT_LOG=-DENABLE_MYQTT_LOG
endif

INCLUDES = -I. -I$(top_srcdir)/lib $(compiler_options) -I$(top_srcdir) -D__COMPILING_MYQTT__ -D__axl_disable_broken_bool_def__  \
	$(AXL_CFLAGS) $(INCLUDE_MYQTT_LOG) $(PTHREAD_CFLAGS) $(TLS_CFLAGS) \
	-DVERSION=\""$(MYQTT_VERSION)"\" \
	-DPACKAGE_DTD_DIR=\""$(datadir)"\" \
	-DPACKAGE_TOP_DIR=\""$(top_srcdir)"\" 

libmyqtt_tls_1_0_includedir = $(includedir)/myqtt

lib_LTLIBRARIES = libmyqtt-tls-1.0.la

libmyqtt_tls_1_0_la_SOURCES = myqtt-tls.c 
libmyqtt_tls_1_0_include_HEADERS = myqtt-tls.h

libmyqtt_tls_1_0_la_LIBADD = \
	$(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS) $(TLS_LIBS) $(top_builddir)/lib/libmyqtt-1.0.la 

libmyqtt_tls_1_0_la_LDFLAGS = -no-undefined -export-symbols-regex '^(myqtt|__myqtt|_myqtt).*' 

libmyqtt-tls-1.0.def: update-def

update-def:
	echo "EXPORTS" > libmyqtt-tls-1.0.def
	cat .libs/libmyqtt-tls-1.0.exp | grep -v io_waiting_poll | grep -v io_waiting_epoll | grep -v __myqtt >> libmyqtt-tls-1.1.def