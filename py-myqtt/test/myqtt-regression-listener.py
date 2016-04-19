#!/usr/bin/python
# -*- coding: utf-8 -*-
#  MyQtt: A high performance open source MQTT implementation
#  Copyright (C) 2016 Advanced Software Production Line, S.L.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public License
#  as published by the Free Software Foundation; either version 2.1
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free
#  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
#  02111-1307 USA
#  
#  You may find a copy of the license under this software is released
#  at COPYING file. This is LGPL software: you are welcome to develop
#  proprietary applications using this library without any royalty or
#  fee but returning back any change, improvement or addition in the
#  form of source code, project image, documentation patches, etc.
#
#  For commercial support on build MQTT enabled solutions contact us:
#          
#      Postal address:
#         Advanced Software Production Line, S.L.
#         C/ Antonio Suarez Nº 10, 
#         Edificio Alius A, Despacho 102
#         Alcalá de Henares 28802 (Madrid)
#         Spain
#
#      Email address:
#         info@aspl.es - http://www.aspl.es/mqtt
#                        http://www.aspl.es/myqtt
#
import myqtt
import myqtt.tls
import time

import os
import signal
import sys

def info (msg):
    print "[ INFO  ] : " + msg

def error (msg):
    print "[ ERROR ] : " + msg

def ok (msg):
    print "[  OK   ] : " + msg

def signal_handler (signal, stackframe):
    print ("Received signal: " + str (signal))
    return

def on_publish (ctx, conn, msg, data):
    info ("Topic received: %s" % (msg.topic))
    if msg.topic == "myqtt/admin/get-conn-user":
        time.sleep (1)
        if not conn.pub ("myqtt/admin/get-conn-user", conn.username, len (conn.username), myqtt.qos0, False, 0):
            error ("Error: failed to publish the connection's username")

        # now indicate engine to discard message received
        return myqtt.PUBLISH_DISCARD

    if msg.topic == "myqtt/admin/get-client-identifier":
        time.sleep (1)
        info ("Sending client identifier...")
        if not conn.pub ("myqtt/admin/get-client-identifier", conn.client_id, len (conn.client_id), myqtt.qos0, False, 0):
            error ("Error: failed to publish the client identifier")

        # now indicate engine to discard message received
        return myqtt.PUBLISH_DISCARD

    if msg.topic == "myqtt/admin/get-server-name":
        server_name = conn.server_name
        if not server_name:
            server_name = conn.host
        
        if not conn.pub ("myqtt/admin/get-server-name", server_name, len (server_name), myqtt.qos0, False, 0):
            error ("Error: failed to send serverName")

        return myqtt.PUBLISH_DISCARD

    # let know engine to go ahead publishing the message received
    return myqtt.PUBLISH_OK

def on_connect (ctx, conn, data):
    info ("Called on connect handler..")

    if conn.username and conn.password:
        # username and password where defined, handle them
        if conn.username != "aspl" or conn.password != "test":
            return myqtt.CONNACK_BAD_USERNAME_OR_PASSWORD
        # end if

    # end if

    return myqtt.CONNACK_ACCEPTED

def __certificate_handler (ctx, conn, serverName, userData):

    if serverName == "localhost":
        return "../../test/localhost-server.crt"

    if serverName == "test19a.localhost":
        return "../../test/test19a-localhost-server.crt"

    return None

def __private_handler (ctx, conn, serverName, userData):

    if serverName == "localhost":
        return "../../test/localhost-server.key"

    if serverName == "test19a.localhost":
        return "../../test/test19a-localhost-server.key"
    

    return None

def __chain_handler (ctx, conn, serverName, userData):

    return None
    
        
if __name__ == '__main__':

    # create a context
    ctx = myqtt.Ctx ()

    # init context
    if not ctx.init ():
        error ("Unable to init ctx, failed to start listener")
        sys.exit(-1)

    # configure signal handling
    signal.signal (signal.SIGTERM, signal_handler)
    signal.signal (signal.SIGINT, signal_handler)
    signal.signal (signal.SIGQUIT, signal_handler)

    # configure storage
    info ("Configuring MQTT storage path..")
    os.system ("find .myqtt-listener -type f -exec rm {} \\; > /dev/null 2>&1")
    ctx.storage_set_path (".myqtt-listener", 4096)

    # create a listener
    info ("Starting listener at 0.0.0.0:34010")
    listener = myqtt.create_listener (ctx, "0.0.0.0", "34010")

    # check listener started
    if not listener.is_ok ():
        error ("ERROR: failed to start listener. Maybe there is another instance running at 34010?")
        sys.exit (-1)

    ### begin: TLS support
    listener2 = myqtt.tls.create_listener (ctx, "0.0.0.0", "34011")
    if not listener2.is_ok ():
        error ("ERROR: failed to create TLS listener..")
        sys.exit (-1)
        
    # set certificate
    myqtt.tls.set_certificate (listener2, "../../test/test-certificate.crt",  "../../test/test-private.key")
    opts = myqtt.ConnOpts ()
    if not myqtt.tls.set_ssl_certs (opts, "../../test/server.pem", "../../test/server.pem", None, "../../test/root.pem"):
        error ("Unable to configure certificates, server.pem and root.pem")
        sys.exit (-1)
    # end if

    # do not verify peer
    myqtt.tls.ssl_peer_verify (opts, True)

    listener3 = myqtt.tls.create_listener (ctx, "0.0.0.0", "1911", opts)
    if not listener3.is_ok ():
        error ("ERROR: failed to create TLS listener with especial options..")
        sys.exit (-1)    

    # configure auxiliar handlers 
    myqtt.tls.set_certificate_handlers (ctx, __certificate_handler, __private_handler, __chain_handler)
    
    ### end: TLS support

    # configure on publish
    ctx.set_on_publish (on_publish)
    ctx.set_on_connect (on_connect)

    # do a wait operation
    info ("waiting requests..")
    myqtt.wait_listeners (ctx, unlock_on_signal=True)

    

        
