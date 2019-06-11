<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-generate-toc again -->
**Table of Contents**

- [About](#about)
- [Feature](#feature)
- [Note](#note)

<!-- markdown-toc end -->

# About
This a small program that a client can send messages to clients with a server in between them. 

The function of the server is that when a client supposes Client A sends a message to the server, the server should forward that message to the other clients. 

Same way when the Client B sends a message to the server it should be forwarded to the other.

# Feature
Type `quit` in client to end chatting.

Note that the client can only receives message after sending message, this might can be solved by many ways:
1. Using blocking sockets with reading/writing threads
2. Using non-blocking socket I/O with event polling
3. And then there is asynchronous socket I/O using Overlapped I/O, I/O Completion Ports, or Registered I/O extensions

See [Winsock - How to receive and send data simultaneously?](https://stackoverflow.com/questions/38728713/winsock-how-to-receive-and-send-data-simultaneously) for more detail.

# Note
I can run successfully after compiling it using Dev-C++ but fail with CodeBlocks.

After debugging, I find it has something to do with `char tempbuf[DEFAULT_BUFLEN];`. Using `memcpy()` to convert struct type into char array needs the char array is big enough. Since I use `x86_64-w64-mingw32-g++.exe` to compile the server file, the size of tempbuf may not big enough to hold message struct. So I need to double the length of tempbuf.