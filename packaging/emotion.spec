Name:             emotion
Summary:          Media Library
Version:          0.1.0+svn.69618slp2+build01
Release:          1
Group:            System Environment/Libraries
License:          BSD
URL:              http://www.enlightenment.org/
Source0:          %{name}-%{version}.tar.gz
Requires(post):   /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:    pkgconfig(eet)
BuildRequires:    pkgconfig(evas)
BuildRequires:    pkgconfig(edje)
BuildRequires:    pkgconfig(ecore)
BuildRequires:    pkgconfig(embryo)
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


%files devel
%defattr(-, root, root)
%{_includedir}/*
%{_libdir}/pkgconfig/emotion*.pc
%{_libdir}/libemotion.so


%files gstreamer
%defattr(-, root, root)
%{_libdir}/emotion/*gstreamer*.so
