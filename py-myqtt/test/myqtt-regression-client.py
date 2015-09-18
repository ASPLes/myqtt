#!/usr/bin/python
# -*- coding: utf-8 -*-
#  MyQtt: A high performance open source MQTT implementation
#  Copyright (C) 2014 Advanced Software Production Line, S.L.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public License
#  as published by the Free Software Foundation; either version 2.1 of
#  the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
#  USA
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

# import sys for command line parsing
import sys
import time
import os

# import python myqtt binding
import myqtt

####################
# regression tests #
####################

def test_00_a_check (queue):

    a_tuple = queue.pop ()
    if not a_tuple:
        error ("Found not defined expected tuple, but found: " + a_tuple)
        return False
    if a_tuple[0] != 2 or a_tuple[1] != 3:
        error ("Expected to find differente values but found: " + str (a_tuple[0]) + ", and: " + str (a_tuple[1]))
        return False

    # get a string
    a_string = queue.pop ()
    if a_string != "This is an string":
        error ("Expected to receive string: 'This is an string', but received: " + a_string)
        return False

    # get a list
    a_list = queue.pop ()
    if len (a_list) != 4:
        error ("Expected to find list length: " + len (a_list))
        return False

    return True

def test_00_a():
    ##########
    # create a queue
    queue = myqtt.AsyncQueue ()

    # call to terminate queue 
    del queue


    ######### now check data storage
    queue = myqtt.AsyncQueue ()
    
    # push items
    queue.push (1)
    queue.push (2)
    queue.push (3)
    
    # get items
    value = queue.pop ()
    if value != 1:
        error ("Expected to find 1 but found: " + str(value))
        return False

    value = queue.pop ()
    if value != 2:
        error ("Expected to find 2 but found: " + str(value))
        return False

    value = queue.pop ()
    if value != 3:
        error ("Expected to find 3 but found: " + str(value))
        return False

    # call to unref 
    # del queue # queue.unref ()

    ###### now masive add operations
    queue = myqtt.AsyncQueue ()

    # add items
    iterator = 0
    while iterator < 1000:
        queue.push (iterator)
        iterator += 1

    # restore items
    iterator = 0
    while iterator < 1000:
        value = queue.pop ()
        if value != iterator:
            error ("Expected to find: " + str(value) + ", but found: " + str(iterator))
            return False
        iterator += 1

    ##### now add different types of data
    queue = myqtt.AsyncQueue ()

    queue.push ((2, 3))
    queue.push ("This is an string")
    queue.push ([1, 2, 3, 4])

    # get a tuple
    if not test_00_a_check (queue):
        return False

    #### now add several different item
    queue    = myqtt.AsyncQueue ()
    iterator = 0
    while iterator < 1000:
        
        queue.push ((2, 3))
        queue.push ("This is an string")
        queue.push ([1, 2, 3, 4])

        # next iterator
        iterator += 1

    # now retreive all items
    iterator = 0
    while iterator < 1000:
        # check queue items
        if not test_00_a_check (queue):
            return False
        
        # next iterator
        iterator += 1

    return True

def test_01():
    # call to initilize a context and to finish it 
    ctx = myqtt.Ctx ()

    # init context and finish it */
    info ("init context..")
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    # ok, now finish context
    info ("finishing context..")
    ctx.exit ()

    # finish ctx 
    del ctx

    return True

def test_02():
    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    # call to create a connection
    conn = myqtt.Conn (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    info ("MQTT connection created to: " + conn.host + ":" + conn.port) 
    
    # now close the connection
    info ("Now closing the MQTT session..")
    conn.close ()

    ctx.exit ()

    # finish ctx 
    del ctx

    return True


def test_03 ():
    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    # call to create a connection
    conn = myqtt.Conn (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    info ("MQTT connection created to: " + conn.host + ":" + conn.port) 

    # now subscribe
    (status, sub_qos) = conn.sub ("topic", myqtt.qos0, 10)
    if not status:
        error ("Failed to subscribe")
        return False

    info ("Subscription done, sub_qos is: %d" % sub_qos)

    # check here subscriptions done
    
    # now close the connection
    info ("Now closing the MQTT session..")
    conn.close ()

    ctx.exit ()

    # finish ctx 
    del ctx

    return True

def test_04_on_msg (ctx, conn, msg, data):
    info ("Test --: called test_04_on_msg, pushing message...")
    queue = data
    queue.push (msg)
    return

def test_04_fail (ctx, conn, msg, data):
    error ("test_04_fail: Shouldn't be called, failing..")
    return

def test_04 ():
    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    # call to create a connection
    # client_identifier = "test_03"
    # clean_session = True
    # keep_alive = 30
    conn = myqtt.Conn (ctx, host, port, "test_03", True, 30)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    info ("MQTT connection created to: " + conn.host + ":" + conn.port) 

    # call to create a connection
    # client_identifier = "test_03-2"
    # clean_session = True
    # keep_alive = 30
    conn2 = myqtt.Conn (ctx, host, port, "test_03-2", True, 30)

    # check connection status after if 
    if not conn2.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now subscribe with conn
    (status, sub_qos) = conn.sub ("topic", myqtt.qos0, 10)
    if not status:
        error ("Failed to subscribe")
        return False

    info ("Subscription done, sub_qos is: %d" % sub_qos)

    # create queue
    queue = myqtt.AsyncQueue ()
    conn.set_on_msg (test_04_on_msg, queue)
    conn2.set_on_msg (test_04_fail)

    info ("Publishing message...")
    if not conn.pub ("topic", "This is a test message....", 24, myqtt.qos0, False, 0):
        error ("Failed to publish message..")
        return False

    # receive message
    info ("Getting notification from queue (should call test_04_on_msg)")
    msg = queue.pop ()

    if msg.size != 24:
        error ("Expected payload size of 24 but found: %d.." % msg.payload_size)
        return False

    if msg.type != "PUBLISH":
        error ("Expected PUBLISH message but found: %s" % msg.type)
        return False

    if msg.content != "This is a test message..":
        error ("Expected different message, but found: " + msg.content)
        return False

    # now close the connection
    info ("Now closing both MQTT sessions..")
    conn.close ()
    conn2.close ()

    ctx.exit ()

    # finish ctx 
    del ctx

    return True

def test_05 ():
    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    # call to create a connection
    # client_identifier = "test_05"
    # clean_session = True
    # keep_alive = 30
    conn = myqtt.Conn (ctx, host, port, "test_05", True, 30)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    if not conn.ping (10):
        error ("Expected to ping server without error, but found failure")
        return False

    return True

def test_06 ():
    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    # call to create a connection
    # client_identifier = "test_06.identifier"
    # clean_session = True
    # keep_alive = 30
    conn = myqtt.Conn (ctx, host, port, "test_06.identifier", True, 30)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    info ("Get client identifer as percieved by the server...")
    if not conn.pub ("myqtt/admin/get-client-identifier", "", 0, myqtt.qos0, False, 0):
        error ("Failed to publish message requesting client identifier..")
        return False

    info ("Getting next message reply")
    msg = conn.get_next (10000)
    if not msg:
        error ("Expected to receive message but None was found..")
        return False

    if msg.content != "test_06.identifier":
        error ("Expected to receive identifier but found: " + msg.content)
        return False

    info ("Now, trying to connect with the same identifier...")
    conn2 = myqtt.Conn (ctx, host, port, "test_06.identifier", True, 30)
    if conn2.is_ok ():
        error ("Expected to find connection failure but found it worked")
        return False

    if conn2.last_err != myqtt.CONNACK_IDENTIFIER_REJECTED:
        print "%s (%s)" % (conn2.last_err, type(conn2.last_err).__name__)
        print "%s (%s)" % (myqtt.CONNACK_IDENTIFIER_REJECTED, type (myqtt.CONNACK_IDENTIFIER_REJECTED).__name__)
        error ("Expected to find the following error %d but found %d" % (myqtt.CONNACK_IDENTIFIER_REJECTED, conn2.last_err))
        return False

    info ("Test --: now check automatic client ids..")

    conn3 = myqtt.Conn (ctx, host, port, None, True, 30)
    if not conn3.pub ("myqtt/admin/get-client-identifier", "", 0, myqtt.qos0, False, 0):
        error ("Failed to publish message requesting client identifier..")
        return False

    info ("Getting next message reply")
    msg = conn3.get_next (10000)
    if not msg:
        error ("Expected to receive message but None was found..")
        return False

    info ("Automatic identifier assigned to the connection was: " + msg.content)

    
    return True

def test_07 ():
    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    opts = myqtt.ConnOpts ()
    opts.set_auth ("aspl", "wrong-password")

    # call to create a connection
    # client_identifier = None
    # clean_session = True
    # keep_alive = 30
    conn = myqtt.Conn (ctx, host, port, None, True, 30, opts)

    # check connection status after if 
    if conn.is_ok ():
        error ("Expected to find connection ERROR for wrong auth but found it working")
        return False

    opts = myqtt.ConnOpts ()
    opts.set_auth ("aspl", "test")

    # client_identifier = None
    # clean_session = True
    # keep_alive = 30
    conn = myqtt.Conn (ctx, host, port, None, True, 30, opts)
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    if not conn.pub ("myqtt/admin/get-conn-user", "", 0, myqtt.qos0, False, 0):
        error ("Unable to send message to get current connecting user..")
        return False

    info ("Getting message with the connecting user..")
    msg = conn.get_next (10000)

    if not msg:
        error ("Expected to receive a message with the connecting user but None was received")
        return False
    
    if msg.content != "aspl":
        error ("Expected to find user aspl but found: %s" % msg.content)
        return False

    info ("Test --: auth test ok..")

    return True

def test_08_should_receive (ctx, conn, msg, data):
    info ("Test --: received message..")
    data.push (msg)
    return

def test_08_should_not_receive (ctx, conn, msg, data):
    error ("ERROR: received a message on a handler that shouldn't have been called..")
    return

def test_08 ():
    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    opts = myqtt.ConnOpts ()
    opts.set_will (myqtt.qos2, "I lost connection", "Hey I lost connection, this is my status:....", False)

    # call to create a connection
    # client_identifier = None
    # clean_session = True
    # keep_alive = 30
    conn = myqtt.Conn (ctx, host, port, None, True, 30, opts)
    if not conn.is_ok ():
        error ("Expected to find proper connection..")
        return False

    # connect without options
    conn2 = myqtt.Conn (ctx, host, port, None, True, 30)
    if not conn2.is_ok ():
        error ("Expected to find proper connection..")
        return False

    # subscripbe
    (status, sub_qos) = conn2.sub ("I lost connection", myqtt.qos0)
    if not status:
        error ("Failed to subscribe..")
        return False

    # create a third connection but without subscription
    conn3 = myqtt.Conn (ctx, host, port, None, True, 30)
    if not conn3.is_ok ():
        error ("Expected to find proper connection..")
        return False

    info ("Test --: 3 links connected, now close connection with will")
    queue = myqtt.AsyncQueue ()
    conn2.set_on_msg (test_08_should_receive, queue)
    conn3.set_on_msg (test_08_should_not_receive)

    # close the connection
    conn.shutdown ()

    # wait for message 
    info ("Test --: waiting for will... (3 seconds at most)")
    msg = queue.timedpop (3000000)
    if not msg:
        error ("ERROR: expected to receive msg reference but found NULL value..")
        return False

    if msg.topic != "I lost connection":
        error ("ERROR: expected to find topic name 'I lost connection' but found: '%s'" % msg.topic)
        return False

    if msg.content != "Hey I lost connection, this is my status:....":
        error ("ERROR: expected to find will content but found: %s" % msg.content)
        return False
    
    # no need to release queue
    # no need to release msg

    # no need to close conn
    # no need to close conn2
    # no need to close conn3

    # no need to finish ctx
    return True

def test_09 ():
    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    info ("Test 09: checking will message is not sent in the case of connection close")

    opts = myqtt.ConnOpts ()
    opts.set_will (myqtt.qos2, "I lost connection", "Hey I lost connection, this is my status:....", False)

    # call to create a connection
    # client_identifier = None
    # clean_session = True
    # keep_alive = 30
    conn = myqtt.Conn (ctx, host, port, None, True, 30, opts)
    if not conn.is_ok ():
        error ("Expected to find proper connection..")
        return False

    # connect without options
    conn2 = myqtt.Conn (ctx, host, port, None, True, 30)
    if not conn2.is_ok ():
        error ("Expected to find proper connection..")
        return False

    # subscripbe
    (status, sub_qos) = conn2.sub ("I lost connection", myqtt.qos0)
    if not status:
        error ("Failed to subscribe..")
        return False

    # create a third connection but without subscription
    conn3 = myqtt.Conn (ctx, host, port, None, True, 30)
    if not conn3.is_ok ():
        error ("Expected to find proper connection..")
        return False

    info ("Test --: 3 links connected, now close connection with will")
    queue = myqtt.AsyncQueue ()
    conn2.set_on_msg (test_08_should_receive, queue)
    conn3.set_on_msg (test_08_should_not_receive)

    # close the connection
    conn.close ()

    # wait for message 
    info ("Test --: waiting for will... (3 seconds at most)")
    msg = queue.timedpop (3000000)
    if msg:
        error ("ERROR: expected to NOT receive msg reference but found NULL value..")
        return False

    # no need to release queue

    # no need to close conn
    # no need to close conn2
    # no need to close conn3

    # no need to finish ctx
    return True

def test_10_ctx ():
    import myqtt

    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ()
        return (False, "Failed to init MyQtt context")

    return (True, ctx)

def test_10_conn (ctx):
    import myqtt
    
    # call to create a connection
    conn = myqtt.Conn (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        return (False, "Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)

    return (True, conn)

def test_10_conn_aux ():
    import myqtt

    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ()
        return (False, "Failed to init MyQtt context")

    # call to create a connection
    conn = myqtt.Conn (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        return (False, "Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)

    info ("Conn and context created, reporting..")
    return (True, conn)

def test_10 ():

    # get context
    (status, ctx) = test_10_ctx ()
    if not status:
        return (False, "Expected to find proper context but found failure...: %s" % ctx)


    # call to create a connection
    (status, conn) = test_10_conn (ctx)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    info ("MQTT connection created to: " + conn.host + ":" + conn.port) 
    
    # now close the connection
    info ("Now closing the MQTT session..")
    conn.close ()

    info ("Now checking creating a context and a connection in a different function and just reporting the connection..")
    (status, conn) = test_10_conn_aux ()
    info ("test_10_conn_aux() finished, checking result..")

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # no need to release queue

    # no need to close conn
    # no need to close conn2
    # no need to close conn3

    # no need to finish ctx
    return True

def test_11_log_handler (ctx, __file, line, log_level, msg, data):
    level_label = "-----"
    if log_level == myqtt.level_debug:
        level_label = "Debug"
    elif log_level == myqtt.level_warning:
        level_label = "Warng"
    elif log_level == myqtt.level_critical:
        level_label = "Error"

    # print "  1) %s" % level_label
    # print "  2) %s" % __file
    # print "  3) %s" % line
    # print "  4) %s" % log_level
    # print "  5) %s" % msg
    print "  Test --: %s: %s:%d %s" % (level_label, __file, line, msg)
    return

def test_11 ():

    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    # configure log handler
    ctx.set_log_handler (test_11_log_handler)

    # call to create a connection
    conn = myqtt.Conn (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    info ("MQTT connection created to: " + conn.host + ":" + conn.port) 
    
    # now close the connection
    info ("Now closing the MQTT session..")
    conn.close ()

    # force wrong connection
    conn = myqtt.Conn (ctx, "kdkjg-dgkjdg", port)

    # check connection status after if 
    if conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now close the connection
    info ("Now closing this non working connection MQTT session..")
    conn.close ()

    # no need to release queue

    # no need to close conn
    # no need to close conn2
    # no need to close conn3

    # no need to finish ctx
    return True

def test_17c_common (label, topic, msg_content, qos):
    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    info ("Test %s: QoS=%d checking big message (checking with message size: %d).." % (label, qos, len (msg_content)))

    # call to create a connection
    # client_identifier = None
    # clean_session = False
    # keep_alive = 30
    conn = myqtt.Conn (ctx, host, port, None, False, 30, None)
    if not conn.is_ok ():
        error ("Expected to find proper connection..")
        return False

    # subscripbe
    (status, sub_qos) = conn.sub (topic, qos)
    if not status:
        error ("Failed to subscribe..")
        return False

    info ("Test %s: sending message.." % label)
    queue = myqtt.AsyncQueue ()
    conn.set_on_msg (test_08_should_receive, queue)

    if not conn.pub (topic, msg_content, len (msg_content), qos, False, 10):
        error ("Unable to send message to get current connecting user..")
        return False

    # wait for message 
    info ("Test %s: waiting for message size=%d... (3 seconds at most)" % (label, len (msg_content)))
    msg = queue.timedpop (3000000)
    if not msg:
        error ("ERROR: expected to NOT receive msg reference but found NULL value..")
        return False

    info ("Test %s: received message size %d" % (label, len (msg.content)))
    if msg.content != msg_content:
        error ("ERROR: expected different message...")
        return False

    if msg.size != len (msg_content):
        error ("ERROR: expected different sizes (%d != %d)" % (msg.size, len (msg_content)))
        return False

    if msg.qos != qos:
        error ("ERROR: expected to find diffent qos=%d received=%d" % (qos, msg.qos))
        return False

    # no need to release queue
    # no need to close conn
    # no need to finish ctx
    return True

define_big_mesg = "This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 7) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346"

define_big_mesg_2 = "This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 7) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 10) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 7) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346  11) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 7) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 10) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 7) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 12)  This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 7) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 10) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 7) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346  11) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 7) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 10) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 7) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 6) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 5) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346 4) This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346....This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346...This is a retained message for future subscribers..kljsdg 90sdgrokj234 jklgo823u4t kjlwegklj23 jklegr5hu8 klj235 23rglkjry346"


def test_17c_with_qos (qos):
    if not test_17c_common ("17-c", "this/is/a/test", define_big_mesg, qos):
        return False

    if not test_17c_common ("17-c1", "this/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topic", define_big_mesg, qos):
        return False

    if not test_17c_common ("17-c2", "this/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topic", define_big_mesg, qos):
        return False

    if not test_17c_common ("17-c3", "this/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topic", define_big_mesg, qos):
        return False

    if not test_17c_common ("17-c4", "this/is/a/test", define_big_mesg_2, qos):
        return False

    if not test_17c_common ("17-c5", "this/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topic", define_big_mesg_2, qos):
        return False

    if not test_17c_common ("17-c6", "this/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topicthis/is/a/test/this-is-a-very-long-topic", define_big_mesg_2, qos):
        return False

    return True

def test_17c ():
    if not test_17c_with_qos (myqtt.qos0):
        return False

    if not test_17c_with_qos (myqtt.qos1):
        return False

    if not test_17c_with_qos (myqtt.qos2):
        return False

    return True

def test_17d_reply_get_my_products  (ctx, conn, msg, data):

    info ("Test 17-d: replying replytopic=%s" % msg.content)
    
    # get content
    content = open ("../../test/message-test-17-d.txt").read ()

    # publish reply into the topic indicated
    if not conn.pub (msg.content, content, len (content), False, 10):
        error ("Error: unable to publish message as reply for %s" % msg.content)

    return

def test_17d ():

    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    info ("Test 17-d: STEP 1: creating connection and subscribe to get the message..")

    # 
    # STEP 1: create a connection and subscribe to an especific topic 
    #


    # call to create a connection
    # client_identifier = None
    # clean_session     = True
    # keep_alive        = 30
    conn = myqtt.Conn (ctx, host, port, "test_01", True, 30, None)
    if not conn.is_ok ():
        error ("Expected to find proper connection..")
        return False

    import hashlib
    topic = "my/product/reply/%s" % hashlib.md5 ("%s" % conn).hexdigest ()

    # subscripbe
    (status, sub_qos) = conn.sub (topic, 2)
    if not status:
        error ("Failed to subscribe..")
        return False

    info ("Test 17-d: sending message..")
    queue = myqtt.AsyncQueue ()
    conn.set_on_msg (test_08_should_receive, queue)


    # 
    # STEP 2: now create a second connection that will handle
    # the reception of the message and will reply to it.
    # 

    info ("Test 17-d: STEP 2: connecting with a second connection and register a handler to reply to get/my/products")

    # call to create a connection
    # client_identifier = test_02
    # clean_session     = True
    # keep_alive        = 30
    conn2 = myqtt.Conn (ctx, host, port, "test_02", True, 30, None)
    if not conn2.is_ok ():
        error ("Expected to find proper connection..")
        return False

    # subscribe
    (status, sub_qos) = conn2.sub ("get/my/products", myqtt.qos2)
    if not status:
        error ("Failed to subscribe..")
        return False

    queue2 = myqtt.AsyncQueue ()
    conn2.set_on_msg (test_17d_reply_get_my_products, queue2)


    #
    # STEP 3: now create a third connection to publish a message requesting a reply
    # 

    info ("Test 17-d: STEP 3: connecting with a third connection and publish a get/my/products")

    # call to create a connection
    # client_identifier = test_03
    # clean_session     = True
    # keep_alive        = 30
    conn3 = myqtt.Conn (ctx, host, port, "test_03", True, 30, None)
    if not conn3.is_ok ():
        error ("Expected to find proper connection..")
        return False

    if not conn3.pub ("get/my/products", topic, len (topic), 2, False, 10):
        error ("Unable to send message to get current products")
        return False

    # 
    # STEP 4: wait for the reply
    #

    info ("Test 17-d: STEP 4: wait for reply on first queue")

    # wait for message 
    info ("Test 17-d: STEP 4: waiting for reply..")
    msg = queue.timedpop (10000000)
    if not msg:
        error ("ERROR: expected to NOT receive msg reference but found NULL value..")
        return False

    info ("Test 17-d: received message size %d" % len (msg.content))
    msg_content = open ("../../test/message-test-17-d.txt").read ()
    if msg.content != msg_content:
        error ("ERROR: expected different message...")
        return False

    info ("Test 17-d: message content ok, checking sizes.. too")

    if msg.size != len (msg_content):
        error ("ERROR: expected different sizes (%d != %d)" % (msg.size, len (msg_content)))
        return False

    # no need to release queue
    # no need to close conn
    # no need to finish ctx
    

    return True
    
def test_18 ():

    m = __import__ ("myqtt.tls")
    if not m:
        info ("Test 18: ** skipping myqtt.tls checking, module is not present .. ** ")
        return True

    import myqtt.tls

    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    info ("Test 09: checking will message is not sent in the case of connection close")

    opts = myqtt.ConnOpts ()
    myqtt.tls.ssl_peer_verify (opts, False)

    # call to create a connection
    # client_identifier = None
    # clean_session = True
    # keep_alive = 30
    conn = myqtt.tls.create_conn (ctx, host, tls_port, None, True, 30, opts)
    if not conn.is_ok ():
        error ("Expected to find proper connection (expected proper connection at test_18, it shouldn't fail)..")
        return False

    if not conn.pub ("this/is/a/test/18", "Test message 1", 14, myqtt.qos2, True, 10):
        error ("Unable to publish message..")
        return False

    if not conn.pub ("this/is/a/test/18", "Test message 2", 14, myqtt.qos2, True, 10):
        error ("Unable to publish message..")
        return False

    info ("Test --: checking connection")

    if not conn.is_ok ():
        error ("Expected being able to connect to %s:%s" % (host, tls_port))
        return False

    if not myqtt.tls.is_on (conn):
        error ("Expected to find TLS enabled connection but found it is not")
        return False

    # no need to close conn
    # no need to finish ctx
    return True

def test_19 ():

    m = __import__ ("myqtt.tls")
    if not m:
        info ("Test 19: ** skipping myqtt.tls checking, module is not present .. ** ")
        return True

    import myqtt.tls

    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    info ("Test 19: (step 1) checking will message is not sent in the case of connection close")

    opts = myqtt.ConnOpts ()
    myqtt.tls.ssl_peer_verify (opts, False)

    # configure certificates
    myqtt.tls.set_ssl_certs (opts, "../../test/client.pem", "../../test/client.pem", None, "../../test/root.pem")

    # setup a host name just to ensure handlers
    myqtt.tls.set_server_name (opts, "test19.skipsni.localhost")

    # call to create a connection
    # client_identifier = None
    # clean_session = False
    # keep_alive = 30
    conn = myqtt.tls.create_conn (ctx, host, "1911", None, False, 30, opts)
    if not conn.is_ok ():
        error ("Expected to find proper connection (first step for test_19 failed)..")
        return False

    if not conn.pub ("this/is/a/test/19", "Test message 1", 14, myqtt.qos2, True, 10):
        error ("Unable to publish message..")
        return False

    if not conn.pub ("this/is/a/test/19", "Test message 2", 14, myqtt.qos2, True, 10):
        error ("Unable to publish message..")
        return False

    info ("Test --: checking connection")

    if not conn.is_ok ():
        error ("Expected being able to connect to %s:%s" % (host, tls_port))
        return False

    if not myqtt.tls.is_on (conn):
        error ("Expected to find TLS enabled connection but found it is not")
        return False

    ### STEP 2 ###
    info ("Test 19: (step 2) attempting to connect without prividing certificates")

    opts = myqtt.ConnOpts ()
    myqtt.tls.ssl_peer_verify (opts, False)

    # call to create a connection
    # client_identifier = None
    # clean_session = False
    # keep_alive = 30
    conn = myqtt.tls.create_conn (ctx, host, "1911", None, False, 30, opts)
    if conn.is_ok ():
        error ("Expected to find connection failure connection..")
        return False

    ### STEP 3 ###
    info ("Test 19: (step 3) attempting to connect but providing certificates")
    
    opts = myqtt.ConnOpts ()

    # configure certificates
    myqtt.tls.set_ssl_certs (opts, "../../test/test-certificate.crt", "../../test/test-private.key", None, "../../test/root.pem")

    # call to create a connection
    # client_identifier = None
    # clean_session = False
    # keep_alive = 30
    conn = myqtt.tls.create_conn (ctx, host, "1911", None, False, 30, opts)
    if conn.is_ok ():
        error ("Expected to find proper connection..")
        return False

    # no need to close conn
    # no need to finish ctx
    return True

def test_22_reconnected (conn, queue):
    info ("Test 22: connection reconnected! pushing beacon: 4")
    queue.push (4)
    return

def test_22_queue_message (ctx, conn, msg, data):
    info ("Test --: received message..")
    data.push (msg)
    return

def test_22_close_recover_and_send (conn, queue, label):

    # get the socket 
    _socket = conn.socket
    info ("Test --: %s -- socket=%d received from connection-id=%d" % (label, _socket, conn.id))


    info ("Test --: %s -- closing it.." % label)
    os.close (_socket)

    info ("Test --: %s -- waiting for reconnection (10 seconds at most).." % label)
    queue.timedpop (10000000)

    if not conn.is_ok ():
        error ("Expected to find proper connectiong working after reconnect but found it failing..")
        return False

    if conn.socket <= 0:
        error ("Expected to find socket defined > 0 but found %d" % conn.socket)
        return False

    os.close (conn.socket)
    queue.timedpop (10000000)
    info ("Test --: %s -- so far, we should receive a reconnect.." % label)

    if not conn.is_ok ():
        error ("Expected to find proper connectiong working after reconnect but found it failing (2)..")
        return False

    if conn.socket <= 0:
        error ("Expected to find socket defined > 0 but found %d (2)" % conn.socket)
        return False

   #  queue.timedpop (10000000)

    info ("Test --: %s -- sending some data, socket is %d.." % (label, conn.socket))
    conn.set_on_msg (test_22_queue_message, queue)

    if not conn.pub ("myqtt/admin/get-server-name", "", 0, myqtt.qos0, False, 10):
        error ("Unable to publish message..")
        return False

    msg = queue.timedpop (10000000)
    if not msg:
        error ("ERROR: expected to find message reply...but nothing was found..")
        return False

    info ("Test --: %s -- message received received (%s, %s, socket = %d)" % (label, msg, type (msg).__name__, conn.socket))
    info ("Test --: %s -- data received, connection is working: %s" % (label, msg.content))

    return True
    

def test_22 ():

    import myqtt

    # call to initialize a context 
    ctx = myqtt.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init MyQtt context")
        return False

    info ("Test 22: Connect to the server and force disconnect to let the library reconnect..")

    opts = myqtt.ConnOpts ()
    opts.set_reconnect ()
    # opts.set_reconnect (True) # explicit reconnect
    # opts.set_reconnect (False) # disable reconnection

    queue = myqtt.AsyncQueue ()

    # call to create a connection
    # client_identifier = "test_01"
    # clean_session = False
    # keep_alive = 30
    conn = myqtt.Conn (ctx, host, port, "test_01", True, 30, opts)
    if not conn.is_ok ():
        error ("Expected being able to connect to %s:%s" % (host, tls_port))
        return False

    # configure reconnect function
    conn.set_on_reconnect (test_22_reconnected, queue)

    # do reconnect checks
    if not test_22_close_recover_and_send (conn, queue, "plain-mqtt"):
        return False

    conn.close ()

    m = __import__ ("myqtt.tls")
    if not m:
        info ("Test 22: ** skipping myqtt.tls checking, module is not present .. ** ")
        return True

    import myqtt.tls

    info ("Test 22: now check TLS connection reconnect option..")

    opts = myqtt.ConnOpts ()
    myqtt.tls.ssl_peer_verify (opts, False)
    opts.set_reconnect ()

    conn = myqtt.tls.create_conn (ctx, host, tls_port, None, False, 30, opts)
    if not conn.is_ok ():
        error ("Expected being able to connect to %s:%s" % (host, tls_port))
        return False

    # configure reconnect function
    conn.set_on_reconnect (test_22_reconnected, queue)

    # do reconnect checks
    if not test_22_close_recover_and_send (conn, queue, "mqtt-tls"):
        return False

    conn.close ()

    # no need to close conn
    # no need to finish ctx
    return True
    

###########################
# intrastructure support  #
###########################

def info (msg):
    print "[ INFO  ] : " + msg

def error (msg):
    print "[ ERROR ] : " + msg

def ok (msg):
    print "[  OK   ] : " + msg

def run_all_tests ():
    test_count = 0
    for test in tests:
        
         # print log
        info ("TEST-" + str(test_count) + ": Running " + test[1])
        
        # call test
        if not test[0]():
            error ("detected test failure at: " + test[1])
            return False

        # next test
        test_count += 1
    
    ok ("All tests ok!")
    return True

# declare list of tests available
tests = [
   (test_00_a, "Check PyMyQtt async queue wrapper"),
   (test_01,   "Check PyMyQtt context initialization"),
   (test_02,   "Check PyMyQtt basic MQTT connection"),
   (test_03,   "Check PyMyQtt basic MQTT connection and subscription"),
   (test_04,   "Check PyMyQtt basic subscribe function (QOS 0) and publish"),
   (test_05,   "Check PyMyQtt check ping server (PINGREQ)"),
   (test_06,   "Check PyMyQtt check client identifier function"),
   (test_07,   "Check PyMyqtt client auth (CONNECT simple auth)"),
   (test_08,   "Check PyMyqtt test will support (without auth)"),
   (test_09,   "Check PyMyqtt test will is not published with disconnect (without auth)"),
   (test_10,   "Check PyMyqtt automatic reference collection"),
   (test_11,   "Check PyMyqtt set log handler "),
   (test_17c,  "Check PyMyqtt big message support"),
   (test_17d,  "Check PyMyqtt different exchanges with QoS 2 messages"),
   # tls support
   (test_18,   "Check PyMyqtt test TLS support"),
   (test_19,   "Check PyMyqtt TLS support (server side certificate auth: common CA)"),
   # reconnect support
   (test_22,   "Check PyMyqtt support for automatic-transparent reconnect"),
]

# declare default host and port
host     = "localhost"
port     = "34010"
tls_port = "34011"

if __name__ == '__main__':
    iterator = 0
    for arg in sys.argv:
        # according to the argument position, take the value 
        if iterator == 1:
            host = arg
        elif iterator == 2:
            port = arg
            
        # next iterator
        iterator += 1

    # drop a log
    info ("Running tests against " + host + ":" + port)
    info ("For valgrind, run with: libtool --mode=execute valgrind --leak-check=yes  --suppressions=suppressions.valgrind  ./myqtt-regression-client.py")

    # call to run all tests
    run_all_tests ()




