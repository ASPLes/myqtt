EXTRA_DIST=__init__.py tls.py

if ENABLE_PY_MYQTT_LOG
INCLUDE_PY_MYQTT_LOG=-DENABLE_PY_MYQTT_LOG
endif

# if ENABLE_TLS_SUPPORT
# tls_lib=libpy_myqtt_tls_11.la
# tls_includes=-I$(top_srcdir)/tls
# endif

INCLUDES = $(compiler_options) -DCOMPILATION_DATE=`date +%s` -D__COMPILING_MYQTTD__   \
	   -DMYQTT_VERSION=\"$(MYQTT_VERSION)\" -DVERSION=\"$(MYQTT_VERSION)\" -I$(top_srcdir)/lib \
	   -DSYSCONFDIR=\""$(sysconfdir)"\" -DDEFINE_CHROOT_PROTO -DDEFINE_KILL_PROTO -DDEFINE_MKSTEMP_PROTO \
	   $(AXL_CFLAGS)  -g -Wall -Werror -Wstrict-prototypes $(PYTHON_CFLAGS) $(INCLUDE_PY_MYQTT_LOG)

lib_LTLIBRARIES    = libpy_myqtt_11.la $(tls_lib) 

# libpy_myqtt_11.la
libpy_myqtt_11_la_include_HEADERS  = \
	py_myqtt.h \
	py_myqtt_handle.h \
	py_myqtt_ctx.h \
	py_myqtt_conn.h  \
	py_myqtt_async_queue.h \
	py_myqtt_msg.h

libpy_myqtt_11_la_SOURCES  = \
	py_myqtt.c \
	py_myqtt_handle.c \
	py_myqtt_ctx.c \
	py_myqtt_conn.c \
	py_myqtt_async_queue.c \
	py_myqtt_msg.c

libpy_myqtt_11_la_LDFLAGS  = $(PYTHON_LIBS) $(top_srcdir)/lib/libmyqtt-1.0.la $(AXL_LIBS)
libpy_myqtt_11_la_includedir = $(includedir)/py_myqtt

libpy_myqtt_11_la_LIBADD = $(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS)

# if ENABLE_TLS_SUPPORT
# libpy_myqtt_tls_11.la
# libpy_myqtt_tls_11_la_include_HEADERS = \
# 	py_myqtt_tls.h
# libpy_myqtt_tls_11_la_SOURCES = \
# 	py_myqtt_tls.c
# libpy_myqtt_tls_11_la_LDFLAGS  = $(PYTHON_LIBS) $(top_srcdir)/src/libmyqtt-1.1.la $(top_srcdir)/tls/libmyqtt-tls-1.1.la $(AXL_LIBS) libpy_myqtt_11.la
# libpy_myqtt_tls_11_la_includedir = $(includedir)/py_myqtt
# 
# libpy_myqtt_tls_11_la_LIBADD = $(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS)
# endif

libdir = ${pythondir}/myqtt

# post-build script
all-local: 
	@echo "Running postbuild tasks.."

clean-local:
	@rm -f myqtt$(SHARED_EXTENSION) _sasl$(SHARED_EXTENSION) _tls$(SHARED_EXTENSION)

install-exec-hook:
	@echo "site installation ${DESTDIR}${pythondir}/myqtt.."
	install -d ${DESTDIR}${pythondir}/myqtt
	install -m 644 \
		tls.py \
		__init__.py \
		${DESTDIR}${pythondir}/myqtt
