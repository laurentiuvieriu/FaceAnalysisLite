//
// Created by radu on 19/01/17.
//

#include "zhelpers.hpp"
#include "zmq.hpp"
#include <iostream>

using namespace std;

int main () {
    //  Prepare our context and subscriber
    zmq::context_t context(1);
    zmq::socket_t subscriber (context, ZMQ_SUB);
    subscriber.connect("tcp://*:6000");
    subscriber.setsockopt( ZMQ_SUBSCRIBE, "valenceLevel", 1);

    while (1) {

        //  Read envelope with address
        std::string address = s_recv (subscriber);
        //  Read message contents
        std::string contents = s_recv (subscriber);

        cout << "[" << address << "] " << contents << endl;
    }
    return 0;
}