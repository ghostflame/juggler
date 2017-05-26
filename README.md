# Juggler

A simple UDP forwarder capable of one-to-many or many-to-one, or many-to-many.
Performs round-robin loadbalancing of targets.


```
juggler -s <port> [-s <port> ...] -t [<ip>:]<port> [-t [<ip>:]<port> ...]
```

