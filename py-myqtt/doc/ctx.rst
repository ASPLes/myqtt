:mod:`myqtt.Ctx` --- PyMyQttCtx class: context handling
=========================================================

.. currentmodule:: myqtt


API documentation for myqtt.Ctx object representing a myqtt
independent context. myqtt.Ctx object is a fundamental type and
represents a single execution context within a process, that includes
the reader loop, the sequencer loop and a shared context execution for
all connections, listeners and channels created inside it.

In most situations it is only required to create a single context as follows::

   ctx = myqtt.Ctx ()
   if not ctx.init ():
        print ("ERROR: Failed to create myqtt.Ctx object")

   # now it is possible to create connections and channels
   conn = myqtt.Conn (ctx, "localhost", "1883")
   if not conn.is_ok ():
        print ("ERROR: connection failed to localhost, error was: " + conn.error_msg)

Due to python automatic collection it is not required to explicitly
close a context because that's is done automatically when the
function/method that created the ctx object finishes. However, to
explicitly terminate a myqtt execution context just do::

  ctx.exit ()

After creating a context, you can now either create a connection or a
listener using:

.. toctree::
   :maxdepth: 1

   conn

==========
Module API
==========

.. class:: Ctx

   .. method:: init ()
   
      Allows to init the context created.

      :rtype: Returns True if the context was started and initialized or False if failed.

   .. method:: exit ()
   
      Allows to finish an initialized context (:meth:`Ctx.init`)

   .. method:: new_event (microseconds, handler, [user_data], [user_data2])
   
      Allows to install an event handler, a method that will be called each microseconds, optionally receiving one or two parameters. This method implements python api for myqtt_thread_pool_new_event. See also its documentation.

      The following is an example of handler (first parameter is myqtt.Ctx, the rest two are the user parameters)::

            def event_handler (ctx, param1, param2):
      	    	# do some stuff
		if something:
	     	   return True # return True to remove the event (no more calls)

	  	return False # make the event to keep on working (more calls to come)

      :rtype: Returns a handle id representing the event installed. This handle id can be used to remove the event from outside the handler. Use :mod:`remove_event` to do so.

   .. method:: remove_event (handle_id)
   
      Allows to remove an event installed using :mod:`new_event` providing the handle_id representing the event. This value is the unique identifier returned by :mod:`new_event` every time a new event is installed.

      :rtype: Returns True if the event was found and removed, otherwise False is returned.

   .. method:: enable_too_long_notify_to_file (watching_period, file)
   
      Allows to enable a too long notify handler that internally logs those notifications into the provided file. The watching_period allows to control what is the period over which a notification is recorded.

      :rtype: Returns True if the notifier was installed

   .. attribute:: log

      (Read/Write attribute) (True/False) returns or set current debug log. See myqtt_log_is_enabled.

   .. attribute:: log2

      (Read/Write attribute) (True/False) returns or set current second level debug log which includes more detailed messages suppresed. See myqtt_log2_is_enabled.

   .. attribute:: color_log

      (Read only attribute) (True/False) returns or set current debug log colourification. See myqtt_color_log_is_enabled.

   .. attribute:: ref_count

      (Read only attribute) (Number) returns current myqtt.Ctx reference counting state.


   .. method:: set_on_publish (on_publish_handler, on_publish_data)
   
      Allows to configure a handler that is called every time a
      PUBLISH operation is received so the handler can be used to
      filter and autorized messages received or to redirect them::

	def on_publish (ctx, conn, msg, data):

           return myqtt.PUBLISH_DISCARD # to discard
	   return myqtt.PUBLISH_OK # to allow publish
      
   .. method:: set_on_connect (on_connect_handler, on_connect_data)
   
      Allows to configure a handler that is called every time a
      CONNECT is received::

	def on_connect (ctx, conn, data):

           return myqtt.CONNACK_BAD_USERNAME_OR_PASSWORD # to discard
	   return myqtt.CONNACK_ACCEPTED # to allow publish


   .. method:: set_log_handler (log_handler, log_handler_data)
   
      Allows to configure a handler that is called when the engine wants to report a log (myqtt.level_debug, myqtt.level_warning, myqtt.level_critical)::

	def log_handler (ctx, __file, line, log_level, msg, data):
	    # do some reporting here: see test_11 from myqtt-regression-client.py for more details
	    return


   .. method:: storage_set_path (storage_path, hash_size)
   
      Allows to configure storage path and hashing value. The hashing
      value allows to group temporal messages into the number of
      folders provided. 4096 is a good recommended values.

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
      
      Note using this function is very rare and highly not recommended.

      NOTE: using this code is really only recommended in very few
      cases where myqtt usage is being done from a process that starts
      and finishes on every requests, thus, resource deallocation is
      not an issue. However, it is highly not recommended.
            
    
