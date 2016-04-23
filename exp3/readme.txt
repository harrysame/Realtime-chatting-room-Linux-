This is a simple version of client-server chatting system using TCP/IP protocol.

Server listens to port 4567 and display the message once it receives from the client.
It can handle different clients at the same time due to the implementation of multi-processing--fork().

Client has to pass parameter in the following way:./ppclient 127.0.0.1 4567
Because this is a single host version so the IP is set to 127.0.0.1
