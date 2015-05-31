# only import base library
from libpy_myqtt_10 import *

# some constants used by the library
qos0 = 0
qos1 = 1
qos2 = 2

# message types
CONNECT      = 1
CONNACK      = 2
PUBLISH      = 3
PUBACK       = 4
PUBREC       = 5
PUBREL       = 6
PUBCOMP      = 7
SUBSCRIBE    = 8
SUBACK       = 9
UNSUBSCRIBE  = 10
UNSUBACK     = 11
PINGREG      = 12
PINGRESP     = 13
DISCONNECT   = 14

# MyQttPublishCodes : Return codes for on_publish (ctx.set_on_publish) handler
PUBLISH_OK         = 1
PUBLISH_DISCARD    = 2
PUBLISH_CONN_CLOSE = 3

# MyQttConnAckTypes : Error codes reported by conn.last_err
CONNACK_UNKNOWN_ERR                 = -4
CONNACK_CONNECT_TIMEOUT             = -3
CONNACK_UNABLE_TO_CONNECT           = -2
CONNACK_DEFERRED                    = -1
CONNACK_ACCEPTED                    = 0
CONNACK_REFUSED                     = 1
CONNACK_IDENTIFIER_REJECTED         = 2
CONNACK_SERVER_UNAVAILABLE          = 3
CONNACK_BAD_USERNAME_OR_PASSWORD    = 4
CONNACK_NOT_AUTHORIZED              = 5






