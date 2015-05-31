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
#   (test_00_a, "Check PyMyQtt async queue wrapper"),
#   (test_01,   "Check PyMyQtt context initialization"),
#   (test_02,   "Check PyMyQtt basic MQTT connection"),
#   (test_03,   "Check PyMyQtt basic MQTT connection and subscription"),
#   (test_04,   "Check PyMyQtt basic subscribe function (QOS 0) and publish"),
#   (test_05,   "Check PyMyQtt check ping server (PINGREQ)"),
#   (test_06,   "Check PyMyQtt check client identifier function"),
   (test_07,   "Check PyMyqtt client auth (CONNECT simple auth)")
]

# declare default host and port
host     = "localhost"
port     = "34010"

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




