<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-generate-toc again -->
**Table of Contents**

- [About](#about)
- [Reference](#reference)

<!-- markdown-toc end -->

# About
There are many ways to handle multiple client connections. The first and most intuitive one is using threads like what I do in `SingleServerSingleClientMS`. As soon as a client connects, assign a separate thread to process each client. However threads are too much work and difficult to code properly.

There are other techniques like polling. Polling involves monitoring multiple sockets to see if "something" happened on any of them. For example, the server could be monitoring the sockets of 5 connected clients, and as soon as any of them send a message, the server gets notified of the event and then processes it. In this way it can handle multiple sockets. The winsock api provides a function called `select` which can monitor multiple sockets for some activity.

Since we are able to handle all sockets together at once it is called **asynchronous socket programming**. It is also called **event-driven socket programming** or select()-based multiplexing.

# Reference
[Code a simple tcp socket server in winsock](https://www.binarytides.com/code-tcp-socket-server-winsock/)