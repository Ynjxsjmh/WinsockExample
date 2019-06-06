<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-generate-toc again -->
**Table of Contents**

- [About](#about)

<!-- markdown-toc end -->

# About
This program implements what is not realized in `SingleServerMultipleEncClientBroadcast` by using thread.

# Brief Introduction
This a simple chat client, where clients join the server and send messages to it, which then in turn sends those messages to all the other connected clients.

It has the clients be able to send messages to the server while also being able to simultaneously receive inbound messages from other clients via the server.
