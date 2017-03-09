// buffer publisher that subscribes to the face analysis publisher and streams statistics on the signals
// Author: RLV (UNITN)
// last update: 02/03/17

#include "zhelpers.hpp"
#include "zmq.hpp"
#include <iostream>
#include <map>
#include <boost/lexical_cast.hpp>

using namespace std;

vector<string> get_arguments(int argc, char **argv){

    vector<string> arguments;

    // First argument is reserved for the name of the executable
    for(int i = 0; i < argc; ++i)
    {
        arguments.push_back(string(argv[i]));
    }
    return arguments;
}

void read_arguments(vector<string> &arguments, string &pubPort, string &subPort, double &integTime) {

    bool* valid = new bool[arguments.size()];
    valid[0] = true;

    // First check if there is a root argument (so that videos and outputs could be defined more easilly)
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i].compare("-pubPort") == 0) {
            pubPort = arguments[i + 1];
            valid[i] = false;
            valid[i + 1] = false;
            i++;
        }
        if (arguments[i].compare("-subPort") == 0) {
            subPort = arguments[i + 1];
            valid[i] = false;
            valid[i + 1] = false;
            i++;
        }
        if (arguments[i].compare("-interval") == 0) {
            stringstream data(arguments[i + 1]);
            data >> integTime;
            valid[i] = false;
            valid[i + 1] = false;
            i++;
        }
    }

    for (int i = (int)arguments.size() - 1; i >= 0; --i)
    {
        if (!valid[i])
        {
            arguments.erase(arguments.begin() + i);
        }
    }
}

int main (int argc, char** argv) {

    vector<string> arguments = get_arguments(argc, argv);

    string pubPort = "";
    string subPort = "";
    double integTime = 0;

    read_arguments(arguments, pubPort, subPort, integTime);

    if (strcmp(pubPort.c_str(), "") == 1){
        std::cerr << "You need to specify a valid publisher port to connect to (using -pubPort argument)" << endl;
        return 1;
    } else {
        cout << "usmSub.cpp -> main: pubPort: " << pubPort << endl;
    }

    if (strcmp(subPort.c_str(), "") == 1){
        subPort = "tcp://*6001";
        cout << "usmSub.cpp -> main: Unspecified publishing port (for external subscribers), using the default value: " << subPort << endl;
    } else {
        cout << "usmSub.cpp -> main: subPort: " << subPort << endl;
    }

    if (integTime == 0){
        integTime = 2;
        cout << "usmSub.cpp -> main: Unspecified integration time, using the default value: " << integTime << "(s)" << endl;
    } else {
        cout << "usmSub.cpp -> main: integration interval: " << integTime << "(s)" << endl;
    }

    zmq::context_t contextSub(1);
    zmq::socket_t subscriber (contextSub, ZMQ_SUB);
//    uint64_t RCVBUF = 50000;
//    subscriber.setsockopt(ZMQ_RCVBUF, &RCVBUF, sizeof(RCVBUF));
//    uint64_t HWM = 1;
//    subscriber.setsockopt(ZMQ_HWM, &HWM, sizeof(HWM));
    subscriber.connect("tcp://*:6000");
    subscriber.setsockopt( ZMQ_SUBSCRIBE, "", 0);

    map<string, int> messageMapDir;
    messageMapDir["headTilt"] = 1;
    messageMapDir["headYaw"] = 2;
    messageMapDir["headRoll"] = 3;
    messageMapDir["valenceLevel"] = 4;
    messageMapDir["arousalLevel"] = 5;
    messageMapDir["painLevel"] = 6;
    messageMapDir["sys_time"] = 7;

    map<int, string> messageMapInv;
    messageMapInv[1] = "headTilt";
    messageMapInv[2] = "headYaw";
    messageMapInv[3] = "headRoll";
    messageMapInv[4] = "valenceLevel";
    messageMapInv[5] = "arousalLevel";
    messageMapInv[6] = "painLevel";
    messageMapInv[7] = "sys_time";

    double accValues[7] = {0, 0, 0, 0, 0, 0, 0};
    double accSqValues[7] = {0, 0, 0, 0, 0, 0, 0};
    double accCount[7] = {0, 0, 0, 0, 0, 0, 0};
    double maxValues[7] = {-90, -90, -90, -1, 0, 0, 0};

    double startTime = 0;

    double valMean[6] = {0, 0, 0, 0, 0, 0};
    double valStd[6] = {0, 0, 0, 0, 0, 0};
    double valMax[6] = {0, 0, 0, 0, 0, 0};

    // ----------------------------------------------------------------------
    // now launch a publisher port for broadcasting results
    zmq::context_t contextPub (1);

    std::cout << "Creating publisher context \n" << std::endl;
    zmq::socket_t publisher (contextPub, ZMQ_PUB);
    publisher.bind("tcp://*:6001");
    // end of creating context for the publisher part
    // ----------------------------------------------------------------------


    while (1) {

        //  Read envelope with address
        std::string address = s_recv (subscriber);
        //  Read message contents
        std::string contents = s_recv (subscriber);

        double localVal = std::stod(contents);
        int localKey = messageMapDir[address]- 1;

        if (localKey < 6) {
            accValues[localKey] += localVal;
            accSqValues[localKey] += std::pow(localVal, 2);
            accCount[localKey] += 1;
            maxValues[localKey] = (maxValues[localKey] < localVal)? localVal: maxValues[localKey];
        } else {
            accValues[localKey] = localVal;
        }

        if ((accValues[6]- startTime) > integTime* 1000) {
            startTime = accValues[6];
            for (int k= 0; k < 6; k++) {
                valMean[k] = accValues[k]/accCount[k];
                valMax[k] = maxValues[k];
                valStd[k] = std::sqrt(std::abs(std::pow(valMean[k], 2) - accSqValues[k]/accCount[k]));
                cout << "Key " << k+ 1 << ": mean = " << valMean[k] << endl;
                cout << "Key " << k+ 1 << ": std = " << valStd[k] << endl;
                cout << "Key " << k+ 1 << ": max = " << valMax[k] << endl;

                accValues[k] = 0;
                accCount[k] = 0;
                accSqValues[k] = 0;
                if (k < 3){
                    maxValues[k] = -90;
                } else if (k == 3){
                    maxValues[k] = -1;
                } else {
                    maxValues[k] = 0;
                }

                if (k == 5) {
                    s_sendmore(publisher, messageMapInv[k + 1] + "_mean");
                    s_send(publisher, boost::lexical_cast<std::string>(valMean[k]));

                    s_sendmore(publisher, messageMapInv[k + 1] + "_std");
                    s_send(publisher, boost::lexical_cast<std::string>(valStd[k]));

                    s_sendmore(publisher, messageMapInv[k + 1] + "_max");
                    s_send(publisher, boost::lexical_cast<std::string>(valMax[k]));
                } else {
                    s_sendmore(publisher, messageMapInv[k + 1] + "_mean");
                    s_send(publisher, boost::lexical_cast<std::string>(valMean[k]));

                    s_sendmore(publisher, messageMapInv[k + 1] + "_std");
                    s_send(publisher, boost::lexical_cast<std::string>(valStd[k]));
                }
            }
            s_sendmore(publisher, "sysTime_face");
            s_send(publisher, boost::lexical_cast<std::string>(accValues[6]));
        }
    }
    return 0;
}