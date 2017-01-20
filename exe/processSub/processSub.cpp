//
// Created by radu on 22/09/16.
//


// Local includes
#include "LandmarkCoreIncludes.h"

#include <Face_utils.h>
#include <FaceAnalyser.h>
#include <GazeEstimation.h>

#include "ferLocalFunctions.h"
#include "utilsProcessSub.h"
//#include "utilsProcessCam.h"
#include <zmq.hpp>
#include "zhelpers.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>

using namespace fer;

void get_double_from_arguments(vector<string> &arguments, string key, double& keyval) {

    bool* valid = new bool[arguments.size()];
    valid[0] = true;

    // First check if there is a root argument (so that videos and outputs could be defined more easilly)
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i].compare(key) == 0) {
            stringstream data(arguments[i + 1]);
            data >> keyval;
            break;
        }
    }
}

void read_arguments(vector<string> &arguments, string &pubPort, string &subPort, string &settingsFile, bool &debugMode) {

    bool* valid = new bool[arguments.size()];
    valid[0] = true;

    // First check if there is a root argument (so that videos and outputs could be defined more easilly)
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i].compare("-pubPort") == 0) {
            pubPort = arguments[i+ 1];
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
        if (arguments[i].compare("-settings") == 0) {
            settingsFile = arguments[i + 1];
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


int main(int argc, char** argv)
{
//  Usage: <path_to_exe>processSub -pubPort <cam pubPort> -settings <settings_file>

    vector<string> argumentsComm = get_arguments(argc, argv);

    string pubPort = "";
    string subPort = "";
    string settings_file = "";
    bool debugMode = false;

    read_arguments(argumentsComm, pubPort, subPort, settings_file, debugMode);

    if (strcmp(pubPort.c_str(), "") == 1){
        pubPort = "tcp://*:5556";
        cout << "processSub.cpp -> main: Unspecified publisher port (-pubPort <port address>), using default value: " << pubPort << endl;
    }

    if (strcmp(subPort.c_str(), "") == 1){
        subPort = "tcp://*:6000";
        cout << "processSub.cpp -> main: Unspecified subscriber port (-subPort <port address>), using default value: " << subPort << endl;
    }

    if (strcmp(settings_file.c_str(), "") == 1){
        cout << "processSub.cpp -> main: Unspecified settings file (-settings <settings file>), aborting... " << endl;
        return 0;
    }

    vector<string> arguments = get_arguments_from_file(settings_file);

//    double expected_fps = 30;
//    get_double_from_arguments(arguments, "-expected_fps", expected_fps);

    // initialization procedure ...
    // Some initial parameters that can be overriden from command line
    vector<string> input_pubId, input_cam, input_files, depth_directories, output_files, tracked_videos_output;

    LandmarkDetector::FaceModelParameters det_parameters(arguments);
    // Always track gaze in feature extraction
    det_parameters.track_gaze = true;

    // Get the input output file parameters

    // Indicates that rotation should be with respect to camera or world coordinates
    bool use_world_coordinates;
    string output_codec; //not used but should
    LandmarkDetector::get_video_input_output_params(input_pubId, input_cam, input_files, depth_directories, output_files, tracked_videos_output, use_world_coordinates, output_codec, arguments);

    bool video_input = false;
    bool verbose = true;
    bool images_as_video = true;
    bool input_cam_flag = false;
    bool pub_sub_flag = false;

    if (!input_cam.empty())
    {
        input_cam_flag = true;
    }

    if (!input_pubId.empty()){
        pub_sub_flag = true;
    }

    if (input_cam_flag && pub_sub_flag){
        cout << "Only one source of information is accepted! (either camera, or publisher frames)" << endl;
        return 1;
    }

    // Grab camera parameters, if they are not defined (approximate values will be used)
    float fx = 0, fy = 0, cx = 0, cy = 0;
    int d = 0;
    // Get camera parameters
    LandmarkDetector::get_camera_params(d, fx, fy, cx, cy, arguments);

    // If cx (optical axis centre) is undefined will use the image size/2 as an estimate
    bool cx_undefined = false;
    bool fx_undefined = false;
    if (cx == 0 || cy == 0)
    {
        cx_undefined = true;
    }
    if (fx == 0 || fy == 0)
    {
        fx_undefined = true;
    }

    // The modules that are being used for tracking
    cout << "Initializing face model ...";
    LandmarkDetector::CLNF face_model(det_parameters.model_location);
    cout << "done" << endl;

    vector<string> output_similarity_align;
    vector<string> output_hog_align_files;

    double sim_scale = 0.7;
    int sim_size = 112;
    bool grayscale = false;
    bool video_output = false;
    bool dynamic = true; // Indicates if a dynamic AU model should be used (dynamic is useful if the video is long enough to include neutral expressions)
    int num_hog_rows;
    int num_hog_cols;

    // output parameters
    bool output_2D_landmarks = false;
    bool output_3D_landmarks = false;
    bool output_model_params = false;
    bool output_frame_idx = false;
    bool output_timestamp = false;
    bool output_confidence = false;
    bool output_success = false;
    bool output_head_position = false;
    bool output_head_pose = false;
    bool output_AUs_reg = false;
    bool output_AUs_class = false;
    bool output_gaze = false;
    bool output_pain_level = false;
    bool output_valence = false;

    get_output_feature_params(output_similarity_align, output_hog_align_files, sim_scale, sim_size, grayscale, verbose, dynamic,
                              output_2D_landmarks, output_3D_landmarks, output_model_params, output_frame_idx, output_timestamp,
                              output_confidence, output_success, output_head_position, output_head_pose, output_AUs_reg, output_AUs_class,
                              output_gaze, output_pain_level, output_valence, arguments);

    // Used for image masking

    // if output_pain_level is on, then load the pain models too ...
    paramList params_pain;
    const string featDataRoot_pain = get_key_from_arguments(arguments, "-rootDir_pain");//"/home/radu/work/cpp/20160922_OpenFace/OpenFace/exe/processCam/data/params_pain/";
    RV_readParamList(featDataRoot_pain, &params_pain);

    vector<randomTree> forest_pain;
    const string treesDir_pain = featDataRoot_pain+ "trees/";
    forest_pain = RV_readForest(treesDir_pain, params_pain);

    paramList params_valence;
    const string featDataRoot_valence = get_key_from_arguments(arguments, "-rootDir_valence");//"/home/radu/work/cpp/20160922_OpenFace/OpenFace/exe/processCam/data/params_pain/";
    RV_readParamList(featDataRoot_valence, &params_valence);

    vector<randomTree> forest_valence;
    const string treesDir_valence = featDataRoot_valence+ "trees/";
    forest_valence = RV_readForest(treesDir_valence, params_valence);

    string tri_loc;
    if(boost::filesystem::exists(path("model/tris_68_full.txt"))){
        tri_loc = "model/tris_68_full.txt";
    }
    else{
        path loc = path(arguments[0]).parent_path() / "model/tris_68_full.txt";
        tri_loc = loc.string();

        if(!exists(loc)){
            cout << "Can't find triangulation files, exiting" << endl;
            return 1;
        }
    }

    // Will warp to scaled mean shape
    cv::Mat_<double> similarity_normalised_shape = face_model.pdm.mean_shape * sim_scale;
    // Discard the z component
    similarity_normalised_shape = similarity_normalised_shape(cv::Rect(0, 0, 1, 2*similarity_normalised_shape.rows/3)).clone();

    // If multiple video files are tracked, use this to indicate if we are done
    bool done = false;
    int f_n = 0;
    int curr_img = -1;

    string au_loc;

    string au_loc_local;
    if (dynamic){
        au_loc_local = "AU_predictors/AU_all_best.txt";
    }
    else{
        au_loc_local = "AU_predictors/AU_all_static.txt";
    }

    if(boost::filesystem::exists(path(au_loc_local))){
        au_loc = au_loc_local;
    }
    else{
        path loc = path(arguments[0]).parent_path() / au_loc_local;
        if(exists(loc)){
            au_loc = loc.string();
        }
        else{
            cout << "Can't find AU prediction files, exiting ..." << endl;
            return 1;
        }
    }

    // Creating a face analyser that will be used for AU extraction
    FaceAnalysis::FaceAnalyser face_analyser(vector<cv::Vec3d>(), 0.7, 112, 112, au_loc, tri_loc);

    // ----------------------------------------------------------------------
    // finally subscribe to the pub Id ...
    zmq::context_t context (1);

    //  Socket to talk to server
    std::cout << "Collecting updates from server…\n" << std::endl;
    zmq::socket_t subscriber (context, ZMQ_SUB);
//    const char* pub_adress = input_pubId[0].c_str();

    uint64_t RCVBUF = 500000;
    subscriber.setsockopt(ZMQ_RCVBUF, &RCVBUF, sizeof(RCVBUF));
    uint64_t HWM = 1;
    subscriber.setsockopt(ZMQ_HWM, &HWM, sizeof(HWM));
    subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);

    subscriber.connect(pubPort.c_str());
//    subscriber.connect(pub_adress);

    // end of subscribing to publisher
    // ----------------------------------------------------------------------
    // now launch a publisher port for broadcasting results
    zmq::context_t contextPub (1);

    //  Socket to talk to server
    std::cout << "Creating publisher context \n" << std::endl;
    zmq::socket_t publisher (context, ZMQ_PUB);
//    const char* pub_adress = input_pubId[0].c_str();

//    uint64_t SNDBUF = 5000;
//    publisher.setsockopt(ZMQ_SNDBUF, &SNDBUF, sizeof(SNDBUF));
//    publisher.setsockopt(ZMQ_HWM, &HWM, sizeof(HWM));
    publisher.bind(subPort.c_str());
    // end of creating context for the publisher part
    // ----------------------------------------------------------------------

    boost::circular_buffer<double> painVec(30);
    boost::circular_buffer<double> valenceVec(30);
    vector<double> valenceMap = {-0.81, -0.68, -0.41, -0.12, -0.10, 0.0, 0.9};

    // end of initialization ...

    while(!done)
    {
        zmq::message_t message;
        Mat captured_image(480, 640, CV_8SC3);
        int total_frames = -1;
        int reported_completion = 0;

        double fps_vid_in = -1.0;

        curr_img++;

        subscriber.recv(&message);
        memcpy(captured_image.data, message.data(), message.size());
        captured_image.convertTo(captured_image, CV_8U);
//        cap >> captured_image;
        if( captured_image.empty() ) break;

        // If optical centers are not defined just use center of image
        if(cx_undefined){
            cx = captured_image.cols / 2.0f;
            cy = captured_image.rows / 2.0f;
        }

        // Use a rough guess-timate of focal length
        if (fx_undefined){
            fx = 500 * (captured_image.cols / 640.0);
            fy = 500 * (captured_image.rows / 480.0);

            fx = (fx + fy) / 2.0;
            fy = fx;
        }

        // Creating output files
        std::ofstream output_file;

        if (!output_files.empty()){
            output_file.open(output_files[f_n], ios_base::out);
            prepareOutputFile(&output_file, output_2D_landmarks, output_3D_landmarks, output_model_params, output_frame_idx,
                              output_timestamp, output_confidence, output_success, output_head_position, output_head_pose, output_AUs_reg, output_AUs_class,
                              output_gaze, output_pain_level, face_model.pdm.NumberOfPoints(), face_model.pdm.NumberOfModes(), face_analyser.GetAUClassNames(),
                              face_analyser.GetAURegNames());
        }

        // Saving the HOG features
        std::ofstream hog_output_file;
        if(!output_hog_align_files.empty())
        {
            hog_output_file.open(output_hog_align_files[f_n], ios_base::out | ios_base::binary);
        }

        // saving the videos
        cv::VideoWriter writerFace;
        if(!tracked_videos_output.empty())
        {
            try
            {
                writerFace.open(tracked_videos_output[f_n], CV_FOURCC(output_codec[0],output_codec[1],output_codec[2],output_codec[3]), 30, captured_image.size(), true);
            }
            catch(cv::Exception e)
            {
                WARN_STREAM( "Could not open VideoWriter, OUTPUT FILE WILL NOT BE WRITTEN. Currently using codec " << output_codec << ", try using an other one (-oc option)");
            }


        }
        int frame_count = 0;

        // This is useful for a second pass run (if want AU predictions)
        vector<cv::Vec6d> params_global_video;
        vector<bool> successes_video;
        vector<cv::Mat_<double>> params_local_video;
        vector<cv::Mat_<double>> detected_landmarks_video;

        // Use for timestamping if using a webcam
        int64 t_initial = cv::getTickCount();

        bool visualise_hog = verbose;

        // Timestamp in seconds of current processing
        double time_stamp = 0;

        INFO_STREAM( "Starting tracking");
        while(!captured_image.empty())
        {

            // Grab the timestamp first
            if (video_input)
            {
                time_stamp = (double)frame_count * (1.0 / fps_vid_in);
            }
            else
            {
                // if loading images assume 30fps
                time_stamp = (double)frame_count * (1.0 / 30.0);
            }

            // Reading the images
            cv::Mat_<uchar> grayscale_image;

            if(captured_image.channels() == 3)
            {
                cvtColor(captured_image, grayscale_image, CV_BGR2GRAY);
            }
            else
            {
                grayscale_image = captured_image.clone();
            }

            // The actual facial landmark detection / tracking
            bool detection_success;

            if(video_input || images_as_video)
            {
                detection_success = LandmarkDetector::DetectLandmarksInVideo(grayscale_image, face_model, det_parameters);
            }
            else
            {
                detection_success = LandmarkDetector::DetectLandmarksInImage(grayscale_image, face_model, det_parameters);
            }

            // Gaze tracking, absolute gaze direction
            cv::Point3f gazeDirection0(0, 0, -1);
            cv::Point3f gazeDirection1(0, 0, -1);

            if (det_parameters.track_gaze && detection_success && face_model.eye_model)
            {
                FaceAnalysis::EstimateGaze(face_model, gazeDirection0, fx, fy, cx, cy, true);
                FaceAnalysis::EstimateGaze(face_model, gazeDirection1, fx, fy, cx, cy, false);
            }

            // Do face alignment
            cv::Mat sim_warped_img;
            cv::Mat_<double> hog_descriptor;

            // But only if needed in output
            if(!output_similarity_align.empty() || hog_output_file.is_open() || output_AUs_reg || output_AUs_class || output_pain_level)
            {
                face_analyser.AddNextFrame(captured_image, face_model, time_stamp, false, !det_parameters.quiet_mode);
                face_analyser.GetLatestAlignedFace(sim_warped_img);

//                if(!det_parameters.quiet_mode)
//                {
//                    cv::imshow("sim_warp", sim_warped_img);
//                }
                if(hog_output_file.is_open())
                {
                    FaceAnalysis::Extract_FHOG_descriptor(hog_descriptor, sim_warped_img, num_hog_rows, num_hog_cols);

                    if(visualise_hog && !det_parameters.quiet_mode)
                    {
                        cv::Mat_<double> hog_descriptor_vis;
                        FaceAnalysis::Visualise_FHOG(hog_descriptor, num_hog_rows, num_hog_cols, hog_descriptor_vis);
                        cv::imshow("hog", hog_descriptor_vis);
                    }
                }
            }

            // Work out the pose of the head from the tracked model
            cv::Vec6d pose_estimate;
            if(use_world_coordinates)
            {
                pose_estimate = LandmarkDetector::GetCorrectedPoseWorld(face_model, fx, fy, cx, cy);
            }
            else
            {
                pose_estimate = LandmarkDetector::GetCorrectedPoseCamera(face_model, fx, fy, cx, cy);
            }

            if(hog_output_file.is_open())
            {
                output_HOG_frame(&hog_output_file, detection_success, hog_descriptor, num_hog_rows, num_hog_cols);
            }

            // Write the similarity normalised output
            if(!output_similarity_align.empty())
            {

                if (sim_warped_img.channels() == 3 && grayscale)
                {
                    cvtColor(sim_warped_img, sim_warped_img, CV_BGR2GRAY);
                }

                char name[100];

                // output the frame number
                std::sprintf(name, "frame_det_%06d.bmp", frame_count);

                // Construct the output filename
                boost::filesystem::path slash("/");

                std::string preferredSlash = slash.make_preferred().string();

                string out_file = output_similarity_align[f_n] + preferredSlash + string(name);
                bool write_success = imwrite(out_file, sim_warped_img);

                if (!write_success)
                {
                    cout << "Could not output similarity aligned image image" << endl;
                    return 1;
                }
            }

            // run here the pain detector
//            cv::Mat grayIn;
//            cvtColor(sim_warped_img, grayIn, CV_BGR2GRAY);
//            grayIn.convertTo(grayIn, CV_64FC1);
//
//            Mat featData;
//            RV_featExtractionWholeFrame(grayIn, params_pain, featData);
//            vector<double> pred = RV_testForest(featData, forest_pain, params_pain, 16);

            double instant_painLevel = 0;
            double instant_valence = 0;
            int argMax_valence = 0;

            if (output_pain_level) {
                instant_painLevel = get_pain_level_from_face(face_analyser, forest_pain, params_pain);
            }

            if (output_valence) {
                vector<double> valence_pred;
                valence_pred = get_valence_from_face(face_analyser, forest_valence, params_valence);
//                instant_valence = std::inner_product(valenceMap.begin(), valenceMap.end(), valence_pred.begin(), 0.0);

//                double max_valence = *std::max_element(valence_pred.begin(), valence_pred.end());
                argMax_valence = std::distance(valence_pred.begin(), std::max_element(valence_pred.begin(), valence_pred.end()));
                instant_valence = valenceMap[argMax_valence];
            }

            painVec.push_back(instant_painLevel);
            double pain_sum = std::accumulate(painVec.begin(), painVec.end(), 0.0);
            double painLevel = pain_sum/painVec.size();

            valenceVec.push_back(instant_valence);
            double valence_sum = std::accumulate(valenceVec.begin(), valenceVec.end(), 0.0);
            double valenceLevel = valence_sum/valenceVec.size();

            cv::Vec6d pose_estimate_to_draw = LandmarkDetector::GetCorrectedPoseWorld(face_model, fx, fy, cx, cy);

            double hp_tilt = (double) pose_estimate_to_draw[3] * (-1) * 180 / CV_PI;
            s_sendmore(publisher, "headTilt");
            s_send(publisher, boost::lexical_cast<std::string>(hp_tilt));

            double hp_yaw = (double) pose_estimate_to_draw[4] * 180 / CV_PI;
            s_sendmore(publisher, "headYaw");
            s_send(publisher, boost::lexical_cast<std::string>(hp_yaw));

            double hp_roll = (double) pose_estimate_to_draw[5] * 180 / CV_PI;
            s_sendmore(publisher, "headRoll");
            s_send(publisher, boost::lexical_cast<std::string>(hp_roll));

            s_sendmore(publisher, "valenceLevel");
            s_send(publisher, boost::lexical_cast<std::string>(valenceLevel));
            s_sendmore(publisher, "painLevel");
            s_send(publisher, boost::lexical_cast<std::string>(painLevel));

            // Visualising the tracker
            if (debugMode) {
                visualise_tracking(captured_image, face_model, det_parameters, gazeDirection0, gazeDirection1,
                                   frame_count, fx, fy, cx, cy,
                                   face_analyser, painLevel, valenceLevel, output_head_pose, output_AUs_class);
            }

            // Output the landmarks, pose, gaze, parameters and AUs
            outputAllFeatures(&output_file, output_2D_landmarks, output_3D_landmarks, output_model_params, output_frame_idx,
                              output_timestamp, output_confidence, output_success, output_head_position, output_head_pose, output_AUs_reg, output_AUs_class,
                              output_gaze, output_pain_level, output_valence, face_model, frame_count, time_stamp, detection_success, gazeDirection0, gazeDirection1,
                              pose_estimate, fx, fy, cx, cy, painLevel, valenceLevel, face_analyser);

            // output the tracked video
            if(!tracked_videos_output.empty())
            {
                writerFace << captured_image;
            }

            if(pub_sub_flag)
            {
                subscriber.recv(&message);
                memcpy(captured_image.data, message.data(), message.size());
            }
            else
            {
                captured_image = cv::Mat();
            }
            // detect key presses
            char character_press = cv::waitKey(1);

            // restart the tracker

            if(character_press == 'r')
            {
                face_model.Reset();
            }

            if (character_press=='d')
            {
                captured_image = cv::Mat();
                done = true;
            }
                // quit the application
            if(character_press=='q')
            {
                return(0);
            }

            // Update the frame count
            frame_count++;

        }

        if (!output_files.empty()) {
            output_file.close();
        }

//        if(output_files.size() > 0 && (output_AUs_reg || output_AUs_class))
//        {
//            cout << "Postprocessing the Action Unit predictions" << endl;
//            post_process_output_file(face_analyser, output_files[f_n], dynamic);
//        }
        // Reset the models for the next video
        face_analyser.Reset();
        face_model.Reset();

        frame_count = 0;
        curr_img = -1;
    }

    return 0;
}

