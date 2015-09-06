#!/bin/bash
rsync --exclude=.svn --exclude=update-web.sh -avz *.css *.html aspl-web@www.aspl.es:www/myqtt/
rsync --exclude=.svn --exclude=update-web.sh -avz es/*.html aspl-web@www.aspl.es:www/myqtt/es/
rsync --exclude=.svn --exclude=update-web.sh -avz images/*.png images/*.gif aspl-web@www.aspl.es:www/myqtt/images/



