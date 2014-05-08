COMP8505-Scalable-Server
========================

Objective
To compare the scalability and performance of the multi-threaded, select and e-poll based client-server implementations.

Assignment Requirements
The goal of this assignment is to design and implement three separate servers:
1. A multi-threaded, traditional server
2. A select (level-triggered) multiplexed server
3. An e-poll (edge-triggered) asynchronous server

Each server will be designed to handle multiple connections and transfer a specified amount of data to the connected client.

Each server must be designed to handle multiple connections and transfer a specified amount of data to the connected client.

A simple echo client must be designed and implemented with the ability to send variable-length text strings to the server and the number of times to send the strings will be a user-specified value.

Each client will have to maintain the connection for varying time durations, depending on how much data and iterations. This will be done to keep increasing the load on the server until its performance degrades quite significantly. This will be done to measure how many (scalability) connections the server can handle, and how fast (performance) it can deliver the data back to the clients.

Each client and server will have to maintain their own statistics.
