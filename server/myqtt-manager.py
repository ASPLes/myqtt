#!/usr/bin/python
# coding: utf8
#
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

import commands
import sys
import os
import axl

# command line arguments
from optparse import OptionParser

easy_config_modes = {
    "check" : "Does no change but ensures that the configurations is right, creating missing users, missing directories, etc",
    "anonymous-home" : "Allows to reconfigure MyQttD server for a home/office solution without any security so all connections are accepted. It is good for easy configuration but NOT RECOMMENDED if security is something important for you"
}

show_debug = False

def dbg (message):
    if not show_debug:
        return

    print "  - [ myqtt-manager ] %s" % message
    return

def run_cmd (cmd):
    dbg (cmd)
    return commands.getstatusoutput (cmd)

def get_os ():
    """
    Allows to detect what OS is running the current host and which version.

    The function returns (os_name, os_long_name, os_version) where osname is the OS name
    and version is the OS revision.
    """
    import os

    # this must be this place 
    if os.path.exists ("/etc/debian_version") and not os.path.exists ("/etc/lsb-release"):
        version          = open ("/etc/debian_version").read ().strip ()

        # configure long description
        long_description = "Debian GNU/Linux"
        if os.path.exists ("/etc/os-release"):
            if "Raspbian GNU/Linux" in open ("/etc/os-release").read ():
                long_description = "Raspbian GNU/Linux"
            # end if
        # end if
        
        if version[0] == "5":
            version = "%s lenny" % version
        elif version[0] == "6":
            version = "%s squeeze" % version
        elif version[0] == "7":
            version = "%s wheezy" % version
        elif version[0] == "8":
            version = "%s jessie" % version
        
        return ("debian", long_description, version)

    # very important: this must be this place
    if os.path.exists ("/etc/redhat-release"):
        full_release = open ("/etc/redhat-release").read ().strip ()
        version      = full_release.split (" ")[2]
        for item in full_release.split (" "):
            if item[0].isdigit ():
                version = item
                break
            # end if
        # end for
        majornum     = version[0]

        if "CentOS" in full_release:
            return ("centos", full_release, "%s centos%s" % (version, majornum))
        
        # its a kind of redhat distribution if not redhat but still
        # not supported.

    if os.path.exists ("/etc/lsb-release"):
        # get the whole content
        content = open ("/etc/lsb-release").read ()

        # get version (this is generic)
        version = content.split ("DISTRIB_RELEASE=")[1].split ("\n")[0]

        # get description (this is generic)
        description = content.split ("DISTRIB_DESCRIPTION=")[1].split ("\n")[0].replace ('"', "")

        # get distribution name (this is generic)
        distribution_id = content.split ("DISTRIB_ID=")[1].split ("\n")[0].lower ()

        # try to personalize a bit (adding version)
        if "Ubuntu 12.04" in content and "precise" in content:
            version = "%s precise" % version

        if "Ubuntu 16.04" in content and "xenial" in content:
            version = "%s xenial" % version

        if "Linux Mint 13 Maya" in content:
            version = "%s maya" % version

        # report version
        return (distribution_id, description, version)

    # return known version
    return ("unknown", "Unknown OS found", "0.0 unknown")




def install_arguments():
    # start parser
    parser = OptionParser()

    # add easy config option
    easy_config_keys = ",".join (easy_config_modes)
    help_message     = "Easy config options are a standard and easy way to (re)configure your MyQttd server in a quick and controlled way. Currently we support the following modes: %s Use -x for additional information" % easy_config_keys
    parser.add_option("-e", "--easy-config", dest="easy_config", metavar="INSTALL-OPTION",
                      help=help_message)
    parser.add_option("-y", "--assume-yes", dest="assume_yes", action="store_true", default=False,
                      help="Signal the tool to assume 'yes' response to all security and warning questions")
    parser.add_option("-d", "--debug", dest="show_some_debug", action="store_true", default=False,
                      help="Allows to make the tool to show some debug while operating")
    parser.add_option ("-x", "--explain-easy-config", dest="explain_easy_config", action="store_true", default=False)
    
    # parse options received
    (options, args) = parser.parse_args ()

    # enable debug
    if options.show_some_debug:
        global show_debug
        show_debug = True
    # end if

    # return options and arguments
    return (options, args)

def get_conf_location ():
    (status, info) = run_cmd ("myqttd --conf-location | grep SYSCONFDIR")
    if status:
        return (False, "ERROR: unable to find config location, error was: %s" % info)

    if info:
        # report conf location
        conf_location = "%s/myqtt/myqtt.conf" % info.split (":")[1].strip ()
        dbg ("reporting conf location: %s" % conf_location)
        return (True, conf_location)

    return (True, "/etc/myqtt/myqtt.conf") # default location

def ensure_myqtt_conf_in_place ():

    # get configuration location
    (status, conf_location) = get_conf_location ()
    if not status:
        return (False, conf_location)
    
    if os.path.exists (conf_location):
        return (True, None) # file in place

    # create example location
    dbg ("found myqtt.conf is not there, creating from example..") 
    example_conf = conf_location.replace ("myqtt.conf", "myqtt.example.conf")
    (status, info) = run_cmd ("cp %s %s" % (example_conf, conf_location))
    if status:
        return (False, "ERROR: failed to copy example configuration, error code=%d, unable to continue, output was: %s" % (status, info))

    # report config in place
    return (True, None)

def get_configuration_doc ():
    # get configuration location
    (status, conf_location) = get_conf_location ()
    if not status:
        return (False, "Unable to ensure user, failed to get config location: %s" % conf_location, None)

    # parse configuration file
    (doc, err) = axl.file_parse (conf_location)
    if not doc:
        return (False, "Unable to parse configuration located at %s, error was: %s. Please review, fix it and then rerun the tool." % (conf_location, err.msg), None)

    return (True, doc, conf_location)

def ensure_myqtt_user_exists ():

    # get configuration location
    (status, doc, conf_location) = get_configuration_doc ()
    if not status:
        return (False, "Unable to ensure user exists, error was: %s" % doc)

    # now locate running user configuration
    node = doc.get ("/myqtt/global-settings/running-user")
    if not node:
        # configuration not found, add defaults
        node = axl.Node ("running-user")
        node.attr ("uid", "myqttd")
        node.attr ("gid", "myqttd")

        # add child
        doc.get ("/myqtt/global-settings").set_child (node)

        # save configuration file
        doc.file_dump (conf_location, 4)

    # end if

    # now ensure user exists
    uid_user = node.attr ("uid")
    (status, info) = run_cmd ("id %s" % uid_user)
    if status:
        # user does not exists, create it
        (osname, oslongname, osversion) = get_os ()
        if osname in ["debian", "ubuntu", "linuxmint"]:
            cmd = "adduser --gecos 'MyQttD user' --disabled-login --disabled-password --no-create-home --force-badname %s" % uid_user
        elif osname in ["centos", "redhat"]:
            cmd = "adduser --comment 'MyQttD user' -s /bin/false -M %s" % uid_user

        # call to create user
        (status, info) = run_cmd (cmd)
        if status:
            return (False, "Failed to create user %s with command %s, error code reported=%d, output: %s" % (uid_user, cmd, status, info))
        # end if

    # end if

    # now ensure group exists
    gid_group = node.attr ("gid")
    # FIXME :-)

    return (True, "%s:%s" % (uid_user, gid_group))

def myqtt_restart_service ():
    cmd = "service myqtt restart"
    (status, info) = run_cmd (cmd)
    if status:
        return (False, "MyQttD restart failed, error was: %s" % info)

    # service restarted
    return (True, None)


def get_runtime_datadir ():
    (status, info) = run_cmd ("myqttd --conf-location | grep RUNTIME_DATADIR")
    if status:
        return (False, "ERROR: unable to find config location, error was: %s" % info)

    if info:
        return (True, info.split (":")[1].strip ())

    return (True, "/var/lib/myqtt") # default location

def ensure_myqtt_directories ():

    # get run time location
    (status, run_time_location) = get_runtime_datadir ()
    if not status:
        return (False, "Unable to find run time location, error was: %s" % run_time_location)

    items = run_time_location.split ("/")
    if len (items) < 4:
        return (False, "Run time configuration is small than 4 directories (%s), is not secure to continue" % items)
    
    if not os.path.exists (run_time_location):
        # create runtime location
        (status, info) = run_cmd ("mkdir -p %s" % run_time_location)
        if status:
            return (False, "Unable to create directory %s, error was: %s, exit code=%d" % (run_time_location, status, info))

    # ensure permissions
    (status, user_group) = ensure_myqtt_user_exists ()
    if not status:
        return (False, "Failed to assign permissions, failed to get myqtt user, error was: %s" % user_group)
    cmd = "chown -R %s %s" % (user_group, run_time_location)
    dbg ("RUNNING: %s" % cmd)
    (status, info) = run_cmd (cmd)
    if status:
        return (False, "Unable to change permissions to %s directory, with command  %s, exit code=%d, output=%s" % (run_time_location, cmd, status, info))

    # create dbs
    myqtt_dbs = run_time_location.replace ("/myqtt", "/myqtt-dbs")
    if not os.path.exists (myqtt_dbs):
        # create runtime location
        (status, info) = run_cmd ("mkdir -p %s" % myqtt_dbs)
        if status:
            return (False, "Unable to create directory %s, error was: %s, exit code=%d" % (myqtt_dbs, status, info))

    # ensure permissions
    cmd = "chown -R %s %s" % (user_group, myqtt_dbs)
    dbg ("RUNNING: %s" % cmd)
    (status, info) = run_cmd (cmd)
    if status:
        return (False, "Unable to change permissions to %s directory, with command  %s, exit code=%d, output=%s" % (myqtt_dbs, cmd, status, info))

    # get configuration location
    (status, conf_location) = get_conf_location ()
    if not status:
        return (False, conf_location)

    # get myqtt conf dir
    myqtt_conf_dir = os.path.dirname (conf_location)

    # ensure permissions
    cmd = "chown -R %s %s" % (user_group, myqtt_conf_dir)
    dbg ("RUNNING: %s" % cmd)
    (status, info) = run_cmd (cmd)
    if status:
        return (False, "Unable to change permissions to %s directory, with command  %s, exit code=%d, output=%s" % (myqtt_conf_dir, cmd, status, info))

    # remove permissions to ensure security
    cmd ="chmod go-rwx %s %s %s" % (myqtt_conf_dir, run_time_location, myqtt_dbs)
    (status, info) = run_cmd (cmd)
    if status:
        return (False, "Unable to change permissions to with command  %s, exit code=%d, output=%s" % (cmd, status, info))
    
    # directories in place
    return (True, None)

def enable_mod (mod_name):

    # get configuration location
    (status, conf_location) = get_conf_location ()
    if not status:
        return (False, conf_location)

    # mod dir
    mod_enabled   = "%s/mods-enabled" % os.path.dirname (conf_location)
    mod_available = "%s/mods-available" % os.path.dirname (conf_location)

    # mod name
    mod_name_file = "%s/%s.xml" % (mod_enabled, mod_name)
    
    if os.path.exists (mod_name_file):
        dbg ("Module %s already enabled" % mod_name)
        return (True, None)

    # linking
    cmd = "ln -s %s/%s.xml %s" % (mod_available, mod_name, mod_name_file)
    dbg ("Enabling module: %s" % cmd)
    (status, info) = run_cmd (cmd)
    if status:
        return (False, "Failed to link module, error was: %s" % info)
    
    return (True, None)
    
def disable_mod (mod_name):

    # get configuration location
    (status, conf_location) = get_conf_location ()
    if not status:
        return (False, conf_location)

    # mod dir
    mod_enabled   = "%s/mods-enabled" % os.path.dirname (conf_location)

    # mod name
    mod_name = "%s/%s.xml" % (mod_enabled, mod_name)
    
    if not os.path.exists (mod_name):
        dbg ("Module %s already disabled" % mod_name)
        return (True, None) # nothing to do

    # linking
    cmd = "rm -f %s/%s.xml" % (mod_enabled, mod_name)
    dbg ("Disabling module: %s" % cmd)
    (status, info) = run_cmd (cmd)
    if status:
        return (False, "Failed to link module, error was: %s" % info)
    
    return (True, None)
    
def reconfigure_myqtt_anonymous_home ():

    # ensure myqtt.conf is in place
    (status, info) = ensure_myqtt_conf_in_place ()
    if not status:
        return (False, info)

    # ensure system user exists
    (status, user_group) = ensure_myqtt_user_exists ()
    if not status:
        return (False, user_group)

    # ensure known directories
    (status, info) = ensure_myqtt_directories ()
    if not status:
        return (False, info)

    # disable-empty all domains
    # get configuration location
    (status, doc, conf_location) = get_configuration_doc ()
    if not status:
        return (False, "Unable to ensure user exists, error was: %s" % doc)

    # get first node inside myqtt domains node
    myqtt_domains = doc.get ("/myqtt/myqtt-domains")
    if not myqtt_domains:
        return (False, "Unable to find <myqtt-domains> node inside /myqtt xml-path")

    # find and erase all configuration
    node = myqtt_domains.first_child
    while node:

        # remove node
        node.remove ()
        
        # next node
        node = myqtt_domains.first_child
    # end while

    # get run time location
    (status, run_time_location) = get_runtime_datadir ()
    if not status:
        return (False, "Unable to find run time location, error was: %s" % run_time_location)

    # now add anonymous declaration
    node = axl.Node ("domain")
    node.attr ("name", "anonymous")
    
    storage_dir  = "%s/anonymous" % run_time_location
    node.attr ("storage", storage_dir)
    
    users_db_dir = "%s/anonymous" % run_time_location.replace ("myqtt", "myqtt-dbs")
    node.attr ("users-db", users_db_dir)
    
    node.attr ("use-settings", "no-limits")
    node.attr ("is-active", "yes")

    # add child to domains configuration
    myqtt_domains.set_child (node)

    # create some directories
    if not os.path.exists (storage_dir):
        (status, info) = run_cmd ("mkdir -p %s" % storage_dir)
        if status:
            return (False, "Unable to create directory, error was: %s" % info)
    # end if

    # create some directories
    if not os.path.exists (users_db_dir):
        (status, info) = run_cmd ("mkdir -p %s" % users_db_dir)
        if status:
            return (False, "Unable to create directory, error was: %s" % info)

    # create users.xml file
    open ("%s/users.xml" % users_db_dir, "w").write ('<myqtt-users anonymous="yes" />')
    cmd = "chown -R %s %s %s" % (user_group, users_db_dir, storage_dir)
    dbg ("Running: %s" % cmd)
    (status, info) = run_cmd (cmd)
    if status:
        return (False, "Unable to change directory owner, error was: %s" % info)
    # end if

    # save configuration file
    doc.file_dump (conf_location, 4)

    # enable mod-auth-xml
    (status, info) = enable_mod ("mod-auth-xml")
    if not status:
        return (False, "Unable to activate configuration, failed to enable module: %s" % info)
            
    (status, info) = disable_mod ("mod-auth-mysql")
    if not status:
        return (False, "Unable to disable configuration, failed to disable module: %s" % info)

    # restart service
    (status, info) = myqtt_restart_service ()
    if not status:
        return (False, "Failed to restart service after configuration, error was: %s" % info)
    
    return (True, "Anonymous-home configuration done")

def reconfigure_myqtt_easy (options, args):

    if not options.assume_yes:
        print "The following will remove your existing configuration, replacing it with a new one"
        accept = raw_input ("Do you want to continue? (y/N)")
        if not accept or accept.lower () != "y":
            return (False, "ERROR: not accepted, leaving configuration untouched")
        # end if
    # end if
    
    # check easy configuration is accepted
    if options.easy_config not in easy_config_modes.keys ():
        return (False, "Unable to configure server as requested, provided an easy to config option that is not supported: %s" % options.easy_config)

    # according to the configuration do some setup
    if options.easy_config == "anonymous-home":
        return reconfigure_myqtt_anonymous_home ()

    

    # report caller we have received an option that is not supported
    return (False, "Received an option that is not supported: %s" % options.easy_config)


### MAIN ###
if __name__ == "__main__":
    # install arguments and process them
    (options, args) = install_arguments ()

    if options.explain_easy_config:
        for key in easy_config_modes:
            print "- %s : %s" % (key, easy_config_modes[key])
        sys.exit (0)
            
    elif options.easy_config:
        # call to reconfigure
        (status, info) = reconfigure_myqtt_easy (options, args)
        if not status:
            print "ERROR: %s" % info
            sys.exit (-1)
            
        # reached this point, finish
        print "INFO: easy reconfigure done (%s) -- %s" % (options.easy_config, info)
        sys.exit (0)


