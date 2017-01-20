// usage: <executable> -cam <camId> -dir <path> -rec <recFlag>

#include <zmq.hpp>
#include <iostream>

#include "opencv2/highgui/highgui.hpp"
#include <opencv2/objdetect/objdetect.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>
#include <cv.hpp>

using namespace std;
using namespace boost::filesystem;

vector<string> get_arguments(int argc, char **argv)
{

    vector<string> arguments;

    // First argument is reserved for the name of the executable
    for(int i = 0; i < argc; ++i)
    {
        arguments.push_back(string(argv[i]));
    }
    return arguments;
}

void create_directory(string output_path)
{

    // Creating the right directory structure
    auto p = path(output_path);

    if(!boost::filesystem::exists(p))
    {
        bool success = boost::filesystem::create_directories(p);

        if(!success)
        {
            cout << "Failed to create a directory..." << p.string() << endl;
        }
    }
}

void read_arguments(vector<string> &arguments, int &camId, string &output_dir, bool &recFlag, string &port, bool &debugMode) {

//    string separator = string(1, boost::filesystem::path::preferred_separator);

    // First element is reserved for the executable location (useful for finding relative model locs)
//    boost::filesystem::path root = boost::filesystem::path(arguments[0]).parent_path();

    bool* valid = new bool[arguments.size()];
    valid[0] = true;

    // First check if there is a root argument (so that videos and outputs could be defined more easilly)
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i].compare("-cam") == 0) {
            stringstream data(arguments[i + 1]);
            data >> camId;
            valid[i] = false;
            valid[i + 1] = false;
            i++;
        }
        if (arguments[i].compare("-dir") == 0) {
            output_dir = arguments[i + 1];
            valid[i] = false;
            valid[i + 1] = false;
            i++;
        }

        if (arguments[i].compare("-rec") == 0) {
            stringstream data(arguments[i + 1]);
            data >> recFlag;
            valid[i] = false;
            valid[i + 1] = false;
            i++;
        }

		if (arguments[i].compare("-port") == 0) {
            port = arguments[i + 1];
            valid[i] = false;
            valid[i + 1] = false;
            i++;
        }
        if (arguments[i].compare("-debugMode") == 0) {
            stringstream data(arguments[i + 1]);
            data >> debugMode;
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

std::string FormatTime(boost::posix_time::ptime now)
{
    using namespace boost::posix_time;
    static std::locale loc(std::cout.getloc(), new time_facet("%Y%m%d_%H%M%S"));
    std::stringstream wss;
    wss.imbue(loc);
    wss << now;
    return wss.str();
}

std::string GetTimeBasedName()
{
    using namespace boost::posix_time;
    ptime now = second_clock::local_time();
    return FormatTime(now);
}

unsigned long GetCurrentTimeInMilliseconds()
{
    boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration duration( time.time_of_day() );
    return duration.total_milliseconds();
}

int main (int argc, char** argv) {

    // init for streaming from camera Id

    vector<string> arguments = get_arguments(argc, argv);

    int camId = -1;
    string outDir = "";
    bool recFlag = false;
    string subDir = "";
    string port = "";
    bool debugMode = false;

    read_arguments(arguments, camId, outDir, recFlag, port, debugMode);

    if (strcmp(port.c_str(), "") == 1){
        port = "tcp://*5555";
        cout << "processPub.cpp -> main: Unspecified port, using the default value: " << port << endl;
    }

    if (camId == -1){
        std::cerr << "You need to specify the camera Id (using -cam argument)" << endl;
        return 1;
    }

    if (recFlag == 1) {
        if (outDir.compare("") == 0) {
            outDir = "./out/"; // default value in case -dir is not specified
        }
        subDir = GetTimeBasedName();
        create_directory(outDir + "/" + subDir);
    }

    cv::VideoCapture cap(camId);
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    if (!cap.isOpened()){
        return -1;
    }
    cv::Mat frame = cv::Mat::zeros(480, 640, CV_8UC3);
//    cvNamedWindow("Publisher on port " + port.c_str(), CV_WINDOW_AUTOSIZE);

    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(75);
    compression_params.push_back(0);

    cv::Point textOrig;
    textOrig.x = 30;
    textOrig.y = 30;

    //  Prepare our context and publisher
    cout << "processPub : preparing context for publishing...";
    zmq::context_t context (1);
    zmq::socket_t publisher (context, ZMQ_PUB);
    uint64_t SNDBUF = 500000;
    publisher.setsockopt(ZMQ_SNDBUF, &SNDBUF, sizeof(SNDBUF));
    uint64_t HWM = 1;
    publisher.setsockopt(ZMQ_HWM, &HWM, sizeof(HWM));
    publisher.bind(port.c_str());
    cout << "done, now streaming data ..." << endl;

    while (1) {

        clock_t t1 = clock();
        cap >> frame;

        if (recFlag == 1) {
            string fileName = outDir + "/" + subDir + "/" + (boost::format("%015d.jpg") % GetCurrentTimeInMilliseconds()).str();
            cv::imwrite(fileName.c_str(), frame, compression_params);
        }

        int len = frame.total() * frame.channels();
        zmq::message_t message(len);
        memcpy (message.data(), frame.data, len);
        publisher.send (message);

        if (debugMode) {
            clock_t t2 = clock();
            double frameRate = (double) CLOCKS_PER_SEC / (double) (t2 - t1);
            string frameRateStr = "FPS: " + (boost::format("%.1f") % frameRate).str();
            cv::putText(frame, frameRateStr.c_str(), textOrig, cv::FONT_HERSHEY_SIMPLEX, 0.4, CvScalar(0, 0, 255, 0));

            cv::imshow("Publisher on port " + port, frame);
        }

        char c = (char)cvWaitKey(1);
        if(c==27) {
            break;
        }
    }
    return 0;
}
