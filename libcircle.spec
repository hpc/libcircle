#
# Note that this package is not relocatable

Name:    see META file
Version: see META file
Release: see META file

Summary: Token based load distribution abstraction.

License: See COPYRIGHT file
Group: System Environment/Base
Source: %{name}-%{version}-%{release}.tgz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}
URL: http://github.com/hpc/libcircle

BuildRequires: openmpi

%description
libcircle is an API for distributing embarrassingly parallel workloads using
self-stabilization. Local actions work towards a global objective in a finite
number of operations. It is not meant for applications where cross-process
communication is necessary. The core algorithm used is based on Dijkstra's 1974
token ring proposal and heavily uses MPI under the hood. It should be easy to
use for anyone with a basic grasp of map-reduce (assuming a predefined mapping
process).

#
# Never allow rpm to strip binaries.
#
%define __os_install_post /usr/lib/rpm/brp-compress
%define debug_package %{nil}

#
# Should unpackaged files in a build root terminate a build?
#
# Note: The default value should be 0 for legacy compatibility.
# This was added due to a bug in Suse Linux. For a good reference, see
# http://slforums.typo3-factory.net/index.php?showtopic=11378
%define _unpackaged_files_terminate_build      0

#############################################################################

%prep
%setup -n %{name}-%{version}-%{release}

%build
%configure --program-prefix=%{?_program_prefix:%{_program_prefix}} \
	%{?with_cflags}

make %{?_smp_mflags}

%install
rm -rf "$RPM_BUILD_ROOT"
mkdir -p "$RPM_BUILD_ROOT"
DESTDIR="$RPM_BUILD_ROOT" make install
DESTDIR="$RPM_BUILD_ROOT" make install-contrib

#############################################################################

%clean
rm -rf $RPM_BUILD_ROOT
#############################################################################

%files -f libcircle.files
%defattr(-,root,root,0755)
%doc AUTHORS
%doc NEWS
%doc README.md
%doc DISCLAIMER
%doc COPYING
%{_bindir}/dcopy
%{_bindir}/dstat
%{_libdir}/*.so*
%{_libdir}/src/*
%{_mandir}/man1/*
%{_mandir}/man3/*
%dir %{_sysconfdir}
#############################################################################

%post
if [ -x /sbin/ldconfig ]; then
    /sbin/ldconfig %{_libdir}
fi

%postun
if [ "$1" = 0 ]; then
    if [ -x /sbin/ldconfig ]; then
	/sbin/ldconfig %{_libdir}
    fi
fi
#############################################################################


%changelog
- See the NEWS file for update details
