:mod:`myqtt.handlers` --- PyMyQtt handlers: List of handlers used by PyMyQtt API
================================================================================

.. currentmodule:: myqtt


The following are a list of handlers that are used by PyMyQtt to
notify events. They include a description and an signature example to
ease development:

.. _msg-received-handler:

=================================
Msg received handler (on message)
=================================

This handler is executed when PyMyQtt needs to notify a msg
received. Its signature is the following::

    def msg_received (ctx, conn, msg, data):
        # handle the msg here
        return

This handler is configured by conn.set_on_msg and ctx.set_on_publish

.. _on-close-handler:

===========================
On connection close handler
===========================

This handler is executed when a connection is suddently closed (broken
pipe). Its signature is the following::

    def connection_closed (conn, data):
        # handle connection close
        return

.. _tls-cert-handler:

================================
TLS certificate location handler
================================

This handler is used to get the location of the certificate to be used
during the activation of a TLS listener. The handler signature is::

    def tls_cert_handler (conn, server_name, data):
        return "test.crt"

.. _tls-key-handler:

========================
TLS key location handler
========================

This handler is used to get the location of the private key to be used
during the activation of a TLS listener. The handler signature is::

    def tls_key_handler (conn, server_name, data):
        return "test.key"



