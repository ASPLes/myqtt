tmp_xml_examples = \
	     domain-settings-basic.xml-tmp \
	     domain-settings-standard.xml-tmp \
	     myqtt-domain.xml-tmp \
	     running-user.xml-tmp \
	     port-configuration.xml-tmp \
	     on-bad-signal.xml-tmp \
	     notify-failures.xml-tmp \
	     log-reporting.xml-tmp \
	     override-system-paths.xml-tmp \
	     log-reporting.syslog.xml-tmp \
	     include-from-file.xml-tmp \
	     include-from-dir.xml-tmp \
	     myqttd-modules.xml-tmp \
	     module-conf.xml-tmp \
	     myqtt-domain.example.xml-tmp \
	     myqtt.yourdomain.com.xml-tmp

xml_examples = \
	     domain-settings-basic.xml \
	     domain-settings-standard.xml \
	     myqtt-domain.xml \
	     running-user.xml \
	     port-configuration.xml \
	     on-bad-signal.xml \
	     notify-failures.xml \
	     log-reporting.xml \
	     override-system-paths.xml \
	     log-reporting.xml \
	     include-from-file.xml \
	     include-from-dir.xml \
	     myqttd-modules.xml \
	     module-conf.xml \
	     myqtt-domain.example.xml \
	     myqtt.yourdomain.com.xml

EXTRA_DIST = $(tmp_xml_examples) $(xml_examples) myqtt.doxygen \
	     myqtt.doxygen.in \
	     myqtt-footer.html \
	     myqtt-header.html \
	     main-page-logo.png \
	     myqtt.css \
	     body_back.gif \
	     aspl-logo-header.png \
	     hacha-100x171.png \
	     myqtt-init.d \
             myqtt-rpm-init.d 


bin_SCRIPTS = initial_build_doc

help:
	@echo 
	@echo " Documentation not build!:"
	@echo " ~~~~~~~~~~~~~~~~~~~~~~~~~"
	@echo " --> Doxygen documentation wasn't built because an stamp was detected: doc.stamp"
	@echo " --> To regenerate or force doc building type make build_doc or remove doc.stamp";
	@echo

initial_build_doc: 
	@echo "checking documenation stamp..";
	@rm -f *.xml-tmp
	@test -e doc.stamp & make help
	@test -e doc.stamp || make build_doc
	@test -e doc.stamp || touch doc.stamp

build_doc: prepare_xmls
	rm -rf html/
	@echo "Generating module documentation..";
	doxygen myqtt.doxygen
	cp $(top_srcdir)/doc/main-page-logo.png html
	cp $(top_srcdir)/doc/aspl-logo-header.png html
	cp $(top_srcdir)/doc/myqtt.css html
	cp $(top_srcdir)/doc/body_back.gif html
	cp $(top_srcdir)/doc/hacha-100x171.png html

prepare_xmls: $(tmp_xml_examples)
	axl-knife -i ../server/myqtt.example.conf -o myqtt.example.conf.xml -r -e -p fragment
	axl-knife -i ../server/modules/mod-auth-xml/users.example.xml -o users.example.xml-tmp  -e -p fragment
	axl-knife -i ../server/modules/mod-ssl/ssl.example.conf -o ssl.example.conf.tmp  -e -p fragment

all: build_doc

.xml.xml-tmp:
	axl-knife -i $< -o $@ -e -p fragment
