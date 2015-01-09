if ENABLE_TLS_SUPPORT
TLS_SUPPORT_DIR = tls
TLS_PKGCONFIG = myqtt-tls-1.0.pc
TLS_DEFS = update-tls-defs
endif

if ENABLE_WEBSOCKET_SUPPORT
WEBSOCKET_SUPPORT_DIR = web-socket
WEBSOCKET_PKGCONFIG = myqtt-web-socket-1.0.pc
WEBSOCKET_DEFS = update-web-socket-defs
endif


SUBDIRS = lib $(TLS_SUPPORT_DIR) $(WEBSOCKET_SUPPORT_DIR) test
EXTRA_DIST = myqtt-1.0.pc.in VERSION

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = myqtt-1.0.pc

all-local: update-defs

update-defs: $(WEBSOCKET_DEFS) $(TLS_DEFS)
	cd lib; make update-def; cd ..

update-web-socket-defs:
	cd web-socket; make update-def; cd ..

update-tls-defs:
	cd tls; make update-def; cd ..

