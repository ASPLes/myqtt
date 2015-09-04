PyMyQtt's documentation center!
==============================

PyMyQtt is a python binding for libMyQtt, maintained
and supported by ASPL, that includes full support to write
MQTT client/server  applications entirely written using python.

Because quality matters, as with MyQtt stack, PyMyQtt development is
being driven and checked with a regression test suite to ensure each
realease is ready for production environment.

PyMyQtt execution model for async notifications is really similar to
libMyQtt because the binding makes use of the GIL feature described
in: http://www.python.org/dev/peps/pep-0311/ This means PyMyQtt
library execution will still use threads but only one thread at time
will be executing inside the context of Python.

Because PyMyQtt is a binding, libMyQtt documention is still useful
while using PyMyQtt: http://www.aspl.es/myqtt/doc/html/index.html

**Manuals and additional documentation available:**

.. toctree::
   :maxdepth: 1

   license
   manual
   MyQtt stack documentation center <http://www.aspl.es/myqtt/doc/html/index.html>

**API documentation:**

.. toctree::
   :maxdepth: 1

   myqtt
   ctx
   connection
   msg
   asyncqueue
   handlers
   myqtttls

=================
Community support
=================

Community assisted support is provided through MyQtt mailing list located at: http://lists.aspl.es/cgi-bin/mailman/listinfo/myqtt.

============================
Professional ensured support
============================

ASPL provides professional support for PyMyQtt inside MyQtt
Tech Support program. See the following for more information:
http://www.aspl.es/myqtt/professional.html


