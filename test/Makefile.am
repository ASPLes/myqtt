if ENABLE_POLL_SUPPORT
INCLUDE_MYQTT_POLL=-DMYQTT_HAVE_POLL=1
endif

if ENABLE_EPOLL_SUPPORT
INCLUDE_MYQTT_EPOLL=-DMYQTT_HAVE_EPOLL=1
endif

if ENABLE_MYQTT_LOG
INCLUDE_MYQTT_LOG=-DENABLE_MYQTT_LOG
endif

if ENABLE_TLS_SUPPORT
INCLUDE_TLS_SUPPORT = -DENABLE_TLS_SUPPORT -I$(top_srcdir)/tls
TLS_SUPPORT_LIBS    = $(top_builddir)/tls/libmyqtt-tls-1.1.la 
endif

if ENABLE_WEBSOCKET_SUPPORT
INCLUDE_WEBSOCKET_SUPPORT = -DENABLE_WEBSOCKET_SUPPORT -I$(top_srcdir)/web-socket
WEBSOCKET_SUPPORT_LIBS = $(top_builddir)/web-socket/libmyqtt-websocket-1.1.la
endif

noinst_PROGRAMS = test_01

INCLUDES = -I$(top_srcdir)/lib $(AXL_CFLAGS)  $(PTHREAD_CFLAGS) \
	-I$(READLINE_PATH)/include $(compiler_options) -D__axl_disable_broken_bool_def__   \
        -DVERSION=\""$(MYQTT_VERSION)"\" -I$(top_srcdir)/lib $(INCLUDE_MYQTT_POLL) $(INCLUDE_MYQTT_EPOLL) $(INCLUDE_MYQTT_LOG) 

LIBS            = $(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS) $(NOPOLL_LIBS)

test_01_SOURCES        = test_01.c
test_01_LDADD          = $(LIBS) $(top_builddir)/lib/libmyqtt-1.0.la 



