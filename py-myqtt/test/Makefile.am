EXTRA_DIST = myqtt-regression-client.py myqtt-regression-listener.py 

all: prepare
	@echo "To run regression tests do the following steps:"
	@echo " - ./myqtt-regression-listener.py"
	@echo "   <in other terminal>"
	@echo " - ./myqtt-regression-client.py"

run-tests: prepare check

prepare:
	@cd ../../lib; make; cd -
	@test -d myqtt || mkdir myqtt
        # copy dynamic libraries

	find .. -name libpy_myqtt_10$(SHARED_EXTENSION) -type l -exec cp {}  myqtt/ \;
if ENABLE_TLS_SUPPORT
	find .. -name libpy_myqtt_tls_10$(SHARED_EXTENSION) -type l -exec cp {}  myqtt/ \;
endif

        # copy python imports
	@test -e myqtt/__init__.py || cp ../__init__.py myqtt/
	@diff myqtt/__init__.py ../__init__.py || cp ../__init__.py myqtt/

	@test -e myqtt/tls.py      || cp ../tls.py myqtt/
	@diff myqtt/tls.py ../tls.py || cp ../tls.py myqtt/

check:
	python myqtty-regression-client.py

