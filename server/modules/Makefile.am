if ENABLE_TLS_SUPPORT
TLS_SUPPORT_DIR = mod-ssl
endif

SUBDIRS = mod-auth-xml $(TLS_SUPPORT_DIR)

install-exec-hook:
	mkdir -p $(sysconfdir)/myqtt/mods-enabled

