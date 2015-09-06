:mod:`myqtt.tls` --- TLS support for PyMyQtt
============================================

.. currentmodule:: myqtt.tls


MyQtt TLS support is provided by an independent python module that is
imported like this::

  import myqtt.tls

After that, no specific initialization code is required.


==========
Module API
==========

.. method:: create_listener (ctx, bindaddress, port, [conn_opts])

   Allows to create a TLS listener on the provided bindaddress:port.

   :param ctx: The context where to create the listener
   :type ctx: myqtt.Ctx

   :param bindaddress: The bindaddress to be used
   :type bindaddress: String

   :param port: port to run the listener
   :type port: String

   :param conn_opts: Optional connection options to configure
   :type conn_opts: myqtt.ConnOpts

.. method:: create_conn (ctx, bindaddress, port, [conn_opts])

   Allows to create a MQTT TLS connection to the destination
   configured. This function works the same as the myqtt.Conn method
   but providing support for SSL/TLS

   :param ctx: MyQtt context where the connection will be created.
   :type ctx: myqtt.Ctx

   :param host: Host to connect to.
   :type host: String

   :param port: Port to connect to.
   :type port: String

   :param client_identifier: Client identifier
   :type client_identifier: String

   :param clean_session: Clean session
   :type clean_session: Boolean

   :param keep_alive: Keep alive configuration
   :type keep_alive: Integer

   :param conn_opts: Optional connection options to be used by the connection 
   :type conn_opts: String


.. method:: set_certificate (listener, certificate_file,  certificate_key, [certificate_chain])

   Allows to configure the certificate (crt, key and chain) to be used
   by the provided listener.

   :param listener: The listener to be configured with the provided certificate
   :type listener: myqtt.Conn

   :param certificate_file: Path to the certificate file to use
   :type certificate_file: String

   :param certificate_key: Path to the certificate key to use
   :type certificate_key: String

   :param certificate_chain: Path to the certificate chain 
   :type certificate_chain: String


.. method:: is_on (connection)

   Allows to check if the provided connection is running with TLS/SSL activated

   :param connection: The connection to be checked
   :type connection: myqtt.Conn


.. method:: ssl_peer_verify (opts, disable)

   Allows to configure certificate verification on the provided
   myqtt.ConnOpts that will be used on the listener or connection
   creation.

   :param opts: The connection options to be configured
   :type opts: myqtt.ConnOpts

   :param disable: True/False to enable disable SSL peer verification
   :type disable: Boolean

.. method:: set_ssl_certs (opts, certificate, private_key, chain_certificate, ca_certificate)

   Allows to configure certificate on the provided connection options.

   :param opts: The connection option to be configured
   :type opts: myqtt.ConnOpts

   :param certificate: Path to the certificate file to use
   :type certificate: String

   :param private_key: Path to the certificate key to use
   :type private_key: String

   :param chain_certificate: Path to the certificate chain 
   :type chain_certificate: String

   :param ca_certificate: Path to the CA certificate
   :type cat_certificate: String

.. method:: set_server_name (opts, server_name)

   Allows to configure the server name that is going to be used during
   the SSL/TLS negotiation. This is used, for example, during SNI
   client process to be able to notify server side which serverName we
   are interested in so the right certificate can be selected.

   :param opts: The connection option to be configured
   :type opts: myqtt.ConnOpts

   :param server_name: The server name to configure
   :type server_name: String

.. method:: set_certificate_handlers (ctx, certificate_handler, key_handler, chain_handler, user_data)

   Allows to configure a set of handlers that will dynamically
   determine the right set of certificates according to the serverName
   requested.

   :param ctx: The context to configure
   :type ctx: myqtt.Ctx

   :param certificate_handler: The handler that is called to get the certificate according to the server Name provided
   :type certificate: Handler: handler(ctx, conn, serverName, userData) -> path certificate (String)

   :param key_handler: The handler that is called to get the certificate according to the server Name provided
   :type key_handler: handler: handler(ctx, conn, serverName, userData) -> path certificate (String)

   :param chain_handler: The handler that is called to get the chain certificate according to the server Name provided
   :type chain_certificate: handler:  handler(ctx, conn, serverName, userData) -> path certificate (String)

.. method:: verify_cert (conn)

   Allows to verify certificate provided by remote side on the
   provided connection.

   :param conn: The connection to verify its certificate
   :type conn: myqtt.Conn





