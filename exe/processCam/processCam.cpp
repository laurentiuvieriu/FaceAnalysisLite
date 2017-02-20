
// this program implements an autonomous camera processor, that handles camera frames and outputs facial analysis estimates
// usage: <executable> <path_to_settings.ini>settings.ini
// Author: RLV (UNITN)
// last update: 20/02/17


// Local includes
#include "LandmarkCoreIncludes.h"

#include <Face_utils.h>
#include <FaceAnalyser.h>
#include <GazeEstimation.h>

#include "ferLocalFunctions.h"
#include "utilsProcessCam.h"
#include <boost/circular_buffer.hpp>

using namespace fer;


int main(int argc, char** argv)
{

    string settings_file = "";

    if (argc != 2){
        cout << "Usage: <path_to_exe>processSub <settings_file>" << endl;
        return 1;
    } else{
        settings_file = argv[1];
    }

    vector<string> arguments = get_arguments_from_file(settings_file);

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

    if (!input_cam.empty())
    {
        input_cam_flag = true;
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
    LandmarkDetector::CLNF face_model(det_parameters.model_location);

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
            cout << "Can't find AU prediction files, exiting" << endl;
            return 1;
        }
    }

    // Creating a  face analyser that will be used for AU extraction
    FaceAnalysis::FaceAnalyser face_analyser(vector<cv::Vec3d>(), 0.7, 112, 112, au_loc, tri_loc);

    boost::circular_buffer<double> painVec(30);
    boost::circular_buffer<double> valenceVec(30);
    vector<double> valenceMap = {-0.81, -0.68, -0.41, -0.12, -0.10, 0.0, 0.9};

    // end of initialization ...

    VideoCapture cap;
    // open the default camera, use something different from 0 otherwise;
    // Check VideoCapture documentation.

    int camId = std::stoi(input_cam[0]);

    if(!cap.open(camId))
        return 0;

    cv::Mat emo_neg  = cv::imread("../../../data/emo_neg_small.jpg"); // 110 x 120 pix
    cv::Mat emo_neu  = cv::imread("../../../data/emo_neu_small.jpg");
    cv::Mat emo_pos  = cv::imread("../../../data/emo_pos_small.jpg");

    while(!done)
    {
        Mat captured_image;
        int total_frames = -1;
        int reported_completion = 0;

        double fps_vid_in = -1.0;

        curr_img++;

        cap >> captured_image;
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
//            double valenceLevel = (double) argMax_valence;

            // Visualising the tracker
            visualise_tracking(captured_image, emo_neu, emo_neg, emo_pos, face_model, det_parameters, gazeDirection0, gazeDirection1, frame_count, fx, fy, cx, cy,
                               face_analyser, painLevel, valenceLevel, output_head_pose, output_AUs_class);

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

            if(input_cam_flag)
            {
                cap >> captured_image;
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

        output_file.close();

        if(output_files.size() > 0 && (output_AUs_reg || output_AUs_class))
        {
            cout << "Postprocessing the Action Unit predictions" << endl;
            post_process_output_file(face_analyser, output_files[f_n], dynamic);
        }
        // Reset the models for the next video
        face_analyser.Reset();
        face_model.Reset();

        frame_count = 0;
        curr_img = -1;
    }

    return 0;
}

