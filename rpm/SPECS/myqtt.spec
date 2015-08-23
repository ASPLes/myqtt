%define release_date %(date +"%a %b %d %Y")
%define myqtt_version %(cat VERSION)

Name:           myqtt
Version:        %{myqtt_version}
Release:        5%{?dist}
Summary:        Open source professional MQTT C stack
Group:          System Environment/Libraries
License:        LGPLv2+ 
URL:            http://www.aspl.es/myqtt
Source:         %{name}-%{version}.tar.gz

# BuildRequires:  libidn-devel
# BuildRequires:  krb5-devel
# BuildRequires:  libntlm-devel
# BuildRequires:  pkgconfig

%define debug_package %{nil}

%description
MyQtt is an Open Source professional MQTT stack written in ANSI C,
focused on providing support to create MQTT servers/brokers. MyQtt has
a modular design that allows creating MQTT brokers by using the API
provided by libMyQtt (which in fact is composed by several libraries:
libmyqtt, libmyqtt-tls, libmyqtt-websocket), but it is also provided a
ready to use MQTT broker called MyQttD which is built on top of
libMyQtt. MyQttD server is extensible by adding C plugins.


%prep
%setup -q

%build
PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/local/lib/pkgconfig %configure --prefix=/usr --sysconfdir=/etc
make clean
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot} INSTALL='install -p'
find %{buildroot} -name '*.la' -exec rm -f {} ';'
find %{buildroot} -name 'mod-test.*' -exec rm -f {} ';'
# find %{buildroot} -name 'mod_test_10b.*' -exec rm -f {} ';'
# find %{buildroot} -name 'mod_test_10_prev.*' -exec rm -f {} ';'
# find %{buildroot} -name 'mod_test_11.*' -exec rm -f {} ';'
# find %{buildroot} -name 'mod_test_15.*' -exec rm -f {} ';'
find %{buildroot} -name '*.conf.tmp' -exec rm -f {} ';'
find %{buildroot} -name '*.xml-tmp' -exec rm -f {} ';'
find %{buildroot} -name '*.win32.xml' -exec rm -f {} ';'
mkdir -p %{buildroot}/etc/init.d
mkdir -p %{buildroot}/etc/myqtt/mods-enabled
install -p %{_builddir}/%{name}-%{version}/doc/myqtt-rpm-init.d %{buildroot}/etc/init.d/myqtt

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

# %files -f %{name}.lang
%doc AUTHORS COPYING NEWS README THANKS

# libmyqtt-1.0 package
%package -n libmyqtt-1.0
Summary: OpenSource MQTT stack written in ANSI C
Group: System Environment/Libraries
Requires: libaxl1
%description  -n libmyqtt-1.0
MyQtt is an Open Source professional MQTT stack written in ANSI C,
focused on providing support to create MQTT brokers. MyQtt has a
modular design that allows creating MQTT brokers by using the API
provided by libMyQtt (which in fact is composed by several
libraries: libmyqtt, libmyqtt-tls, libmyqtt-websocket). It is also
provided a ready to use MQTT broker called MyQttD which is built on
top of libMyQtt. MyQttD server is extensible by adding C plugins.
%files -n libmyqtt-1.0
  /usr/lib64/libmyqtt-1.0.a
  /usr/lib64/libmyqtt-1.0.so
  /usr/lib64/libmyqtt-1.0.so.0
  /usr/lib64/libmyqtt-1.0.so.0.0.0
       
# libmyqtt-1.0-dev package
%package -n libmyqtt-1.0-dev
Summary: Development headers for the core library implementation. 
Group: System Environment/Libraries
Requires: libaxl-dev
Requires: libmyqtt-1.0
%description  -n libmyqtt-1.0-dev
Development headers required to create myqtt modules or tools.
%files -n libmyqtt-1.0-dev
  /usr/include/myqtt-1.0/myqtt-hash-private.h
  /usr/include/myqtt-1.0/myqtt-types.h
  /usr/include/myqtt-1.0/myqtt-ctx-private.h
  /usr/include/myqtt-1.0/myqtt.h
  /usr/include/myqtt-1.0/myqtt-sequencer.h
  /usr/include/myqtt-1.0/myqtt-thread.h
  /usr/include/myqtt-1.0/myqtt-handlers.h
  /usr/include/myqtt-1.0/myqtt-thread-pool.h
  /usr/include/myqtt-1.0/myqtt-io.h
  /usr/include/myqtt-1.0/myqtt-win32.h
  /usr/include/myqtt-1.0/myqtt-listener-private.h
  /usr/include/myqtt-1.0/myqtt-msg.h
  /usr/include/myqtt-1.0/myqtt-support.h
  /usr/include/myqtt-1.0/myqtt-storage.h
  /usr/include/myqtt-1.0/myqtt-msg-private.h
  /usr/include/myqtt-1.0/myqtt-listener.h
  /usr/include/myqtt-1.0/myqtt-conn-private.h
  /usr/include/myqtt-1.0/myqtt-hash.h
  /usr/include/myqtt-1.0/myqtt-conn.h
  /usr/include/myqtt-1.0/myqtt-reader.h
  /usr/include/myqtt-1.0/myqtt-addrinfo.h
  /usr/include/myqtt-1.0/myqtt-ctx.h
  /usr/include/myqtt-1.0/myqtt-errno.h
  /usr/lib64/pkgconfig/myqtt-1.0.pc

# myqttd-server package
%package -n myqttd-server
Summary: Opensource MQTT Server -- MyQttD server 
Group: System Environment/Libraries
Requires: libmyqtt-1.0
%description  -n myqttd-server
  Opensource MQTT Server -- MyQttD server 
%files -n myqttd-server
  /etc/init.d/myqtt
  /usr/bin/myqttd
  /etc/myqtt/mods-available
  /etc/myqtt/myqtt.example.conf
%post -n myqttd-server
chkconfig myqtt on
if [ ! -d /etc/myqtt/mods-enabled ]; then
   mkdir /etc/myqtt/mods-enabled
fi
if [ ! -f /etc/myqtt/myqtt.conf ]; then
        cp /etc/myqtt/myqtt.example.conf /etc/myqtt/myqtt.conf
fi
service myqtt restart

# libmyqttd-server-1.0-dev package
%package -n libmyqttd-server-1.0-dev
Summary: libMyQttD server (develpment headers)
Group: System Environment/Libraries
Requires: libaxl-dev
Requires: libmyqtt-1.0-dev
%description  -n libmyqttd-server-1.0-dev
libMyQttD server (develpment headers)
%files -n libmyqttd-server-1.0-dev
   /usr/include/myqttd/exarg.h
   /usr/include/myqttd/myqttd-child.h
   /usr/include/myqttd/myqttd-config.h
   /usr/include/myqttd/myqttd-conn-mgr.h
   /usr/include/myqttd/myqttd-ctx.h
   /usr/include/myqttd/myqttd-domain.h
   /usr/include/myqttd/myqttd-handlers.h
   /usr/include/myqttd/myqttd-log.h
   /usr/include/myqttd/myqttd-loop.h
   /usr/include/myqttd/myqttd-moddef.h
   /usr/include/myqttd/myqttd-module.h
   /usr/include/myqttd/myqttd-process.h
   /usr/include/myqttd/myqttd-run.h
   /usr/include/myqttd/myqttd-signal.h
   /usr/include/myqttd/myqttd-support.h
   /usr/include/myqttd/myqttd-types.h
   /usr/include/myqttd/myqttd-users.h
   /usr/include/myqttd/myqttd.h

# libmyqtt-tls-1.0 package
%package -n libmyqtt-tls-1.0
Summary: TLS extension support for libMyQtt
Group: System Environment/Libraries
Requires: libaxl1
Requires: libmyqtt-1.0
%description  -n libmyqtt-tls-1.0
TLS extension support for libMyQtt
%files -n libmyqtt-tls-1.0
  /usr/lib64/libmyqtt-tls-1.0.so.0
  /usr/lib64/libmyqtt-tls-1.0.so
  /usr/lib64/libmyqtt-tls-1.0.so.0.0.0
  /usr/lib64/libmyqtt-tls-1.0.a


# libmyqtt-tls-1.0-dev package
%package -n libmyqtt-tls-1.0-dev
Summary: TLS extension support for libMyQtt (develpment headers)
Group: System Environment/Libraries
Requires: libaxl-dev
Requires: libmyqtt-1.0-dev
Requires: libssl-dev
Requires: libmyqtt-tls-1.0
%description  -n libmyqtt-tls-1.0-dev
TLS extension support for libMyQtt (develpment headers)
%files -n libmyqtt-tls-1.0-dev
  /usr/include/myqtt/myqtt-tls.h


# libmyqtt-websocket-1.0 package
%package -n libmyqtt-websocket-1.0
Summary: WebSocket extension support for libMyQtt
Group: System Environment/Libraries
Requires: libnopoll0
Requires: libaxl1
Requires: libmyqtt-1.0
%description  -n libmyqtt-websocket-1.0
WebSocket extension support for libMyQtt
%files -n libmyqtt-websocket-1.0
  /usr/lib64/libmyqtt-web-socket-1.0.so
  /usr/lib64/libmyqtt-web-socket-1.0.so.0
  /usr/lib64/libmyqtt-web-socket-1.0.so.0.0.0
  /usr/lib64/libmyqtt-web-socket-1.0.a


# libmyqtt-websocket-1.0-dev package
%package -n libmyqtt-websocket-1.0-dev
Summary: WebSocket extension support for libMyQtt (develpment headers)
Group: System Environment/Libraries
Requires: libnopoll0-dev
Requires: libaxl-dev
Requires: libmyqtt-1.0-dev
Requires: libmyqtt-websocket-1.0
%description  -n libmyqtt-websocket-1.0-dev
WebSocket extension support for libMyQtt (develpment headers)
%files -n libmyqtt-websocket-1.0-dev
  /usr/include/myqtt/myqtt-web-socket.h


# myqtt-client-1.0 package
%package -n myqtt-client-1.0
Summary: MyQtt command line client
Group: System Environment/Libraries
Requires: libmyqtt-1.0
Requires: libmyqtt-websocket-1.0
Requires: libmyqtt-tls-1.0
%description  -n myqtt-client-1.0
MyQtt command line client
%files -n myqtt-client-1.0
  /usr/bin/myqtt-client

# python-myqtt package
%package -n python-myqtt
Summary: MyQtt command line client
Group: System Environment/Libraries
Requires: libmyqtt-1.0
Requires: python
%description  -n python-myqtt
MyQtt command line client
%files -n python-myqtt
  /usr/lib/python2.6/site-packages/myqtt/*

# python-myqtt-dev package
%package -n python-myqtt-dev
Summary: PyMyQtt devel files
Group: System Environment/Libraries
Requires: libmyqtt-1.0
Requires: python
%description  -n python-myqtt-dev
PyMyQtt devel files
%files -n python-myqtt-dev
   /usr/include/py_myqtt/py_myqtt.h
   /usr/include/py_myqtt/py_myqtt_async_queue.h
   /usr/include/py_myqtt/py_myqtt_conn.h
   /usr/include/py_myqtt/py_myqtt_conn_opts.h
   /usr/include/py_myqtt/py_myqtt_ctx.h
   /usr/include/py_myqtt/py_myqtt_handle.h
   /usr/include/py_myqtt/py_myqtt_msg.h
   /usr/include/py_myqtt/py_myqtt_tls.h

# python-myqtt-tls package
%package -n python-myqtt-tls
Summary: Python bindings for libMyQtt TLS
Group: System Environment/Libraries
Requires: libmyqtt-tls-1.0
Requires: python
%description  -n python-myqtt-tls
Python bindings for libMyQtt TLS
%files -n python-myqtt-tls
  /usr/lib/python2.6/site-packages/myqtt/libpy_myqtt_tls_10.so.0
  /usr/lib/python2.6/site-packages/myqtt/tls.py
  /usr/lib/python2.6/site-packages/myqtt/libpy_myqtt_tls_10.so.0.0.0
  /usr/lib/python2.6/site-packages/myqtt/libpy_myqtt_tls_10.so
  /usr/lib/python2.6/site-packages/myqtt/libpy_myqtt_tls_10.a


# libmyqttd-server-1.0 package
%package -n libmyqttd-server-1.0
Summary: libMyQttD library, core library used by MyQttD broker/server
Group: System Environment/Libraries
Requires: libmyqtt-1.0
%description  -n libmyqttd-server-1.0
libMyQttD library, core library used by MyQttD broker/server
%files -n libmyqttd-server-1.0
  /usr/lib64/libmyqttd.so
  /usr/lib64/libmyqttd.a
  /usr/lib64/libmyqttd.so.0.0.0
  /usr/lib64/libmyqttd.so.0


# myqttd-mod-auth-xml package
%package -n myqttd-mod-auth-xml
Summary: Extension auth plugin to provide auth-xml backend
Group: System Environment/Libraries
Requires: libmyqtt-1.0
%description  -n myqttd-mod-auth-xml
Extension auth plugin to provide auth-xml backend
%files -n myqttd-mod-auth-xml
  /usr/lib/myqtt/modules/mod-auth-xml.a
  /usr/lib/myqtt/modules/mod-auth-xml.so.0
  /usr/lib/myqtt/modules/mod-auth-xml.so.0.0.0
  /usr/lib/myqtt/modules/mod-auth-xml.so
  /etc/myqtt/mods-available/mod-auth-xml.xml


# myqttd-mod-ssl package
%package -n myqttd-mod-ssl
Summary: Extension plugin that provides SSL/TLS support to MyQttD
Group: System Environment/Libraries
Requires: libmyqtt-1.0
%description  -n myqttd-mod-ssl
Extension plugin that provides SSL/TLS support to MyQttD
%files -n myqttd-mod-ssl
  /usr/lib/myqtt/modules/mod-ssl.so
  /usr/lib/myqtt/modules/mod-ssl.so.0
  /usr/lib/myqtt/modules/mod-ssl.a
  /usr/lib/myqtt/modules/mod-ssl.so.0.0.0
  /etc/myqtt/ssl/ssl.example.conf
  /etc/myqtt/mods-available/mod-ssl.xml


# myqttd-mod-websocket package
%package -n myqttd-mod-websocket
Summary: Extension plugin that provides WebSocket support to MyQttD
Group: System Environment/Libraries
Requires: libmyqtt-1.0
%description  -n myqttd-mod-websocket
Extension plugin that provides WebSocket support to MyQttD
%files -n myqttd-mod-websocket
  /usr/lib/myqtt/modules/mod-web-socket.so.0.0.0
  /usr/lib/myqtt/modules/mod-web-socket.so.0
  /usr/lib/myqtt/modules/mod-web-socket.a
  /usr/lib/myqtt/modules/mod-web-socket.so
  /etc/myqtt/mods-available/mod-web-socket.xml
  /etc/myqtt/web-socket/web-socket.example.conf


# myqttd-mod-status package
%package -n myqttd-mod-status
Summary: Extension plugin that provides status options to clients
Group: System Environment/Libraries
Requires: libmyqtt-1.0
%description  -n myqttd-mod-status
Extension plugin that provides status options to clients
%files -n myqttd-mod-status
  /usr/lib/myqtt/modules/mod-status.a
  /usr/lib/myqtt/modules/mod-status.so
  /usr/lib/myqtt/modules/mod-status.so.0.0.0
  /usr/lib/myqtt/modules/mod-status.so.0
  /etc/myqtt/mods-available/mod-status.xml
  /etc/myqtt/status/status.example.conf






%changelog
%include rpm/SPECS/changelog.inc

