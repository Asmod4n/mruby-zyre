# mruby-zyre

This is a naive wrapper around https://github.com/zeromq/zyre.
Most methods found at https://github.com/zeromq/zyre#api-summary are mapped 1:1 to ruby.

Prerequirements
===============

You need to have https://github.com/zeromq/libzmq, https://github.com/zeromq/czmq and https://github.com/zeromq/zyre installed in a path mruby can find it.
zyre isn't yet in most package managers, the maintainers recommend it to install the latest git master.

Examples
========

Simple local subnet discovery via UDP
----------------------
```ruby
zyre1 = Zyre.new#(name = nil)
zyre1["MyHeader"] = "MyValue"
zyre1.join("channel")

zyre2 = Zyre.new
zyre2.join("channel")

zyre1.start
zyre2.start

zyre2.recv
peer = zyre2.peers.first
puts zyre2.peer_header_value(peer, "MyHeader")
zyre2.shout("channel", "hallo")
zyre2.whisper(peer, "hello")
puts zyre1.recv
puts zyre1.recv
puts zyre1.recv
puts zyre1.recv
puts zyre1.peers_by_group("channel")
```

Discovery via TCP
-----------------
```ruby
zyre1 = Zyre.new "node1"
zyre1.endpoint = "tcp://127.0.0.1:5671" # this enables gossip discovery via tcp instead of udp broadcasts in the local subnet
zyre1.gossip_bind "tcp://127.0.0.1:5670" # at least one node must bind to a endpoint which other nodes can connect to
zyre1.join "channel"

zyre2 = Zyre.new "node2"
zyre2.endpoint = "tcp://127.0.0.1:5672" # wildcards are sadly not supported, even though libzmq supports them
zyre2.gossip_connect "tcp://127.0.0.1:5670"
zyre2.join "channel"

zyre1.start
zyre2.start

zyre2.recv
```

You can let several nodes bind to a gossip endpoint and connect them together
----
```ruby
zyre1 = Zyre.new "node1"
zyre1.endpoint = "tcp://127.0.0.1:5671"
zyre1.gossip_bind "tcp://127.0.0.1:5670"
zyre2.gossip_connect "tcp://127.0.0.1:5673"
zyre1.join "channel"

zyre2 = Zyre.new "node2"
zyre2.endpoint = "tcp://127.0.0.1:5672"
zyre2.gossip_bind "tcp://127.0.0.1:5673"
zyre2.gossip_connect "tcp://127.0.0.1:5670"
zyre2.join "channel"

zyre3 = Zyre.new
zyre3.endpoint = "tcp://127.0.0.1:5674"
zyre3.gossip_connect "tcp://127.0.0.1:5670"
zyre3.gossip_connect "tcp://127.0.0.1:5673"
zyre3.join "channel"

zyre1.start
zyre2.start
zyre3.start

zyre3.recv
puts zyre3.peers
```
It should also be possible to bind to several gossip endpoints

Encryption/Authentication
=========================

None as of this writing. Feel free to fix that upstream. zeromq has built in support for strong encryption and authentication, but so far no czmq based services offer it.
