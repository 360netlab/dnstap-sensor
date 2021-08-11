## Dnstap-sensor

The dnstap-sensor receives the DNS server's dnstap from the unix socket created by the -u parameter, and sends it to syslog after formatting.

Base64 copy from this [repository](https://github.com/littlstar/b64.c).

### Usage

```
dnstap-sensor <-u unix socket path> <-p facility.priority> [-d]

arguments:
	-u: read dnstap payloads from unix socket
	-p: syslog facility namde and priority name
	-d: daemon, run in the background

For example:
dnstap-sensor -u /var/run/named/dnstap.sock -p local6.info -d
```

## Dependency

To install the dependency under CentOS: 
```
sudo yum install protobuf-c-devel
```

## Building from Git repository

```
git clone https://github.com/360netlab/dnstap-sensor 
cd dnstap-sensor
./build_sensor
```
