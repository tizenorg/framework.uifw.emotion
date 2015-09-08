Name:             emotion
Summary:          Media Library
Version:          1.6.0+svn.76438slp2+build10
Release:          1
Group:            System Environment/Libraries
License:          BSD 2-clause
URL:              http://www.enlightenment.org/
Source0:          %{name}-%{version}.tar.gz
Requires(post):   /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:    eina-devel
BuildRequires:    eet-devel
BuildRequires:    evas-devel
BuildRequires:    ecore-devel
BuildRequires:    edje-devel
BuildRequires:    edje-bin
BuildRequires:    gstreamer-devel
BuildRequires:    gst-plugins-base-devel


%description
Video playback library used in Enlightenment DR0.17
 This is the emotion library, a wrapper library used in the next-generation
 Enlightenment suite for video playback.
 .
 This package contains the shared libraries.


%package devel
Summary:    Emotion headers, static libraries, documentation and test programs
Group:      TO_BE/FILLED_IN
Requires:   %{name} = %{version}
Requires: eet-devel evas-devel edje-devel


%description devel
Headers, static libraries, test programs and documentation for Emotion


%package gstreamer
Summary:    Video playback library used in Enlightenment DR0.17
Group:      TO_BE/FILLED_IN
Requires:   %{name} = %{version}
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig


%description gstreamer
Video playback library used in Enlightenment DR0.17
This is the emotion library, a wrapper library used in the next-generation
Enlightenment suite for video playback.
.
This package provides the gstreamer module for emotion.


%prep
%setup -q


%build
export CFLAGS+=" -fvisibility=hidden -fPIC -Wall"
export LDFLAGS+=" -fvisibility=hidden -Wl,--hash-style=both -Wl,--as-needed"

%autogen --enable-static \
         --enable-xine=no \
         --disable-doc

make %{?jobs:-j%jobs}


%install
%make_install
mkdir -p %{buildroot}/%{_datadir}/license
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/%{_datadir}/license/%{name}
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/%{_datadir}/license/%{name}-gstreamer


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%post gstreamer -p /sbin/ldconfig


%postun gstreamer -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%{_libdir}/libemotion.so.*
%{_libdir}/emotion/em_generic.so
%{_libdir}/edje/modules/emotion/*/module.so
%{_datadir}/emotion/data/icon.edj
%{_datadir}/license/%{name}
%manifest %{name}.manifest


%files devel
%defattr(-, root, root)
%{_includedir}/*
%{_libdir}/pkgconfig/emotion*.pc
%{_libdir}/libemotion.so
%{_bindir}/emotion_test
%{_datadir}/emotion/data/theme.edj


%files gstreamer
%defattr(-, root, root)
%{_libdir}/emotion/gstreamer*.so
%{_datadir}/license/%{name}-gstreamer
%manifest %{name}-gstreamer.manifest
