:mod:`myqtt.Msg` --- PyMyQttMsg class: A MQTT message received
==============================================================

.. currentmodule:: myqtt


API documentation for myqtt.Msg object represents a single MQTT
message received

==========
Module API
==========

.. class:: Msg

   .. attribute:: id

      (Read only attribute) (Number) returns frame unique identifier

   .. attribute:: type

      (Read only attribute) (String) returns the msg type (see
      myqtt_msg_get_type_str documentation)

   .. attribute:: payload

      (Read only attribute) (String) returns the raw payload
      associated to the given msg (variable header + payload).

   .. attribute:: payload_size

      (Read only attribute) (Number) returns the raw payload size

   .. attribute:: content

      (Read only attribute) (String) returns just application
      message. See myqtt_msg_get_app_msg

   .. attribute:: content_size

      (Read only attribute) (Number) returns the content size
      (application's message size). See myqtt_msg_get_app_msg_size

   .. attribute:: topic

      (Read only attribute) (String) returns the topic of the message
      (if defined)

   .. attribute:: qos

      (Read only attribute) (Number) returns the qos of the message
