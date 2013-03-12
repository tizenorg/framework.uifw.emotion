Name:             emotion
Summary:          Media Library
Version:          1.7.1+svn.76438+build01r01
Release:          1
Group:            System Environment/Libraries
License:          BSD
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

%autogen
%configure --enable-static \
	--disable-rpath --enable-xine=no --disable-doc
make %{?jobs:-j%jobs}


%install
rm -rf %{buildroot}
%make_install
mkdir -p %{buildroot}/usr/share/license
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/usr/share/license/%{name}


%post -p /sbin/ldconfig


%postun -p /sbin/ldconfig


%post gstreamer -p /sbin/ldconfig


%postun gstreamer -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%{_libdir}/libemotion.so.*
%{_bindir}/emotion_*
%{_datadir}/emotion/data/*.edj
%{_libdir}/emotion
%{_libdir}/edje/modules/emotion/*/module.so
%manifest %{name}.manifest
/usr/share/license/%{name}
%manifest %{name}.manifest


%files devel
%defattr(-, root, root)
%{_includedir}/*
%{_libdir}/pkgconfig/emotion*.pc
%{_libdir}/libemotion.so


%files gstreamer
%defattr(-, root, root)
%{_libdir}/emotion/*gstreamer*.so
%manifest %{name}-gstreamer.manifest
