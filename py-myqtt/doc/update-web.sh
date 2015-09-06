#!/bin/bash
rsync --exclude=.svn --exclude=update-web.sh -avz .build/ aspl-web@www.aspl.es:www/myqtt/py-myqtt-doc/



