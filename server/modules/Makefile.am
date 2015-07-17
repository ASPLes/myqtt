if ENABLE_TLS_SUPPORT
TLS_SUPPORT_DIR = mod-ssl
endif

if ENABLE_WEBSOCKET_SUPPORT
WEBSOCKET_SUPPORT_DIR = mod-web-socket
endif

SUBDIRS = mod-auth-xml $(TLS_SUPPORT_DIR) $(WEBSOCKET_SUPPORT_DIR) mod-status mod-test

install-exec-hook:
	mkdir -p $(sysconfdir)/myqtt/mods-enabled

