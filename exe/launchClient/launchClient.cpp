// example of an outside client that sends commands to the launcher
// Author: RLV (UNITN)
// last update: 20/02/17

#include <zmq.hpp>
#include <iostream>
#include <unistd.h>

int main ()
{
    //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);

    std::cout << "Connecting to launcher server…" << std::endl;
    socket.connect ("tcp://localhost:5555");

    //  Do 10 requests, waiting each time for a response

    for (int request_nbr = 0; request_nbr <= 1; request_nbr++) {
        zmq::message_t request (6);
        memcpy (request.data (), "Ignore", 6);
        std::cout << "Sending Ignore message (" << request_nbr << ")...";
        socket.send (request);

        //  Get the reply.
        zmq::message_t reply;
        socket.recv (&reply);
        std::string rep = std::string(static_cast<char*>(reply.data()), reply.size());
        std::cout << "response from server: " << rep << std::endl;
    }

    zmq::message_t request (5);
    memcpy (request.data (), "Start", 5);
    std::cout << "Sending Start request... ";
    socket.send (request);

    //  Get the reply.
    zmq::message_t reply;
    socket.recv (&reply);
    std::string rep = std::string(static_cast<char*>(reply.data()), reply.size());
    std::cout << "response from server: " << rep << std::endl;

    sleep(600);
    zmq::message_t request_1 (8);
    memcpy (request_1.data (), "Kill all", 8);
    std::cout << "Sending Kill request... ";
    socket.send (request_1);

    //  Get the reply.
    zmq::message_t reply_1;
    socket.recv (&reply_1);
    std::string rep_1 = std::string(static_cast<char*>(reply_1.data()), reply_1.size());
    std::cout << "response from server: " << rep_1 << std::endl;

    sleep(2);
    zmq::message_t request_2 (4);
    memcpy (request_2.data (), "Stop", 4);
    std::cout << "Sending Stop request... ";
    socket.send (request_2);

    //  Get the reply.
    zmq::message_t reply_2;
    socket.recv (&reply_2);
    std::string rep_2 = std::string(static_cast<char*>(reply_2.data()), reply_2.size());
    std::cout << "response from server: " << rep_2 << std::endl;

    return 0;
}
