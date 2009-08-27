%define name    ola
%define version 0.3.1
%define release %mkrel 1


Name:      %{name}
Version:   %{version}
Release:   %{release}
Summary:   OLA - Open Lighting Architecture
Group:     Other
License:   GPL
URL:       http://code.google.com/p/linux-lighting
# Other doc found at:
#  http://www.opendmx.net/index.php/LLA_0.3
#  http://www.opendmx.net/index.php/LLA_on_Linux
Source0:   %{name}/%{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: libmicrohttpd-devel >= 0.4.0, libcppunit-devel, protobuf-devel >= 2.1.0, libctemplate-devel >= 0.95
Requires:      libmicrohttpd >= 0.4.0, libcppunit, protobuf >= 2.1.0, libctemplate >= 0.95

%description
The Open Lighting Architecture (OLA) consists of two parts, the daemon olad and the library, libola.


%package -n libola
Group:         Other
Summary:       OLA - Open Lighting Architecture

%description -n libola
The OLA library


%package -n libola-devel
Group:         Other
Summary:       OLA - Open Lighting Architecture

%description -n libola-devel
The OLA library headers


%prep
%setup -q -n %{name}


%build
%define _disable_ld_no_undefined 1
export LDFLAGS="-Wl,-undefined -Wl,dynamic_lookup"
autoreconf -i
%configure
%make

%check
%make check


%install
rm -rf %buildroot
%makeinstall DESTDIR=%buildroot


%clean
rm -rf %buildroot


%files
%defattr(-,root,root,-)
/usr/bin/olad
/usr/bin/olad_test

%files -n libola
%defattr(-,root,root,-)
%{_libdir}/libola.so*
%{_libdir}/olad/*.so*
%{_libdir}/libola*.so*

%files -n libola-devel
%defattr(-,root,root,-)
/usr/include/
%{_libdir}/olad/libola*.a
%{_libdir}/olad/libola*.la
%{_libdir}/libola*.a
%{_libdir}/libola*.la
%{_libdir}/pkgconfig/libola*.pc

%doc AUTHORS ChangeLog COPYING INSTALL NEWS README TODO


%changelog
* Thu Aug 27 2009 Kevin Deldycke <kevin@deldycke.com> 0.3.1.trunk.20090827-1mdv2009.1
- Rename all project from lla to OLA
- Upgrade to the latest OLA 0.3.1 from the master branch of the git repository
- OLA now requires libmicrohttpd, libcppunit, protobuf and libctemplate
- Disable the --no-undefined option and make all undefined symbols weakly bound
- Add check step
- Rebuild RPM for Mandriva 2009.1

* Mon May 12 2008 Kevin Deldycke <kev@coolcavemen.com> 0.2.3.200710210908-1mdv2008.1
- Ported from Fedora Core 8 ( http://rpms.netmindz.net/FC8/SRPMS.netmindz/lla-0.2.3.200710210908-1.fc8.src.rpm ) to Mandriva 2008.1

* Sun Apr 29 2007 Will Tatam <will@netmindz.net> 0.1.3-1
- Fist Build
