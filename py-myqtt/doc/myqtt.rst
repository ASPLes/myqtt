:mod:`myqtt` --- PyMyQtt base module: base functions (create listeners, register handlers)
==========================================================================================

.. module:: myqtt
   :synopsis: MyQtt library base module
.. moduleauthor:: Advanced Software Production Line, S.L.


This modules includes all functions required to establish a MQTT
session, create MQTT listeners

This module also includes type definition for the following classes:

.. toctree::
   :maxdepth: 1

   ctx
   conn
   msg
   asyncqueue
   handlers

==========
Module API
==========

.. function:: create_listener (ctx, host, port)

   Allows to create a MQTT listener to receiving incoming
   connections. Here is an example::

      # create a listener
      listener = myqtt.create_listener (ctx, "0.0.0.0", "1883")

      # check listener started
      if not listener.is_ok ():
          # do some error handling
          sys.exit (-1)

      # do a wait operation
      myqtt.wait_listeners (ctx, unlock_on_signal=True)

   :param ctx: myqtt context where the listener will be created
   :type ctx: myqtt.Ctx
   
   :param host: the hostname
   :type host: String
  
   :param port: the port to connect to
   :type  port: String

   :rtype: myqtt.Connection representing the listener created.

.. function:: wait_listeners (ctx, [unlock_on_signal])

   Allows to perform a wait operation until myqtt context is finished
   or due to a signal received. 

   :param ctx: context where a listener or a set of listener were created.
   :type ctx: myqtt.Ctx

   :param unlock_on_signal: unlock_on_signal expects to receive True to make wait_listener to unlock on signal received.
   :type unlock_on_signal: Integer: True or False

.. function:: unlock_listeners (ctx)

   Allows to unlock the thread that is blocked at wait_listeners call

   :param ctx: context where a listener or a set of listener were created.
   :type ctx: myqtt.Ctx


