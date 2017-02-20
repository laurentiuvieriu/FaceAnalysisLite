//the launcher is responsible with launching the frame publisher and the two (n?) subscribers that read and process the frames
// it launches all components as separate services and keeps track of their PID
// also, the launcher should specify the comm ports for both the Publisher and Subs (that will themselves publish their results), in addition to its own comm port
// all these are read from the settings.ini file

// Author: RLV (UNITN)
// last update: 20/02/17

#include <iostream>
#include <zmq.hpp>
#include <unistd.h>
#include <boost/algorithm/string.hpp>
#include <filesystem/fstream.hpp>

#include <signal.h>
std::string getErrMsg(int errnum);

using namespace std;

vector<string> get_arguments_from_file(const string fileName);
string get_key_from_arguments(vector<string> &arguments, const string key);
void get_launcher_params(vector<string> &commRoots, vector<string> &commPaths, vector<vector<string>> &commArgs, vector<string> &arguments);
pid_t launch_process (const char *root, const char *path, char *const args[]);

int main (int argc, char** argv) {

    string settings_file = "";

    if (argc != 2){
        cout << "Usage: <path_to_exe>processSub <settings_file>" << endl;
        return 1;
    } else{
        settings_file = argv[1];
    }

    vector<string> arguments = get_arguments_from_file(settings_file);

    //  Prepare our context and socket
    const string serverPort = get_key_from_arguments(arguments, "-serverPort");
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind (serverPort.c_str());

    std::vector <pid_t> pids;

    while (true) {
        zmq::message_t request;

        //  Wait for next request from client
        socket.recv (&request);

        std::string req = std::string(static_cast<char*>(request.data()), request.size());

        if (boost::iequals(req, "Ignore"))
        {
            std::cout << "Received Ignore message --> ignoring ..." << std::endl;
            //  Do some 'work'
            sleep(1);

            //  Send reply back to client
            zmq::message_t reply (6);
            memcpy (reply.data (), "Ignore", 6);
            socket.send (reply);
        }else if (boost::iequals(req, "Start")) {
            std::cout << "Received Start request -> starting ..." << std::endl;

            vector<string> commRoots, commPaths;
            vector<vector<string>> commArgs;

            get_launcher_params(commRoots, commPaths, commArgs, arguments);

            int numComm = commArgs.size();
            vector<vector<char*>> allComms;
            for (int i= 0; i< numComm; i++){
                int commLength = commArgs[i].size();
                vector<char*> buffFull;
                for (int k= 0; k< commLength; k++){
                    char *const buff = const_cast<char*>(commArgs[i][k].c_str());
                    buffFull.push_back(buff);
                }
                buffFull.push_back(0);
                allComms.push_back(buffFull);
            }

            cout << "launcher.cpp: found " << numComm << " commands in the settings file, executing ..." << endl;

            for (int i= 0; i< numComm; i++){
                pid_t pID = launch_process(&commRoots[i].front(), &commPaths[i].front(), &allComms[i].front());
                pids.push_back(pID);
            }

            cout << "...done!" << endl;

            //  Send reply back to client
            zmq::message_t reply (17);
            memcpy (reply.data (), "Processes Started", 17);
            socket.send (reply);
        } else if (boost::iequals(req, "Kill all")) {
            std::cout << "Received Kill request -> starting killing..." << std::endl;
            for (int i= 0; i< pids.size(); i++) {
                cout << "Killing pid  " << pids[i] << " in 3 sec ...";
                sleep(3);
                kill(pids[i], SIGKILL);
                cout << "done!" << std::endl;
            }
            zmq::message_t reply (16);
            memcpy (reply.data (), "Processes Killed", 16);
            socket.send (reply);
        } else if (boost::iequals(req, "Stop")) {
            std::cout << "Received Stoping request -> stopping ..." << std::endl;

            zmq::message_t reply (7);
            memcpy (reply.data (), "Stopped", 7);
            socket.send (reply);
            socket.close();
            return 0;
        } else {
            std::cout << "Unknown request ..." << std::endl;
            //  Do some 'work'
            sleep(1);

            //  Send reply back to client
            zmq::message_t reply (7);
            memcpy (reply.data (), "Unknown", 7);
            socket.send (reply);
        }
    }
    return 0;
}

vector<string> get_arguments_from_file(const string fileName){
    vector<string> arguments;
    boost::filesystem::ifstream file(fileName);
    vector<string> parts;
    if (file.is_open()){
        while (!file.eof()){
            string line;
            getline(file, line, '\n');
            parts.clear();
            if (!(line[0] == '#')){
                boost::split(parts, line, boost::is_any_of(" \n"));
                for (int k = 0; k < parts.size(); k++) {
                    if (!boost::iequals(parts[k], "")) {
                        arguments.push_back(parts[k]);
                    }
                }
            }
        }
        file.close();
    }
    else cout << "Unable to open settings file!" << endl;
    return arguments;
}
string get_key_from_arguments(vector<string> &arguments, const string key){
    string out;
    for(size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i].compare(key) == 0) {
            out = arguments[i + 1];
            break;
        }
    }
    return out;
}

void get_launcher_params(vector<string> &commRoots, vector<string> &commPaths, vector<vector<string>> &commArgs, vector<string> &arguments)
{
    commRoots.clear();
    commPaths.clear();
    commArgs.clear();

    bool* valid = new bool[arguments.size()];

    for (size_t i = 0; i < arguments.size(); ++i)
    {
        valid[i] = true;
    }

    // First check if there is a root argument (so that videos and outputs could be defined more easilly)
//    for (size_t i = 0; i < arguments.size(); ++i)
//    {
//
//        if (arguments[i].compare("-serverPort") == 0)
//        {
//            serverPort = arguments[i + 1];
//            valid[i] = false;
//            valid[i + 1] = false;
//            i++;
//        }
//
//        if (arguments[i].compare("-camPubPort") == 0)
//        {
//            camPubPort = arguments[i + 1];
//            valid[i] = false;
//            valid[i + 1] = false;
//            i++;
//        }
//
//        if (arguments[i].compare("-faceAnalysisPubPort") == 0)
//        {
//            faceAnalysisPubPort = arguments[i + 1];
//            valid[i] = false;
//            valid[i + 1] = false;
//            i++;
//        }
//
//        if (arguments[i].compare("-heartRatePubPort") == 0)
//        {
//            heartRatePubPort = arguments[i + 1];
//            valid[i] = false;
//            valid[i + 1] = false;
//            i++;
//        }
//    }

    int i= 0;
    while (i < arguments.size()- 2)
    {
        if ((arguments[i].compare("-execvProc") == 0) && (valid[i]))
        {
            valid[i] = false;
            commRoots.push_back(arguments[i + 1]);
            valid[i+ 1] = false;
            i++;

            commPaths.push_back(arguments[i+ 1]);
            valid[i+ 1] = false;
            i++;

            string nextWord = arguments[i+ 1];
            vector<string> buff;
            while (nextWord.compare("-end") != 0){
                buff.push_back(nextWord.c_str());
                valid[i+ 1] = false;
                i++;
                nextWord = arguments[i + 1];
            }
            commArgs.push_back(buff);
        }else{
            i++;
        }
    }

    for (int i = arguments.size() - 1; i >= 0; --i)
    {
        if (!valid[i])
        {
            arguments.erase(arguments.begin() + i);
        }
    }

}


pid_t launch_process (const char *root, const char *path, char *const args[]){
    // launching the first subscriber ...
    pid_t pID = fork();
    if (pID == 0)                // child
    {
        errno = 0;
        int ign = chdir(root);
        if (ign != 0){
            cout << "launcher.cpp -> launch_process: Unsuccessful folder change" << endl;
        }
        int execReturn = execv (path, args);
        if(execReturn == -1)
        {
            cout << "Failure! execve error code=" << errno << endl;
            cout << getErrMsg(errno) << endl;
        }
        _exit(0); // If exec fails then exit forked process.
    }

    if (pID < 0)             // failed to fork
    {
        cerr << "Failed to fork" << endl;
    }

    cout << "Process started with PID " << pID << std::endl;
    return pID;
}

std::string getErrMsg(int errnum)
{

    switch ( errnum ) {

#ifdef EACCES
        case EACCES :
        {
            return "EACCES Permission denied";
        }
#endif

#ifdef EPERM
        case EPERM :
        {
            return "EPERM Not super-user";
        }
#endif

#ifdef E2BIG
        case E2BIG :
        {
            return "E2BIG Arg list too long";
        }
#endif

#ifdef ENOEXEC
        case ENOEXEC :
        {
            return "ENOEXEC Exec format error";
        }
#endif

#ifdef EFAULT
        case EFAULT :
        {
            return "EFAULT Bad address";
        }
#endif

#ifdef ENAMETOOLONG
        case ENAMETOOLONG :
        {
            return "ENAMETOOLONG path name is too long     ";
        }
#endif

#ifdef ENOENT
        case ENOENT :
        {
            return "ENOENT No such file or directory";
        }
#endif

#ifdef ENOMEM
        case ENOMEM :
        {
            return "ENOMEM Not enough core";
        }
#endif

#ifdef ENOTDIR
        case ENOTDIR :
        {
            return "ENOTDIR Not a directory";
        }
#endif

#ifdef ELOOP
        case ELOOP :
        {
            return "ELOOP Too many symbolic links";
        }
#endif

#ifdef ETXTBSY
        case ETXTBSY :
        {
            return "ETXTBSY Text file busy";
        }
#endif

#ifdef EIO
        case EIO :
        {
            return "EIO I/O error";
        }
#endif

#ifdef ENFILE
        case ENFILE :
        {
            return "ENFILE Too many open files in system";
        }
#endif

#ifdef EINVAL
        case EINVAL :
        {
            return "EINVAL Invalid argument";
        }
#endif

#ifdef EISDIR
        case EISDIR :
        {
            return "EISDIR Is a directory";
        }
#endif

#ifdef ELIBBAD
        case ELIBBAD :
        {
            return "ELIBBAD Accessing a corrupted shared lib";
        }
#endif

        default :
        {
            std::string errorMsg(strerror(errnum));
            if ( errnum ) return errorMsg;
        }
    }
}

