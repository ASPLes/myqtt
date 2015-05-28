PyMyqtt manual
===============

===============================
Creating a PyMyQtt MQTT  client
===============================

1. First, we have to create a MyQtt context. This is done like
follows::

   # import base myqtt module
   import myqtt

   # import sys (sys.exit)
   import sys

   # create a myqtt.Ctx object 
   ctx = myqtt.Ctx ()

   # now, init the context and check status
   if not ctx.init ():
      print ("ERROR: Unable to init ctx, failed to continue")
      sys.exit (-1)

   # ctx created

2. Now, we have to establish a MQTT session with the remote
listener. This is done like follows::

   # create a MQTT session connected to localhost:1602
   conn = myqtt.Conn (ctx, "localhost", "1602")

   # check connection status
   if not conn.is_ok ():
      print ("ERROR: Failed to create connection with 'localhost':1602")	
      sys.exit (-1)

3. Once we have a MQTT session opened we can subscribe to a topic like this::

    # now subscribe
    (status, sub_qos) = conn.sub ("topic", myqtt.qos0, 10)
    if not status:
        error ("Failed to subscribe")
        return False

    info ("Subscription done, sub_qos is: %d" % sub_qos)

4. Now, to publish a message to a particular topic use::

   if not conn.pub ("topic", "this is a test message", myqtt.qos0, True, 10):
        error ("Failed to subscribe")
        return False

   info ("Publish op done")

5. Now, to receive replies and other requests, we have to configure a
frame received handler::

   def on_msg (ctx, conn, msg, data):
       # messages received asynchronously
       print ("Msg content: " + msg.payload)
       return
   
   conn.set_msg_received (on_msg)

.. note::

   Note also full regression test client, which includes all features tested, is located at: https://dolphin.aspl.es/svn/publico/myqtt/py-myqtt/test/myqtt-regression-client.py

========================
Creating a MQTT listener
========================

1. The process of creating a BEEP listener is pretty
straitforward. You have to follow the same initialization process as
the client. Then, you have to register profiles that will be supported
by your listener. This is done as follows::

   def default_frame_received (conn, channel, frame, data):

       print ("Frame content received: " + frame.payload)

       # reply in the case of MSG received
       if frame.type == 'MSG':
       	  # reply doing an echo
       	  channel.send_rpy (frame.payload, frame.payload_size, frame.msg_no)

       return
       # end default_frame_received 		   		   		       

   # register support for a profile
   vortex.register_profile (ctx, "urn:aspl.es:beep:profiles:my-profile",
   			    frame_received=default_frame_received)

2. After your listener signals its support for a particular profile,
it is required to create a listener instance::

   # start listener and check status
   listener = vortex.create_listener (ctx, "0.0.0.0", "1602")
   
   if not listener.is_ok ():
      print ("ERROR: failed to start listener, error was was: " + listener.error_msg)
      sys.exit (-1)

3. Because we have to wait for frames to be received we need a wait to
block the listener. The following is not strictly necessary it you
have another way to make the main thread to not finish::

   # wait for requests
   vortex.wait_listeners (ctx, unlock_on_signal=True)
   

.. note::

   Full listener source code can be found at: https://dolphin.aspl.es/svn/publico/af-arch/trunk/libvortex-1.1/py-vortex/test/simple-listener.py

.. note::

   Note also full regression test listener, which includes all features tested, is located at: https://dolphin.aspl.es/svn/publico/af-arch/trunk/libvortex-1.1/py-vortex/test/vortex-regression-listener.py

========================================
Enabling server side SASL authentication
========================================

To enable server side SASL authentication, we activate the set of
mechanisms that will be used to implement auth operations and a handler
(or a set of handlers) that will be called to complete auth
operation. Some handlers must return True/False to accept/deny the
auth operation. Other SASL mechanisms must return the password
associated to a user. See documentation associated to each mechanish.

In all cases, vortex.sasl it is at the end a binding on top of Vortex
Library SASL implementation. See also its documentation.

1. First, you have to include vortex.sasl 
component::

   import vortex
   import vortex.sasl

2. Then, you have to enable which SASL mechanism to be used to
authenticate remote peer. For example, we can use "plain" mechanism as
follows. It is possible to have several mechanism available at the
same time, allowing remote peer to choose one::

   # activate support for SASL plain mechanism
   vortex.sasl.accept_mech (ctx, "plain", auth_handler)

3. After that, each time a request to activate an incoming connection
is handle using auth_handler provided. An example handling SASL plain
mechanism is the following::

   def auth_handler (conn, auth_props, user_data):

       if auth_props["mech"] == vortex.sasl.PLAIN:
       	  # only authenticate users with user bob and password secret
       	  if auth_props["auth_id"] == "bob" and auth_props["password"] == "secret":
	      return True

       # fail to authentcate connection
       return False

Previous auth handler example it's authenticating
statically. Obviously that could be replaced with appropriate database
access check to implement dynamic SASL auth.

===================================
Enabling server side TLS encryption
===================================

The following will show you how to enable TLS profile to protect the content that travels over the connection for all channels. A really usual example of use is to first protect the connection with TLS (which is what we are going to explain) and the start a SASL channel to do the auth part.

1. Anyhow, the first thing you must do is to import the required components::

    import vortex
    import vortex.tls

2. Now, at the server initialization, usually before starting all listeners (vortex.create_listener) you call to register the handlers that will be called to report certificates to be used each time a request to enable TLS is received::

    # enable tls support
    vortex.tls.accept_tls (ctx, 
                           # accept handler
                           accept_handler=tls_accept_handler, accept_handler_data="test", 
                           # cert handler
                           cert_handler=tls_cert_handler, cert_handler_data="test 2",
                           # key handler
                           key_handler=tls_key_handler, key_handler_data="test 3")

3. In the example, is used tls_accept_handler, tls_cert_handler and tls_key_handler to show the concept on how to pass values to those handlers. Now, those tree handlers must return the right values so the vortex engine can successfully activate TLS negotiation. Here is an example::

       def tls_accept_handler(conn, server_name, data):
            # accept TLS request 
            return True

       def tls_cert_handler (conn, server_name, data):
            return "test.crt"

       def tls_key_handler (conn, server_name, data):
            return "test.key"

In the example the tree handler mostly do the minimal effort to complete their job. A more elaborated example will include doing some additional operations to tls_accept_handler to filter the connection according to source address, and/or, inside tls_cert_handler/tls_key_handler return a different certificate according to server_name value received.

Once a connection is successfully secured with TLS, you can call the following to check it at your frame received handlers, for example, if you want to ensure your server do not provide any data without having a TLS secured connection::

     if not vortex.tls.is_enabled (conn):
     	# connection is not secured, close it, or whatever required to stop
        conn.shutdown ()



