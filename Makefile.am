if ENABLE_READLINE 
MYQTT_TEST_DIR = 
endif


SUBDIRS = lib 
EXTRA_DIST = myqtt-1.0.pc.in VERSION

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = myqtt-1.0.pc

all-local: update-defs

update-defs: 
	cd lib; make update-def; cd ..

