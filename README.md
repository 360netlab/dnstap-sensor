## Dnstap-sensor

The dnstap-sensor receives the DNS server's dnstap from the unix or tcp socket, and sends it to syslog after formatting.

Base64 copy from this [repository](https://github.com/littlstar/b64.c).

### Usage

```
dnstap-sensor [-u unix socket path] [-L tcp listen ip] [-P tcp listen port]> [-p facility.priority] [-l facility.priority for log] [-d]

arguments:
    -u: Unix socket path of the dnsptap server to receive dnstap payloads.
        Default: /var/run/named/dnstap.sock
    -L: IP of the dnsptap server to receive dnstap payloads.
        Default: 0.0.0.0
    -P: Port of the dnstap receiver is listening on.
        Default: 6000
    -p: Syslog facility name and priority name.
        Default: local6.info
    -l: Syslog facility name and priority name for log.
        Default: local6.debug
    -d: Daemon, run in the background.
    -h: Show this info.
    Dnstap-sensor is running on unix socket by default.

For example:
dnstap-sensor -L 127.0.0.1 -P 8000 -p local5.info -d
```

Notice:

Because dnstap-sensor and dns server exchange dnstap through unix socket, the uid of the process must be consistent with the uid of the dns server process.

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
rpm -ivh dnstap-sensor-[version-HEAD.arch].rpm
```
