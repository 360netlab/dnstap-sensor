Name:		dnstap-sensor
Version:	1.0
Release:	%{?dist}
Summary:	360Netlab DNSTAP sensor

Group:		360Netlab
License:	 LGPL-3.0 License
URL:		https://netlab.360.com
Source0:	%{name}-%{version}-%{release}.tar.gz

Prefix: /usr/local

%define debug_package %{nil}
%define __strip /bin/true

%description
The dnstap-sensor receives the DNS server's dnstap from the unix socket created by the -u parameter, and sends it to syslog after formatting.

%description -l zh_CN
"dnstap-sensor"程序,从-u参数创建的unix套接字接收DNS服务器写入的dnstap,格式化后发送到syslog.

%prep

%build


%install
mkdir -pv %{buildroot}%{_prefix}/local/bin
cp -r %{_topdir}/../bin/dnstap-sensor %{buildroot}%{_prefix}/local/bin

%files
%defattr(-,root,root)
#%dir %{_prefix}/local/
%{_prefix}/local/bin/dnstap-sensor

%post

%postun

%changelog
