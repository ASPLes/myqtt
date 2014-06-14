EXTRA_DIST = libmyqtt-1.0.def

if ENABLE_MYQTT_LOG
INCLUDE_MYQTT_LOG=-DENABLE_MYQTT_LOG
endif

if ENABLE_POLL_SUPPORT
INCLUDE_MYQTT_POLL=-DMYQTT_HAVE_POLL=1
endif

if ENABLE_EPOLL_SUPPORT
INCLUDE_MYQTT_EPOLL=-DMYQTT_HAVE_EPOLL=1
endif

if DEFAULT_EPOLL
INCLUDE_DEFAULT_EPOLL=-DDEFAULT_EPOLL 
endif

if DEFAULT_POLL
INCLUDE_DEFAULT_POLL=-DDEFAULT_POLL 
endif

INCLUDES = $(compiler_options) $(ansi_option) -I$(top_srcdir) -D__COMPILING_MYQTT__ -D__axl_disable_broken_bool_def__  \
	$(AXL_CFLAGS) $(INCLUDE_MYQTT_LOG) $(PTHREAD_CFLAGS) \
	-DVERSION=\""$(MYQTT_VERSION)"\" \
	-DPACKAGE_DTD_DIR=\""$(datadir)"\" \
	-DPACKAGE_TOP_DIR=\""$(top_srcdir)"\" $(INCLUDE_MYQTT_POLL) $(INCLUDE_MYQTT_EPOLL) $(INCLUDE_DEFAULT_EPOLL) $(INCLUDE_DEFAULT_POLL)

libmyqtt_1_0_includedir = $(includedir)/myqtt-1.0

lib_LTLIBRARIES = libmyqtt-1.0.la

libmyqtt_1_0_la_SOURCES =	\
	myqtt.c \
	myqtt_ctx.c \
	myqtt_conn.c \
	myqtt_reader.c 

libmyqtt_1_0_include_HEADERS = myqtt.h \
	myqtt_ctx.h \
	myqtt_ctx_private.h \
	myqtt_conn.h \
	myqtt_conn_private.h \
	myqtt_reader.h 

libmyqtt_1_0_la_LIBADD = \
	$(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS)

libmyqtt_1_0_la_LDFLAGS = -no-undefined -export-symbols-regex '^(myqtt|__myqtt|_myqtt).*'

libmyqtt-1.0.def: update-def

update-def:
	echo "EXPORTS" > libmyqtt-1.0.def
	cat .libs/libmyqtt-1.0.exp | grep -v io_waiting_poll | grep -v io_waiting_epoll | grep -v __myqtt >> libmyqtt-1.0.def
	echo "__myqtt_connection_set_not_connected" >> libmyqtt-1.0.def
	echo "gettimeofday" >> libmyqtt-1.0.def
