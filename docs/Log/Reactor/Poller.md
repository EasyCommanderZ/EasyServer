# Poller

本来应该是 Epoll，但是想做一个跨平台，在mac上用kqueue实现，所以叫 Poller。

首先理清一下Poller的运行思路。Poller是用于IO复用的。要弄清楚它，首先要弄明白 WebServer 的[并发模型](./并发模型.md)。