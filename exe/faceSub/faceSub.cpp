// example of an outside subscriber that only needs headRoll estimates from the publisher
// Author: RLV (UNITN)
// last update: 20/02/17

#include "zhelpers.hpp"
#include "zmq.hpp"
#include <iostream>

using namespace std;

int main () {
    //  Prepare our context and subscriber
    zmq::context_t context(1);
    zmq::socket_t subscriber (context, ZMQ_SUB);

//    uint64_t RCVBUF = 0;
//    subscriber.setsockopt(ZMQ_RCVBUF, &RCVBUF, sizeof(RCVBUF));
//    uint64_t HWM = 1;
//    subscriber.setsockopt(ZMQ_HWM, &HWM, sizeof(HWM));

    subscriber.connect("tcp://*:6000");
    std::string filter = "sys_time";
    subscriber.setsockopt( ZMQ_SUBSCRIBE, filter.c_str(), filter.length());

    while (1) {

        //  Read envelope with address
        std::string address = s_recv (subscriber);
        //  Read message contents
        std::string contents = s_recv (subscriber);

        cout << "[" << address << "] " << contents << endl;
    }
    return 0;
}