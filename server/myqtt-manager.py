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
    "anonymous-home" : "Allows to reconfigure MyQttD server for a home/office solution without any security so all connections are accepted. It is good for easy configuration but NOT RECOMMENDED if security is something important for you",
    "auth-xml" : "Allows to reconfigure MyQttD server for a home/office solution with connection authentication enabled to accept connection. A good choice in the case you want accept MQTT requests but enforcing connections to be authenticated. This setup also allows MQTT virtual-hosting by grupping users under domains with topic isolation. You can use this option for one group of users or multiple"
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
    parser.add_option ("-e", "--easy-config", dest="easy_config", metavar="INSTALL-OPTION",
                      help=help_message)
    parser.add_option ("-x", "--explain-easy-config", dest="explain_easy_config", action="store_true", default=False)

    # run time options
    parser.add_option ("-d", "--debug", dest="show_some_debug", action="store_true", default=False,
                      help="Allows to make the tool to show some debug while operating")
    parser.add_option ("-y", "--assume-yes", dest="assume_yes", action="store_true", default=False,
                      help="Signal the tool to assume 'yes' response to all security and warning questions")

    # module management
    parser.add_option ("-i", "--enable-mod", dest="enable_mod", metavar="MOD",
                      help="Allows to enable a given module (-i 'mod in')")
    parser.add_option ("-o", "--disable-mod", dest="disable_mod", metavar="MOD",
                      help="Allows to disable a given module (-o 'mod out')")
    parser.add_option ("-m", "--list-modules", dest="list_modules", action="store_true", default=False,
                      help="Allows to list all modules available")

    # domain management
    parser.add_option ("-n", "--list-domains", dest="list_domains", action="store_true", default=False,
                       help="Allows to list all domains installed at this point and linked to the configuration")
    parser.add_option ("-g", "--create-domain", dest="create_domain", metavar="domain_name",
                       help="Allows to create a domain using current authentication backend configured. If no backend is configured, it will create default structures")

    # account management
    parser.add_option ("-a", "--add-account", dest="add_account", metavar="client-id[,username,password] domain",
                       help="Allows to add the provided client-id account, optionally providing a username and password into the provided domain.")
    parser.add_option ("-t", "--set-account-password", dest="set_account_password", metavar="client-id[,username],password domain",
                       help="Allows to set/change username/password  to the provided client-id account.")
    parser.add_option ("-r", "--remove-account", dest="remove_account", metavar="client_id domain",
                       help="Allows to remove the provided client-id account from the given domain.")
    
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

    # dump current configuration
    import hashlib
    import random
    import time
    temp_file_name = "/tmp/%s" % hashlib.md5 ("%s/%s" % (random.randint (1, 100000), time.time ())).hexdigest ()
    (status, info) = run_cmd ("myqttd --show-config > %s" % temp_file_name)
    if status:
        return (False, "Unable to get configuration, error was: %s" % info)

    # parse configuration file
    (doc, err) = axl.file_parse (temp_file_name)

    # remove file before checking anything
    if os.path.exists (temp_file_name):
        os.unlink (temp_file_name)
        
    # check output parsed
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

def myqtt_reload_service ():
    cmd = "service myqtt reload"
    (status, info) = run_cmd (cmd)
    if status:
        return (False, "MyQttD reload failed, error was: %s" % info)

    # service reload
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
    cmd = "rm -f %s" % mod_name
    dbg ("Disabling module: %s" % cmd)
    (status, info) = run_cmd (cmd)
    if status:
        return (False, "Failed to link module, error was: %s" % info)
    
    return (True, None)

def reconfigure_myqtt_anonymous_home (options, args):

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

def check_myqtt_config ():

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

    # restart service
    (status, info) = myqtt_restart_service ()
    if not status:
        return (False, "Failed to restart service after configuration, error was: %s" % info)
    
    return (True, "MyQtt configuration check done")

def reconfigure_myqtt_auth_xml (options, args):

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

    # ensure domains.d directory exists
    conf_dir = os.path.dirname (conf_location)
    if not os.path.exists ("%s/domains.d" % conf_dir):
        # create directory
        (status, info) = run_cmd ("mkdir -p %s/domains.d" % conf_dir)
        if status:
            return (False, "Unable to create directory, error was: %s" % info)
        # ensure permissions
        (status, info) = run_cmd ("chown -R %s %s/domains.d" % (user_group, conf_dir))
        if status:
            return (False, "Unable to fix directory owners, error was: %s" % info)
        # end if
    # end if

    # enable mod-auth-xml
    (status, info) = enable_mod ("mod-auth-xml")
    if not status:
        return (False, "Unable to activate configuration, failed to enable module: %s" % info)

    # disable myqtt module
    (status, info) = disable_mod ("mod-auth-mysql")
    if not status:
        return (False, "Unable to disable configuration, failed to disable module: %s" % info)

    # add include node
    node = axl.Node ("include")
    node.attr ("dir", "/etc/myqtt/domains.d/")
    myqtt_domains.set_child (node)

    # save configuration file
    doc.file_dump (conf_location, 4)

    # restart service
    (status, info) = myqtt_restart_service ()
    if not status:
        return (False, "Failed to restart service after configuration, error was: %s" % info)
    

    return (True, "MyQtt mod-auth-xml backend configuration done")

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
        return reconfigure_myqtt_anonymous_home (options, args)
    elif options.easy_config == "check":
        return check_myqtt_config ()
    elif options.easy_config == "auth-xml":
        return reconfigure_myqtt_auth_xml (options, args)

    

    # report caller we have received an option that is not supported
    return (False, "Received an option that is not supported: %s" % options.easy_config)

def list_modules ():
    # get configuration location
    (status, conf_location) = get_conf_location ()
    if not status:
        return (False, "Failed to list modules, unable to get conf location: %s" % conf_location)

    import os
    result = []
    mod_dir = "%s/mods-available" % os.path.dirname (conf_location)
    dbg ("Listing modules found at: %s" % mod_dir)
    modules = os.listdir (mod_dir)
    for mod in modules:
        if mod:
            mod = mod.strip ()
        if not mod:
            continue

        mod = mod.replace (".xml", "")
        result.append (mod)
    # end for

    # report modules found
    return (True, result)

def is_mod_enabled (mod):

    # get configuration location
    (status, conf_location) = get_conf_location ()
    if not status:
        return False

    import os
    mod_enabled_xml = "%s/mods-enabled/%s.xml" % (os.path.dirname (conf_location), mod)
    return os.path.exists (mod_enabled_xml)

def list_domains (options, args):

    # load and get configuration
    (status, doc, conf_location) = get_configuration_doc ()
    if not status:
        return (False, doc)

    # get first domain and iterate over all of them
    result = []
    domain = doc.get ("/myqtt/myqtt-domains/domain")
    while domain:

        if "skeleton-dont-use-please" in domain.attr ("name"):
            # get next domain 
            domain = domain.next_called ("domain")
            
            continue

        item = {}
        item['name']         = domain.attr ("name")
        item['storage']      = domain.attr ("storage")
        item['users_db']     = domain.attr ("users-db")
        item['use_settings'] = domain.attr ("use-settings")
        item['is_active']    = True
        if domain.has_attr ("is-active") and domain.attr ("is-active") in ["no", "0", "false"]:
            item['is_active'] = False

        # add item to the result
        result.append (item)

        # get next domain 
        domain = domain.next_called ("domain")
        
    # end while

    # report 
    return (True, result)

def is_single_label_domain (label):
    """
    Allows to check if the label provided represents a valid host name
    value. For example srv-app, localdomain and any other single
    domain name value (no .).
    """

    # do quick check
    if "." in label:
        return False

    import re
    return not re.match ("^[a-z0-9A-Z-_]+$", label) is None


def is_domain (domain_name):
    """
    Allows to check if the provided name is a domain valid name (like
    dominio.com or value.dominio.com).

    The function returns True in the case it is a valid domain name,
    otherwise False is returned.
    """
    if domain_name is None or len (domain_name) == 0:
        return False
    
    value = str (domain_name)
    if value[0] == "@":
        value = value[1:]
    
    import re
    return not re.match ("^([a-z0-9A-Z-_]+\.)+[a-zA-Z]+$", value) is None

def mod_auth_mysql_add_domain (domain_name, options, args):

    return (False, "Still not implemented (mod-auth-mysql)")

def create_domain (options, args):

    # domain name
    domain_name = options.create_domain
    if domain_name:
        domain_name = domain_name.strip ()
    if not domain_name:
        return (False, "Domain name is not defined")

    # check domain name has a valid domain name value
    if not is_domain (domain_name) and not is_single_label_domain (domain_name):
        return (False, "Received a domain value %s that is not a valid domain name. It has to be something similar to mqtt.domain.com or mqtt-server" % domain_name)

    if domain_exists (domain_name):
        return (False, "Unable to create domain %s, it already exists" % domain_name)
    # end for

    # get run time location
    (status, run_time_location) = get_runtime_datadir ()
    if not status:
        return (False, "Unable to find run time location, error was: %s" % run_time_location)

    # get configuration location
    (status, conf_location) = get_conf_location ()
    if not status:
        return (False, conf_location)

    # ensure system user exists
    (status, user_group) = ensure_myqtt_user_exists ()
    if not status:
        return (False, user_group)

    # ensure domains.d directory exists
    conf_dir = os.path.dirname (conf_location)
    conf_dir = "%s/domains.d" % conf_dir
    if not os.path.exists (conf_dir):
        return (False, "Unable to continue, directory %s does not exists. Is this MyQtt server configured by myqtt-manager.py? If not, this option will not work" % conf_dir)
    # end if

    # run time location
    dbs_dir = run_time_location.replace ("/myqtt", "/myqtt-dbs")

    # create domain inside configuration
    full_run_time_location = "%s/%s" % (run_time_location, domain_name)
    full_dbs_dir           = "%s/%s" % (dbs_dir , domain_name)
    file_content           = "<domain name='%s' storage='%s' users-db='%s' use-settings='no-limits' />\n" % (domain_name, full_run_time_location, full_dbs_dir)
    open ("%s/%s.conf" % (conf_dir, domain_name), "w").write (file_content)

    # now create directories associated to this domain name
    if not os.path.exists (full_run_time_location):
        (status, info) = run_cmd ("mkdir -p %s" % full_run_time_location)
        if status:
            return (False, "Unable to create directory, error was: %s" % info)
        # end if
    # end if

    if not os.path.exists (full_dbs_dir):
        (status, info) = run_cmd ("mkdir -p %s" % full_dbs_dir)
        if status:
            return (False, "Unable to create directory, error was: %s" % info)
        # end if
    # end if

    # check what backend is being used
    mod_auth_xml   = is_mod_enabled ("mod-auth-xml")
    mod_auth_mysql = is_mod_enabled ("mod-auth-mysql")

    if mod_auth_xml:
        # now create initial users.xml file
        users_xml = "%s/users.xml" % full_dbs_dir
        if not os.path.exists (users_xml):
            # setup default password format sha1
            open (users_xml, "w").write ("<myqtt-users password-format='sha1' />")
        # end if
    # end if

    # ensurep permissions
    (status, info) = run_cmd ("chown -R %s %s %s" % (user_group, full_run_time_location, full_dbs_dir))
    if status:
        return (False, "Unable to change directory permissions, error was: %s" % info)
    # end if

    # if mysql backend enabled, add domain into it
    if mod_auth_mysql:
        (status, info) = mod_auth_mysql_add_domain (domain_name, options, args)
        if not status:
            return (False, "Unable to add domain to MySQL backend, error was: %s" % info)
        # end if
    # end if

    # reload myqtt to have domain loaded
    (status, info) = myqtt_reload_service ()
    if not status:
        # report error found after reloading
        return (False, "Domain %s was added but it failed to reload. You will have to restart myqtt" % domain_name)
    
    return (True, "Domain %s created" % domain_name)

def format_password (password, password_format):
    import hashlib
    
    # get md5 password
    if password_format == 0:
        return (True, password)
    elif password_format == 1:
        value = hashlib.md5 (password).hexdigest ().upper()
    elif password_format == 2:
        value = hashlib.sha (password).hexdigest ().upper()
    else:
        return (False, "ERROR: unsupported hash: " + password_format)
        
    # now we have to insert : every two chars
    iterator = 2
    
    # hash len is the current length of the hash + every : placed. Because
    # it is placed every two positions we multiply the length by 1,5 (-1)
    # to avoid adding : at the end.
    hash_len = len (value) * 1.5 -1
    while iterator < hash_len:
        value = value[:iterator] + ":" + value[iterator:]
        iterator += 3
    # end while

    return (True, value)
    

def add_account_mod_auth_xml (client_id, username, password, domain_name, users_xml):

    # open document
    (doc, err) = axl.file_parse (users_xml)
    if not doc:
        return (False, "Unable to add account, axl.file_parse (%s) failed, error was: %s" % (users_xml, err.msg))
    
    # get password format
    node = doc.get ("/myqtt-users")
    if not node:
        return (False, "Unknown format found at %s. Expected to find <myqtt-users> node at the top" % users_xml)

    if password:
        password_format = 0
        if node.has_attr ("password-format"):
            value = node.attr ("password-format")
            if value == "plain":
                password_format = 0
            elif value == "md5":
                password_format = 1
            elif value == "sha1":
                password_format = 2
            # end if
        # end if

        (status, password) = format_password (password, password_format)
        if not status:
            return (False, "Unable to format password, error was: %s" % password)
        # end if
    # end if

    # check if the clientid is already added
    node = doc.get ("/myqtt-users/user")
    while node:
        if node.attr ("id") == client_id:
            return (False, "Unable to add client-id %s into domain %s (%s). It already exists" % (client_id, domain_name, users_xml))
        # end if
        
        # get next <user /> node 
        node = node.next_called ("user")
    # end while

    user = axl.Node ("user")
    user.attr ("id", client_id)
    if username:
        user.attr ("username", username)
    # end if
    if password:
        user.attr ("password", password)
    # end if

    # set node into the document
    node = doc.get ("/myqtt-users")
    node.set_child (user)

    # save configuration file
    doc.file_dump (users_xml, 4)

    # restart myqtt server
    (status, info) = myqtt_restart_service ()
    if not status:
        return (False, "User was added but failed to restart myqtt service, error was: %s.." % info)
    # end if

    return (True, "User added")

def update_account_mod_auth_xml (client_id, username, password, domain_name, users_xml):

    # open document
    (doc, err) = axl.file_parse (users_xml)
    if not doc:
        return (False, "Unable to add account, axl.file_parse (%s) failed, error was: %s" % (users_xml, err.msg))
    
    # get password format
    node = doc.get ("/myqtt-users")
    if not node:
        return (False, "Unknown format found at %s. Expected to find <myqtt-users> node at the top" % users_xml)

    if password:
        password_format = 0
        if node.has_attr ("password-format"):
            value = node.attr ("password-format")
            if value == "plain":
                password_format = 0
            elif value == "md5":
                password_format = 1
            elif value == "sha1":
                password_format = 2
            # end if
        # end if

        (status, password) = format_password (password, password_format)
        if not status:
            return (False, "Unable to format password, error was: %s" % password)
        # end if
    # end if

    # check if the clientid is already added
    node       = doc.get ("/myqtt-users/user")
    node_found = False
    while node:
        if node.attr ("id") == client_id:
            node_found = True
            break
        # end if
        
        # get next <user /> node 
        node = node.next_called ("user")
    # end while

    if not node_found:
        return (False, "Unable to find client-id %s" % client_id)

    if username:
        node.attr ("username", username)
        
    # end if
    if password:
        node.attr ("password", password)
    # end if

    # save configuration file
    doc.file_dump (users_xml, 4)

    # restart myqtt server
    (status, info) = myqtt_restart_service ()
    if not status:
        return (False, "User was added but failed to restart myqtt service, error was: %s.." % info)
    # end if

    return (True, "User updated")


def add_account (options, args):

    if len (args) == 0:
        return (False, "Not provided domain where to add requested account")

    domain_name = args[0]

    # call to list current domains
    if not domain_exists (domain_name):
        return (False, "Domain %s was not found. Please create it or be sure about the domain you want your account be created" % domain_name)
    
    account   = options.add_account
    items     = account.split (",")
    if len (items) not in [1, 3]:
        return (False, "Received account indication which is not a single client-id or client-id,username,password: %s" % account)

    client_id = items[0]
    username  = None
    password  = None
    if len (items) > 1:
        username = items[1]
        password = items[2]
    # end if

    # build users db dir
    (status, run_time_location) = get_runtime_datadir ()
    if not status:
        return (False, "Unable to find run time location, error was: %s" % run_time_location)
    users_xml = "%s/%s/users.xml" % (run_time_location.replace ("myqtt", "myqtt-dbs"), domain_name)
    dbg ("Creating account %s (user %s:%s) and domain %s" % (client_id, username, password, domain_name))
    if is_mod_enabled ("mod-auth-xml") and os.path.exists (users_xml):
        dbg ("users.xml database exists (%s)" % users_xml)
        return add_account_mod_auth_xml (client_id, username, password, domain_name, users_xml)
    # if is_mod_enabled ("mod-auth-mysql"):
    #   return add_account_mod_auth_mysql (client_id, username, password, domain_name)
    else:
        return (False, "Unable to add account requested, no suitable auth backend was detected")

    # never reached
    return (True, "Account %s (%s) added" % (client_id, domain_name))

def domain_exists (domain_name):

    # call to list current domains
    (status, domains) = list_domains (options, args)
    if not status:
        dbg ("ERROR: failed to list domains, error was: %s" % domains)
        return False

    domain_found = False
    for domain in domains:
        if domain['name'] == domain_name:
            domain_found = True
            break
        # end if
    # end for

    dbg ("Domain %s exists=%d" % (domain_name, domain_found))

    return domain_found

def set_account_password (options, args):
    
    if len (args) == 0:
        return (False, "Not provided domain where to update requested account")

    domain_name = args[0]

    # check if domain exists
    if not domain_exists (domain_name):
        return (False, "Domain %s was not found. Please create it or be sure about the domain you want your account be created" % domain_name)
    
    account   = options.set_account_password
    items     = account.split (",")
    if len (items) not in [2, 3]:
        return (False, "Received account indication which is not a single client-id,password or client-id,username,password: %s" % account)

    client_id = items[0]
    username  = None
    password  = items[1]
    if len (items) > 2:
        username = items[1]
        password = items[2]
    # end if

    # build users db dir
    (status, run_time_location) = get_runtime_datadir ()
    if not status:
        return (False, "Unable to find run time location, error was: %s" % run_time_location)
    users_xml = "%s/%s/users.xml" % (run_time_location.replace ("myqtt", "myqtt-dbs"), domain_name)
    dbg ("Updating account %s (user %s:%s) from domain %s" % (client_id, username, password, domain_name))

    # according to the backend, update the user
    if is_mod_enabled ("mod-auth-xml") and os.path.exists (users_xml):
        dbg ("users.xml database exists (%s)" % users_xml)
        return update_account_mod_auth_xml (client_id, username, password, domain_name, users_xml)
    
    # if is_mod_enabled ("mod-auth-mysql"):
    #   return add_account_mod_auth_mysql (client_id, username, password, domain_name)
    else:
        return (False, "Unable to add account requested, no suitable auth backend was detected")

    # never reached
    return (True, "Account %s (%s) updated" % (client_id, domain_name))

def remove_account_mod_auth_xml (client_id, domain_name, users_xml):
    # open document
    (doc, err) = axl.file_parse (users_xml)
    if not doc:
        return (False, "Unable to add account, axl.file_parse (%s) failed, error was: %s" % (users_xml, err.msg))
    
    # check if the clientid is already added
    node       = doc.get ("/myqtt-users/user")
    node_found = False
    while node:
        if node.attr ("id") == client_id:
            node_found = True
            break
        # end if
        
        # get next <user /> node 
        node = node.next_called ("user")
    # end while

    if not node_found:
        return (False, "Unable to find client-id %s" % client_id)

    # remove node
    node.remove ()

    # save configuration file
    doc.file_dump (users_xml, 4)

    # restart myqtt server
    (status, info) = myqtt_restart_service ()
    if not status:
        return (False, "User was removed but failed to restart myqtt service, error was: %s.." % info)
    # end if

    return (True, "User removed")


def remove_account (options, args):
    
    if len (args) == 0:
        return (False, "Not provided domain where to remove requested account")

    domain_name = args[0]

    # check if domain exists
    if not domain_exists (domain_name):
        return (False, "Domain %s was not found. Please be sure about the domain you want your account be removed" % domain_name)
    
    client_id = options.remove_account

    # build users db dir
    (status, run_time_location) = get_runtime_datadir ()
    if not status:
        return (False, "Unable to find run time location, error was: %s" % run_time_location)
    users_xml = "%s/%s/users.xml" % (run_time_location.replace ("myqtt", "myqtt-dbs"), domain_name)
    dbg ("Removing account %s from domain %s" % (client_id, domain_name))

    # according to the backend, update the user
    if is_mod_enabled ("mod-auth-xml") and os.path.exists (users_xml):
        dbg ("users.xml database exists (%s)" % users_xml)
        return remove_account_mod_auth_xml (client_id, domain_name, users_xml)
    
    # if is_mod_enabled ("mod-auth-mysql"):
    #   return add_account_mod_auth_mysql (client_id, username, password, domain_name)
    else:
        return (False, "Unable to remove account requested, no suitable auth backend was detected")

    # never reached
    return (True, "Account %s (%s) removed" % (client_id, domain_name))
    


### MAIN ###
if __name__ == "__main__":
    # install arguments and process them
    (options, args) = install_arguments ()

    if options.list_modules:
        (status, modules) = list_modules ()
        if not status:
            print "ERROR: failed to list modules, error was: %s" % modules
            sys.exit (-1)
        # end if
        
        for mod in modules:
            status = ""
            if is_mod_enabled (mod):
                status = " (enabled)"
            print " - %s%s" % (mod, status)
        sys.exit (0)

    elif options.enable_mod:

        (status, info) = enable_mod (options.enable_mod)
        if status:
            print "Mod %s enabled. Restart myqtt server" % options.enable_mod
            sys.exit (0)

        print "ERROR: %s" % info
        sys.exit (-1)

    elif options.disable_mod:

        (status, info) = disable_mod (options.disable_mod)
        if status:
            print "Mod %s disabled. Restart myqtt server" % options.disable_mod
            sys.exit (0)

        print "ERROR: %s" % info
        sys.exit (-1)
    
    elif options.explain_easy_config:
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

    elif options.list_domains:
        # call to list current domains
        (status, domains) = list_domains (options, args)
        if not status:
            print "ERROR: failed to list domains, error was: %s" % domains
            sys.exit (-1)

        for domain in domains:
            status = ""
            if domain['is_active']:
                status = "(active)"
            
            print "- %s %s" % (domain['name'], status)
            
        # end if
        sys.exit (0)

    elif options.create_domain:
        
        # call to create domain
        (status, info) = create_domain (options, args)
        if not status:
            print "ERROR: failed to create domain %s, error was: %s" % (options.create_domain, info)
            sys.exit (-1)
        # end if

        print "INFO: domain %s created -- %s" % (options.create_domain, info)
        sys.exit (0)


    elif options.add_account:
        
        # call to create domain
        (status, info) = add_account (options, args)
        if not status:
            print "ERROR: failed to add account %s, error was: %s" % (options.add_account, info)
            sys.exit (-1)
        # end if

        print "INFO: account created -- %s" % info
        sys.exit (0)
        
    elif options.set_account_password:
        
        # call to create domain
        (status, info) = set_account_password (options, args)
        if not status:
            print "ERROR: failed to update account %s, error was: %s" % (options.set_account_password, info)
            sys.exit (-1)
        # end if

        print "INFO: account updated -- %s" % info
        sys.exit (0)

    elif options.remove_account:
        
        # call to create domain
        (status, info) = remove_account (options, args)
        if not status:
            print "ERROR: failed to remove account %s, error was: %s" % (options.remove_account, info)
            sys.exit (-1)
        # end if

        print "INFO: account removed -- %s" % info
        sys.exit (0)

                
            
    


