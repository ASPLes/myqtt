:mod:`myqtt.Conn` --- PyMyQttConn class: MQTT connection creation and management
================================================================================

.. currentmodule:: myqtt


API documentation for myqtt.Conn, an object that represents a MQTT
connection. 

To create a connection, you need a context where to create it. See
myqtt.Ctx documenation to know about it: :class:`myqtt.Ctx`

A connection is created as follows::

   conn = myqtt.Conn (ctx, "localhost", "1883")
   if not conn.is_ok ():
       print ("ERROR: connection failed, error was: " + conn.error_msg)
       return

Note that after creating a connection, you must check if it is ok
using :meth:`myqtt.Conn.is_ok` method.

Once a connection is created, you can publish (conn.pub) and subscribe
(conn.pub) to topics.


==========
Module API
==========

.. class:: Conn (ctx, host, port [, serverName])

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

   .. method:: is_ok ()

      Allows to check if the current instance is properly connected  and available for operations.  This method is used to check  connection status after a failure.

      :rtype: True if the connection is ready, otherwise False is returned.

   .. method:: sub (topic, [qos, retain, wait_publish])

      Allows to subscribe to a particular topic on the provided
      connection	       

      :param topic: The topic to subscribe to.
      :type topic: String

      :param qos: The QoS requested for this subscription
      :type qos: String

      :param retain: Enable/disable retain flag for this subscription
      :type retain: Boolean

      :param wait_sub: Allows to configure the amount of seconds to  wait for the operation to complete
      :type wait_sub: Number

      :rtype: (status, sub_result) Status returns True or False if
	      subscription finished without error and sub_result is
	      the qos value granted by the server

   .. method:: pub (topic, msg, msg_size, [qos, retain, wait_publish])

      Allows to publish the provided message to a particular topic on
      the provided connection

      :param topic: The topic to subscribe to.
      :type topic: String

      :param msg: The message that will be published
      :type msg: String

      :param msg_size: Size of the message to be published
      :type msg_size: Number

      :param qos: The QoS requested for this subscription
      :type qos: String

      :param retain: Enable/disable retain flag for this subscription
      :type retain: Boolean

      :param wait_sub: Allows to configure the amount of seconds to  wait for the operation to complete
      :type wait_sub: Number

      :rtype: True or False if publish operation finished without error

   .. method:: ping ([wait_pingresp])

      Allows to ping remote server and optionally configure how long   to wait for the reply

      :param wait_pingresp: The topic to subscribe to.
      :type wait_pingresp: Number

      :rtype: True or False if ping operation finished without error

   .. method:: get_next ([timeout])

      Allows to block the caller until the next message is returned.

      :param timeout: How long to wait for a reply in seconds. By
		      default 10 seconds is internal configured if
		      nothing is provided. 0 to just get pending
		      message (if any) without waiting.
      :type timeout: Number

      :rtype: Returns next message receieved :ref:`myqtt.Msg` or None if timeout is reached and no message was received.

   .. method:: set_on_msg (on_msg, [on_msg_data])
   
      Allows to configure an async notification handler that will be
      called every time a message is received.

      :param on_msg: The handler to be executed when a message is
		     received on the connection configured
      :type  on_msg: :ref:`on-msg-handler`.

      :param on_msg_data: The user defined data to be passed to the on_msg handler along with handler corresponding parameters.
      :type  on_msg_data: Object

   .. method:: set_on_reconnect (on_reconnect, [on_reconnect_data])
   
      Allows to configure an async notification handler that will be
      called every time a reconnect operation is detected on the
      provided connection

      :param on_reconnect: The handler to be executed when a reconnect
			   is detected on the connection configured
      :type  on_reconnect: :ref:`on-reconnect-handler`.

      :param on_reconnect_data: The user defined data to be passed to the on_reconnect handler along with handler corresponding parameters.
      :type  on_reconnect_data: Object



   .. method:: set_on_close (on_close, [on_close_data])
   
      Allows to configure a handler which will be called in the case the connection is closed. This is useful to detect broken pipe.

      :param on_close: The handler to be executed when the connection close is detected on the instance provided.
      :type  on_close: :ref:`on-close-handler`.

      :param on_close_data: The user defined data to be passed to the on_close handler along with handler corresponding parameters.
      :type  on_close_data: Object

      :return: Returns a new reference to a myqtt.Handle that can be used to remove the on close handler configured using :meth:`remove_on_close`

   .. method:: remove_on_close (handle_ref)
   
      Allows to remove an on close handler configured using :meth:`set_on_close`. The close handler to remove is identified by the handle_ref parameter which is the value returned by the :meth:`set_on_close` handler.

      :param handle_ref: Reference to the on close handler to remove (value returned by :meth:`set_on_close`).
      :type  handl_ref: :class:`myqtt.Handle`

      :return: True in the case the handler was found and removed, otherwise False is returned.


   .. method:: set_data (key, object)
   
      Allows to store arbitrary references associated to the connection. See also get_data.

      :param key: The key index to which the data gets associated.
      :type  key: String

      :param object: The object to store associated on the connection index by the given key.
      :type  object: Object

   .. method:: get_data (key)
   
      Allows to retreive a previously stored object using set_data

      :param key: The index for the object looked up
      :type  key: String

      :rtype: Returns the object stored or None if fails. 

   .. method:: block ([block=True])
   
      Allows to block all incoming content on the provided connection by skiping connection available data state. This method binds myqtt_connection_block C API.

      :param block: Optional boolean value that configure if the connection must be blocked (True) or unblocked (False). If not configured the connection is blocked (True).
      :type  block: Boolean (True if not configured)

   .. method:: is_blocked ()
   
      :rtype: Returns if the connection is blocked (due to :meth:`block`) or not.

   .. method:: close ()
   
      Allows to close the connection using full MQTT close negotation procotol.  

   .. method:: shutdown ()
   
      Allows to close the connection by shutting down the transport layer supporting it. This causes the connection to be closed without taking place MQTT close indication. 

   .. method:: incref ()
   
      Allows to increment python reference count.  This is used in cases where the connection reference is automatically collected by python GC but the MyQttConnection reference that it was representing is still working (and may receive notifications, like frame received). Note that a call to this method, requires a call to :meth:`decref`.

   .. method:: decref ()
   
      Allows to decrement python reference count.  See :meth:`incref` for more information.

   .. method:: skip_conn_close ([skip])
   
      Allows to configure this connection reference to not call to shutdown its associated reference when the python connection is collected. This method is really useful when it is required to keep working a connection that was created inside a function but that has finished.

      By default, any :class:`myqtt.Conn` object created will be finished when its environment finishes. This means that when the function that created the connection finished, then the connection will be finished automatically.

      In many situations this is a desirable behaviour because your
      python application finishes automatically all the stuff
      opened. However, when the connection is created inside a handler
      or some method that implements connection startup but do not
      waits for the reply (asynchronous replies), then the connection
      must be still running until reply arrives. For these scenarios
      you have to use :meth:`skip_conn_close`.

   .. method:: gc ([disable_gc = True])

      Allows to disable automatic memory collection for python
      references finished. By default, PyMyQtt closes connections,
      releases messages and finishes contexts when they are no longer
      used (as notified by Python engine via _dealloc internal C
      funciton).
      
      In general this is the recommended approach and in most of the
      cases you'll notice any problem. 
      
      However, in some cases where it might be needed to disable this
      deallocation (causing an automatic connection close, context
      close, etc) when the scope where that variable is finished, then
      use this function.

      NOTE: using this code is really only recommended in very few
      cases where myqtt usage is being done from a process that starts
      and finishes on every requests, thus, resource deallocation is
      not an issue. However, it is highly not recommended.
      
   .. attribute:: id

      (Read only attribute) (Integer) returns the connection unique identifier.

   .. attribute:: ctx

      (Read only attribute) (myqtt.Ctx) returns the context where the connection was created.

   .. attribute:: error_msg

      (Read only attribute) (String) returns the last error message found while using the connection.

   .. attribute:: status

      (Read only attribute) (Integer) returns an integer status code representing the textual error string returned by attribute error_msg.

   .. attribute:: host

      (Read only attribute) (String) returns the host string the connection is connected to.

   .. attribute:: host_ip

      (Read only attribute) (String) returns the host IP string the connection is connected to.

   .. attribute::  port

      (Read only attribute) (String) returns the port string the connection is using to connected to.

   .. attribute:: server_name

      (Read only attribute) (String) returns connection's configured serverName

   .. attribute:: local_addr

      (Read only attribute) (String) returns the local address used by the connection.

   .. attribute::  local_port

      (Read only attribute) (String) returns the local port string used by the connection.

   .. attribute:: role

      (Read only attribute) (String) returns a string representing the connection role. Allowed connection role strings are: initiator (client connection), listener (connection that was accepted due to a listener installed), master-listener (the master listener that is waiting for new incoming requests).  

   .. attribute:: ref_count

      (Read only attribute) (Integer) returns reference counting
      current state.

   .. attribute:: socket

      (Read only attribute) (Integer) returns the socket associated
      to the cnonection

   .. attribute:: client_id

      (Read only attribute) (String) returns the connection client id
      configured

   .. attribute:: last_err

      (Read only attribute) (Integer) returns last connection error
      found

   .. attribute:: username

      (Read only attribute) (String) returns the username that was
      used by this connection (if any) during CONNECT

   .. attribute:: password

      (Read only attribute) (String) returns the password that was
      used by this connection (if any) during CONNECT. It may not be
      defined later (because engine clears this value once used). In
      the case reconnect support is enabled, it will remain available.

   .. attribute:: ref_count

      (Read only attribute) (Integer) returns reference counting current state.




		  
	  
