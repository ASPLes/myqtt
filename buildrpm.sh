#!/bin/sh

echo "Preparing sources"
rm -rf rpm/SOURCES/*.tar.gz
rm -rf rpm/BUILD/*
rm -rf rpm/BUILDROOT/*
find rpm/RPMS/ -type f -exec rm {} \;

# refresh package
./autogen.sh --prefix=/usr --sysconfdir=/etc
make dist
cp myqtt-`cat VERSION`.tar.gz rpm/SOURCES

echo "Calling to compile packages.."
LANG=C rpmbuild -ba --define '_topdir /usr/src/myqtt/rpm' rpm/SPECS/myqtt.spec

echo "Output ready at rpm/RPMS"
find rpm/RPMS -type f -name '*.rpm' > rpm/RPMS/files
cat rpm/RPMS/files



