:mod:`vortex.handlers` --- PyVortex handlers: List of handlers used by PyVortex API
===================================================================================

.. currentmodule:: vortex


The following are a list of handlers that are used by PyVortex to
notify events. They include a description and an signature example to
ease development:

.. _channel-start-handler:

=====================
Channel start handler
=====================

This handler is executed when a channel start request is received. The
handler returns True to accept channel creation or False to deny
it. Its signature is the following::

    def channel_start_received (channel_num, conn, data):
	      
	# accept the channel to be created
	return True

.. _channel-close-handler:

=====================
Channel close handler
=====================

This handler is executed when a channel close request is received. The
handler returns True to accept channel to be closed, otherwise False
to cancel close operation. Its signature is the following::

    def channel_close_request (channel_num, conn, data):

	# accept the channel to be closed
	return True 

.. _on-channel-handler:

==================
On channel handler
==================

This handler is executed when an async channel creation was requested
(at vortex.Connection.open_channel). The handler receives the channel
created or None reference if a failure was found. Here is an example::

    def on_channel (number, channel, conn, data):
    	# number: of the channel number that was created. In case of failure -1 is received.
	# channel: is the vortex.Channel reference created or None if it failed.
	# conn: the connection where the channel was created
	# data: user defined data (set at open_channel)
	return

.. _frame-received-handler:

======================
Frame received handler
======================

This handler is executed when PyVortex needs to notify a frame
received. Its signature is the following::

    def frame_received (conn, channel, frame, data):
        # handle the frame here
        return

.. _on-close-handler:

===========================
On connection close handler
===========================

This handler is executed when a connection is suddently closed (broken
pipe). Its signature is the following::

    def connection_closed (conn, data):
        # handle connection close
        return

.. _tls-notify-handler:

===============================
TLS status notification handler
===============================

This handler is used to notify TLS activation on a connection. The
handler signature is::

    def tls_notify (conn, status, status_msg, data):
        # handle TLS request
        return

.. _tls-accept-handler:

==========================
TLS accept request handler
==========================

This handler is used accept or deny an incoming TLS request. The
handler must returnd True to accept the request to continue or False
to cancel it. The handler signature is::

    def tls_accept_handler(conn, server_name, data):
        # accept TLS request
        return True

.. _tls-cert-handler:

================================
TLS certificate location handler
================================

This handler is used to get the location of the certificate to be used
during the activation of the TLS profile. The handler signature is::

    def tls_cert_handler (conn, server_name, data):
        return "test.crt"

.. _tls-key-handler:

========================
TLS key location handler
========================

This handler is used to get the location of the private key to be used
during the activation of the TLS profile. The handler signature is::

    def tls_key_handler (conn, server_name, data):
        return "test.key"


.. _create-channel-handler:

===============================================
Channel create handler (for vortex.ChannelPool)
===============================================

This handler is executed when a vortex.ChannelPool requires to add a
new channel into the pool. Its signature is the following::

    def create_channel (conn, channel_num, profile, received, received_data, close, close_data, user_data, next_data):
    	# create a channel
        return conn.open_channel (channel_num, profile)

.. _on-channel-pool-created:

=========================================================
Channel pool create notification (for vortex.ChannelPool)
=========================================================

This handler is executed when a vortex.ChannelPool was created and the
on_created handler was configured at
vortex.Connection.channel_pool_new. Its signature is the following::

    def on_pool_created (pool, data):
    	print ("Pool created: " + str (pool))
        return 
