EXTRA_DIST = test-private.key test-certificate.crt message-test-17-d.txt

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
TLS_SUPPORT_LIBS    = $(top_builddir)/tls/libmyqtt-tls-1.0.la 
endif

if ENABLE_WEBSOCKET_SUPPORT
INCLUDE_WEBSOCKET_SUPPORT = -DENABLE_WEBSOCKET_SUPPORT -I$(top_srcdir)/web-socket $(NOPOLL_CFLAGS)
WEBSOCKET_SUPPORT_LIBS = $(top_builddir)/web-socket/libmyqtt-web-socket-1.0.la
endif

if ENABLE_MOSQUITTO
INCLUDE_MOSQUITTO = -DENABLE_MOSQUITTO
INCLUDE_MOSQUITTO_LIBS = -lmosquitto
endif

noinst_PROGRAMS = myqtt-regression-client myqtt-regression-listener

INCLUDES = -I$(top_srcdir)/lib $(AXL_CFLAGS)  $(PTHREAD_CFLAGS) -DENABLE_INTERNAL_TRACE_CODE \
	-I$(READLINE_PATH)/include $(compiler_options) -D__axl_disable_broken_bool_def__   \
        -DVERSION=\""$(MYQTT_VERSION)"\" -I$(top_srcdir)/lib $(INCLUDE_MYQTT_POLL) $(INCLUDE_MYQTT_EPOLL) $(INCLUDE_MYQTT_LOG) $(INCLUDE_TLS_SUPPORT) $(INCLUDE_WEBSOCKET_SUPPORT) $(INCLUDE_MOSQUITTO)

LIBS            = $(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS) $(NOPOLL_LIBS) $(TLS_SUPPORT_LIBS) $(WEBSOCKET_SUPPORT_LIBS) $(INCLUDE_MOSQUITTO_LIBS)

# client
myqtt_regression_client_SOURCES        = myqtt-regression-client.c
myqtt_regression_client_LDADD          = $(LIBS) $(top_builddir)/lib/libmyqtt-1.0.la 

# listener
myqtt_regression_listener_SOURCES        = myqtt-regression-listener.c
myqtt_regression_listener_LDADD          = $(LIBS) $(top_builddir)/lib/libmyqtt-1.0.la 



