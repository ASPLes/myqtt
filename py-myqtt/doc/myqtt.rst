:mod:`vortex` --- PyVortex base module: base functions (create listeners, register handlers)
============================================================================================

.. module:: vortex
   :synopsis: Vortex library base module
.. moduleauthor:: Advanced Software Production Line, S.L.


This modules includes all functions required to establish a MQTT
session, create MQTT listeners

This module also includes type definition for the following classes:

.. toctree::
   :maxdepth: 1

   ctx
   connection
   msg
   asyncqueue
   handlers

==========
Module API
==========

.. function:: create_listener (ctx, host, port)

   Allows to create a BEEP listener to receiving incoming
   connections. Here is an example:: 

      # create a listener
      listener = vortex.create_listener (ctx, "0.0.0.0", "44010")

      # check listener started
      if not listener.is_ok ():
          # do some error handling
          sys.exit (-1)

      # do a wait operation
      vortex.wait_listeners (ctx, unlock_on_signal=True)

   :param ctx: vortex context where the listener will be created
   :type ctx: vortex.Ctx
   
   :param host: the hostname
   :type host: String
  
   :param port: the port to connect to
   :type  port: String

   :rtype: vortex.Connection representing the listener created.

.. function:: wait_listeners (ctx, [unlock_on_signal])

   Allows to perform a wait operation until vortex context is finished
   or due to a signal received. 

   :param ctx: context where a listener or a set of listener were created.
   :type ctx: vortex.Ctx

   :param unlock_on_signal: unlock_on_signal expects to receive True to make wait_listener to unlock on signal received.
   :type unlock_on_signal: Integer: True or False

.. function:: unlock_listeners (ctx)

   Allows to unlock the thread that is blocked at wait_listeners call

   :param ctx: context where a listener or a set of listener were created.
   :type ctx: vortex.Ctx


.. function:: queue_reply(conn, channel, frame, o)

   Function used inside the queue reply method. This function is used
   as frame received handler, queuring all frames in the queue
   provided as user data. The, a call to channel.get_reply (queue) is
   required to get all frames received.

   Here is an example::

   	# configure frame received handler 
	queue = vortex.AsyncQueue ()
	channel.set_frame_received (vortex.queue_reply, queue)

	# wait for frames to be received
	frame = channel.get_reply (queue)

