tmp_xml_examples = \
	     domain-settings-basic.xml-tmp \
	     domain-settings-standard.xml-tmp \
	     myqtt-domain.xml-tmp \
	     running-user.xml-tmp \
	     port-configuration.xml-tmp

xml_examples = \
	     domain-settings-basic.xml \
	     domain-settings-standard.xml \
	     myqtt-domain.xml \
	     running-user.xml \
	     port-configuration.xml

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

all: build_doc

.xml.xml-tmp:
	axl-knife -i $< -o $@ -e -p fragment