Name:		juggler
Version:	0.0.1
Release:	1%{?dist}
Summary:	A simple UDP packet forwarder in threaded C.

Group:		Applications/Utilities
License:	ASL 2.0
URL:		https://github.com/ghostflame/juggler
Source0:	https://github.com/ghostflame/juggler/archive/%{version}.tar.gz

BuildRequires:	gcc
Requires(pre):

%description
A simple, threaded UDP packet forwarder.  It can do many to one, one to many or
many to many.


%prep
%setup -q


%build
make %{?_smp_mflags}


%install
DESTDIR=%{buildroot} \
BINDIR=%{buildroot}%{_bindir} \
%make_install


%files
%doc
%{_bindir}/juggler


%changelog

