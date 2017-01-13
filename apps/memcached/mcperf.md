# twemperf (mcperf)

## Building mcperf ##

To build mcperf from distribution tarball:

    $ ./configure
    $ make
    $ sudo make install


## Help ##

    Usage: mcperf [-s server] [-p port] [-H] [-l linger]
                  [-c client] [-n num-conns] [-N num-calls]
                  [-r conn-rate] [-R call-rate] [-z sizes]

    Options:
      -s, --server=S        : set the hostname of the server (default: localhost)
      -p, --port=N          : set the port number of the server (default: 11211)
      -l, --linger=N        : set the linger timeout in sec, when closing TCP connections (default: off)
      -n, --num-conns=N     : set the number of connections to create (default: 1)
      -N, --num-calls=N     : set the number of calls to create on each connection (default: 1)
      -r, --conn-rate=R     : set the connection creation rate (default: 0 conns/sec)
      -R, --call-rate=R     : set the call creation rate (default: 0 calls/sec)
      -z, --sizes=R         : set the distribution for item sizes (default: d1 bytes)
      ...

## Examples ##

The following example creates **1000 connections** to a memcached server
running on **localhost:11211**. The connections are created at the rate of
**1000 conns/sec** and on every connection it sends **10 'set' requests** at
the rate of **1000 reqs/sec** with the item sizes derived from a uniform
distribution in the interval of [1,16) bytes.

    $ mcperf --linger=0 --timeout=5 --conn-rate=1000 --call-rate=1000 --num-calls=10 --num-conns=1000 --sizes=u1,16

The following example creates **100 connections** to a memcached server
running on **localhost:11211**. Every connection is created after the previous
connection is closed. On every connection we send **100 'set' requests** and
every request is created after we have received the response for the previous
request. All the set requests generated have a fixed item size of 1 byte.

    $ mcperf --linger=0 --conn-rate=0 --call-rate=0 --num-calls=100 --num-conns=100 --sizes=d1