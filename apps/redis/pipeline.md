## Request/Response protocols and RTT
By default every client sends the next command only when the reply of the previous command is received, this means that the server will likely need a read call in order to read each command from every client. Also RTT is paid as well.

So for instance a four commands sequence is something like this:
- *Client*: INCR X
- *Server*: 1
- *Client*: INCR X
- *Server*: 2
- *Client*: INCR X
- *Server*: 3
- *Client*: INCR X
- *Server*: 4

### Redis Pipelining
A Request/Response server can be implemented so that it is able to process new requests even if the client didn't already read the old responses. This way it is possible to send *multiple commands* to the server without waiting for the replies at all, and finally read the replies in a single step.

This is an example using the raw netcat utility:
```
$ (printf "PING\r\nPING\r\nPING\r\n"; sleep 1) | nc localhost 6379
+PONG
+PONG
+PONG
```
This time we are not paying the cost of RTT for every call, but just one time for the three commands.

To be very explicit, with pipelining the order of operations of our very first example will be the following:
- *Client*: INCR X
- *Client*: INCR X
- *Client*: INCR X
- *Client*: INCR X
- *Server*: 1
- *Server*: 2
- *Server*: 3
- *Server*: 4

**IMPORTANT NOTE**: While the client sends commands using pipelining, the server will be forced to queue the replies, using memory. 
