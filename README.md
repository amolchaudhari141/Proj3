# Proj3
Client Server communication 
You are asked to write a client –server program in which the client contact db
server to access a database. We need to create TCP/UDP socket in (C &amp; Go) You
are asked to write three C files: server.c, client.go and servicemap.go.
First, we will discuss the basic logic behind this project.

1) Sever will broadcast the registration message (PUT BANK620 137,148,216,21,178,110)
to the servicemap .The servicemap will store the message into its dictionary
2) Then servicemap will send OK message to server which will indicate the successful
communication between server and servicmap.
3) Client broadcasts message (GET BANK620) servicemap will search BANK620 into its
dictionary and will reply back
4) In reply from servicemap it will send (datagram )db servers ip address and port number
string (137,148,204,15,178,110) to the client.
5) Client will establish TCP connection with db Server and requests the query command to get
and reply the record for a person.
6) Once the TCP connection is established the received at server it will make the
communication with DB17 and access the data .
7) Once data is found it will sent back to client
