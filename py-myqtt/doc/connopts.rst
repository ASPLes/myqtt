:mod:`myqtt.ConnOpts` --- PyMyQttConnOpts class: MQTT connection options
========================================================================

.. currentmodule:: myqtt


API documentation for myqtt.ConnOpts which provides support for
various connection options like connection authentication, connection
reconnect, certificate verification, etc..

To create a connection option use the following example. Once created,
the connection option is defined to be used just for the connection it
was created. In the case you want to reuse that connection option for
several connection, then you'll have to enable reuse flag::

    # create a connection options
    opts = myqtt.ConnOpts ()
    opts.set_auth ("aspl", "wrong-password")

    # call to create a connection (passing options created as last parameter)
    # client_identifier = None
    # clean_session = True
    # keep_alive = 30
    conn = myqtt.Conn (ctx, host, port, None, True, 30, opts)


==========
Module API
==========

.. class:: ConnOpts ()

   .. method:: set_auth (username, password)

      Allows to configure the username and password to be used by the
      connection. 

      :param username: Username to configure 
      :type username: String

      :param password: Password to configure
      :type passwordctx: String

   .. method:: set_will (will_qos, will_topic, will_message, will_retain)

      Allows to configure will settings to be used by the connection.

      :param will_qos: Will QoS to configure
      :type will_qos: Number (myqtt.qos0, myqtt.qos1, myqtt.qos2)

      :param will_topic: Will topic to configure
      :type will_topic: String

      :param will_message: Will message to configure
      :type will_message: String

      :param will_retain: Retain flag for will message
      :type will_retain: Boolean


   .. method:: set_reconnect ([reconnect])

      Allows to configure automatic reconnection if connection is
      lost. That way same reference keeps on working and user land
      code do not have to implement complex detect-and-reconnect code
      if connection is lost. Optionall, you can also configure
      conn.set_on_reconnect (handler) to get a notification when that
      reconnect happens in the case additional tasks at the user land
      code must be carried out.

      :param reconnect: Optional boolean parameter to indicate to
			enable/disable automatic reconnect. If this
			parameter is not provided, automatic reconnect
			enable is assumed
      :type reconnect: Boolean


      

