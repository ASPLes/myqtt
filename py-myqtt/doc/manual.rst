PyMyqtt manual
===============

===============================
Creating a PyMyQtt MQTT  client
===============================

1. First, we have to create a MyQtt context. This is done like follows::

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


2. Now, we have to establish a MQTT session with the remote listener. This is done like follows::

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


5. Now, to receive replies and other requests, we have to configure a frame received handler::

     def on_msg (ctx, conn, msg, data):
         # messages received asynchronously
	 print ("Msg content: " + msg.payload)
	 return
   
     conn.set_msg_received (on_msg)


.. note::

   Note also full regression test client, which includes all features tested, is located at: https://dolphin.aspl.es/svn/publico/myqtt/py-myqtt/test/myqtt-regression-client.py

==================================
Creating a MQTT server with Python
==================================

1. The process of creating a MQTT listener is pretty
   straitforward. You have to follow the same initialization process
   as the client. This is done as follows::

     # create a context
     ctx = myqtt.Ctx ()

     # init context
     if not ctx.init ():
         error ("Unable to init ctx, failed to start listener")
         sys.exit(-1)

2. After your listener now define some settings and start the list of
   listeners you need::

     # configure storage
     ctx.storage_set_path (".myqtt-listener", 4096)
     
     # create a listener
     info ("Starting listener at 0.0.0.0:1883")
     listener = myqtt.create_listener (ctx, "0.0.0.0", "1883")

     # check listener started
     if not listener.is_ok ():
         error ("ERROR: failed to start listener. Maybe there is another instance running at 1883?")
         sys.exit (-1)

     # maybe start more listeners here by calling to myqtt.create_listener

3. Because we have to wait for msg to be received we need a wait to
   block the listener. The following is not strictly necessary it you have another way to make the main thread to not finish::

     # wait for requests
     myqtt.wait_listeners (ctx, unlock_on_signal=True)
   

.. note::

   Full listener source code can be found at: https://dolphin.aspl.es/svn/publico/myqtt/py-myqtt/test/myqtt-regression-listener.py


4. With the previous simple code you already have a working MQTT
   server that will allow subscription and publishing to any
   topic. Possible, at this point you would like to control how
   messages are published (to discard them, route them, etc). This is
   done by setting a on_publish handler like this::

     def on_publish (ctx, conn, msg, data):
         info ("Topic received: %s" % (msg.topic))

	 if msg.topic == "disard/this/topic":
	     return myqtt.PUBLISH_DISCARD

	 if some_limit_reached_for (conn):
 	     return myqtt.PUBLISH_DISCARD

	 # for the rest of cases
	 return myqtt.PUBLISH_OK

     # configure on publish 
     ctx.set_on_publish (on_publish)

     # please check
     https://dolphin.aspl.es/svn/publico/myqtt/py-myqtt/test/myqtt-regression-listener.py
     for many supported working examples

===================================
Enabling server side authentication
===================================

To enable server side authentication you have to use on_connect
handler where you configure a python function of your choice. Then,
inside that function, you program your database auth backend which
could be a simple list of users hard coded, or a list of users in a
file or more complex tasks like querying a MySQL database to do that
auth.

A very simple example could be::

  # configure on connect handler which is called every time a new
  # connection is received. See also documentation associated to
  # myqtt_ctx_set_on_connect ()
  # 
  ctx.set_on_connect (on_connect)
  
  # then, somewhere in your code place the on_connect handler like
  # this:
  
  def on_connect (ctx, conn, data):
    # very simple example where aspl:test is the only authorized user
    info ("Called on connect handler..")

    if conn.username and conn.password:
        # username and password where defined, handle them
        if conn.username != "aspl" or conn.password != "test":
            return myqtt.CONNACK_BAD_USERNAME_OR_PASSWORD
        # end if

    # end if

    return myqtt.CONNACK_ACCEPTED

Please, check full working version at https://dolphin.aspl.es/svn/publico/myqtt/py-myqtt/test/myqtt-regression-listener.py
  


===================================
Enabling server side TLS encryption
===================================





