//
// Created by radu on 07/10/16.
//

#include "utilsProcessCam.h"

double fps_tracker = -1.0;
int64 t0 = 0;

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

// Useful utility for creating directories for storing the output files
void create_directory_from_file(string output_path)
{

    // Creating the right directory structure

    // First get rid of the file
    auto p = path(path(output_path).parent_path());

    if(!p.empty() && !boost::filesystem::exists(p))
    {
        bool success = boost::filesystem::create_directories(p);
        if(!success)
        {
            cout << "Failed to create a directory... " << p.string() << endl;
        }
    }
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

void visualise_tracking(cv::Mat& captured_image, cv::Mat& emo_neu, cv::Mat& emo_neg, cv::Mat& emo_pos, const LandmarkDetector::CLNF& face_model,
                        const LandmarkDetector::FaceModelParameters& det_parameters,
                        cv::Point3f gazeDirection0, cv::Point3f gazeDirection1, int frame_count, double fx, double fy, double cx, double cy,
                        const FaceAnalysis::FaceAnalyser& face_analyser, double painLevel, double valenceLevel, bool output_head_pose, bool output_AUs_class)
{

    // Drawing the facial landmarks on the face and the bounding box around it if tracking is successful and initialised
    double detection_certainty = face_model.detection_certainty;
    bool detection_success = face_model.detection_success;

    double visualisation_boundary = 0.2;

    // Only draw if the reliability is reasonable, the value is slightly ad-hoc
    if (detection_certainty < visualisation_boundary)
    {
//        LandmarkDetector::Draw(captured_image, face_model);

//        double vis_certainty = detection_certainty;
//        if (vis_certainty > 1)
//            vis_certainty = 1;
//        if (vis_certainty < -1)
//            vis_certainty = -1;

//        vis_certainty = (vis_certainty + 1) / (visualisation_boundary + 1);

        // A rough heuristic for box around the face width
//        int thickness = (int)std::ceil(2.0* ((double)captured_image.cols) / 640.0);

        // head position and orientation ==========================================================
        if (output_head_pose) {
            cv::Vec6d pose_estimate_to_draw = LandmarkDetector::GetCorrectedPoseWorld(face_model, fx, fy, cx, cy);
            vector<string> poseLabs;
            poseLabs.push_back("Pos_Tx ");
            poseLabs.push_back("Pos_Ty ");
            poseLabs.push_back("Pos_Tz ");
            poseLabs.push_back("Pos_Rx (tilt) ");
            poseLabs.push_back("Pos_Ry (yaw) ");
            poseLabs.push_back("Pos_Rz (roll) ");
//
            for (int kk = 3; kk < 6; kk++) {
                char poseC[255];
                double poseVal;
                if (kk >= 3) {
                    poseVal = (double) pose_estimate_to_draw[kk] * 180 / CV_PI;
                } else {
                    poseVal = (double) pose_estimate_to_draw[kk];
                }

                if (kk == 3) {
                    poseVal = (-1.0) * poseVal; // aligning tilt to the protocol
                }

                std::sprintf(poseC, ": %.1f", poseVal);
                string poseSt(poseLabs[kk]);
                poseSt += poseC;
                cv::putText(captured_image, poseSt, cv::Point(10, 20 * (kk + 3)), CV_FONT_HERSHEY_SIMPLEX, 0.5,
                            CV_RGB(255, 0, 0), 1, CV_AA);
            }
        }

        if (output_AUs_class) {
            auto aus_class = face_analyser.GetCurrentAUsClass();

            vector<string> au_class_names = face_analyser.GetAUClassNames();
            std::sort(au_class_names.begin(), au_class_names.end());

            // write out ar the correct index
            int kIdx = 1;
            for (string au_name : au_class_names) {
                char auC[255];

                for (auto au_class : aus_class) {
                    if (au_name.compare(au_class.first) == 0) {
                        std::sprintf(auC, ": %d", (int) au_class.second);
                        string auSt(au_name);
                        auSt += auC;
//                        if ((int) au_class.second == 1) {
                        cv::putText(captured_image, auSt, cv::Point(540, 20 * kIdx), CV_FONT_HERSHEY_SIMPLEX, 0.5,
                                    CV_RGB(255, 0, 0), 1, CV_AA);
                        kIdx++;
//                        }
                    }
                }
            }
        }

        // Draw it in reddish if uncertain, blueish if certain
//        LandmarkDetector::DrawBox(captured_image, pose_estimate_to_draw, cv::Scalar((1 - vis_certainty)*255.0, 0, vis_certainty * 255), thickness, fx, fy, cx, cy);

//        if (det_parameters.track_gaze && detection_success && face_model.eye_model)
//        {
//            FaceAnalysis::DrawGaze(captured_image, face_model, gazeDirection0, gazeDirection1, fx, fy, cx, cy);
//        }
    }

    // Work out the framerate
    if (frame_count % 10 == 0)
    {
        double t1 = cv::getTickCount();
        fps_tracker = 10.0 / (double(t1 - t0) / cv::getTickFrequency());
        t0 = t1;
    }

    // Write out the framerate on the image before displaying it
    char fpsC[255];
    std::sprintf(fpsC, "%d", (int)fps_tracker);
    string fpsSt("FPS:");
    fpsSt += fpsC;
    cv::putText(captured_image, fpsSt, cv::Point(10, 20), CV_FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 0, 0), 1, CV_AA);

    string painLevelStr = "Pain: " + (boost::format("%.2f") % painLevel).str();
    cv::putText(captured_image, painLevelStr, cv::Point(10, 40), CV_FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 0, 0), 1, CV_AA);

//    vector<string> emotions = {"NaN", "Sadness", "Disgust", "Anger", "Fear", "Surprise", "Neutral", "Happyness"};

    string valenceStr = "Valence: " + (boost::format("%.2f") % valenceLevel).str(); //emotions[(int)valenceLevel];
    cv::putText(captured_image, valenceStr, cv::Point(10, 60), CV_FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 0, 0), 1, CV_AA);

    cv::Size t = emo_neu.size();
    cv::Size ci = captured_image.size();
    Mat roi(captured_image, cv::Rect(ci.width- t.width, ci.height- t.height, t.width, t.height));

    if (valenceLevel <= -0.3){
        emo_neg.copyTo(roi);
    }else if (valenceLevel >= 0.3){
        emo_pos.copyTo(roi);
    }else {
        emo_neu.copyTo(roi);
    }

//    cvPutText(captured_image, painLevelStr.c_str(), cvPoint(10, 30), &font, cvScalar(255, 255, 255, 0));

    if (!det_parameters.quiet_mode)
    {
        cv::namedWindow("tracking_result", 1);
        cv::imshow("tracking_result", captured_image);
    }
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

// Allows for post processing of the AU signal
void post_process_output_file(FaceAnalysis::FaceAnalyser& face_analyser, string output_file, bool dynamic)
{

    vector<double> certainties;
    vector<bool> successes;
    vector<double> timestamps;
    vector<std::pair<std::string, vector<double>>> predictions_reg;
    vector<std::pair<std::string, vector<double>>> predictions_class;

    // Construct the new values to overwrite the output file with
    face_analyser.ExtractAllPredictionsOfflineReg(predictions_reg, certainties, successes, timestamps, dynamic);
    face_analyser.ExtractAllPredictionsOfflineClass(predictions_class, certainties, successes, timestamps, dynamic);

    int num_class = predictions_class.size();
    int num_reg = predictions_reg.size();

    // Extract the indices of writing out first
    vector<string> au_reg_names = face_analyser.GetAURegNames();
    std::sort(au_reg_names.begin(), au_reg_names.end());
    vector<int> inds_reg;

    // write out ar the correct index
    for (string au_name : au_reg_names)
    {
        for (int i = 0; i < num_reg; ++i)
        {
            if (au_name.compare(predictions_reg[i].first) == 0)
            {
                inds_reg.push_back(i);
                break;
            }
        }
    }

    vector<string> au_class_names = face_analyser.GetAUClassNames();
    std::sort(au_class_names.begin(), au_class_names.end());
    vector<int> inds_class;

    // write out ar the correct index
    for (string au_name : au_class_names)
    {
        for (int i = 0; i < num_class; ++i)
        {
            if (au_name.compare(predictions_class[i].first) == 0)
            {
                inds_class.push_back(i);
                break;
            }
        }
    }
    // Read all of the output file in
    vector<string> output_file_contents;

    std::ifstream infile(output_file);
    string line;

    while (std::getline(infile, line))
        output_file_contents.push_back(line);

    infile.close();

    // Read the header and find all _r and _c parts in a file and use their indices
    std::vector<std::string> tokens;
    boost::split(tokens, output_file_contents[0], boost::is_any_of(","));

    int begin_ind = -1;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        if (tokens[i].find("AU") != string::npos && begin_ind == -1)
        {
            begin_ind = i;
            break;
        }
    }
    int end_ind = begin_ind + num_class + num_reg;

    // Now overwrite the whole file
    std::ofstream outfile(output_file, ios_base::out);
    // Write the header
    outfile << output_file_contents[0].c_str() << endl;

    // Write the contents
    for (int i = 1; i < (int)output_file_contents.size(); ++i)
    {
        std::vector<std::string> tokens;
        boost::split(tokens, output_file_contents[i], boost::is_any_of(","));

        outfile << tokens[0];

        for (int t = 1; t < (int)tokens.size(); ++t)
        {
            if (t >= begin_ind && t < end_ind)
            {
                if(t - begin_ind < num_reg)
                {
                    outfile << ", " << predictions_reg[inds_reg[t - begin_ind]].second[i - 1];
                }
                else
                {
                    outfile << ", " << predictions_class[inds_class[t - begin_ind - num_reg]].second[i - 1];
                }
            }
            else
            {
                outfile << ", " << tokens[t];
            }
        }
        outfile << endl;
    }


}

void prepareOutputFile(std::ofstream* output_file, bool output_2D_landmarks, bool output_3D_landmarks,
                       bool output_model_params, bool output_frame_idx, bool output_timestamp, bool output_confidence,
                       bool output_success, bool output_head_position, bool output_head_pose, bool output_AUs_reg, bool output_AUs_class, bool output_gaze, bool output_pain_level,
                       int num_landmarks, int num_model_modes, vector<string> au_names_class, vector<string> au_names_reg)
{

    string prefferedDelimiter = ";";
    bool firstFlag = true;
    if (output_frame_idx){
        if (firstFlag) {
            *output_file << "frameIdx";
            firstFlag = false;
        }else{
            *output_file << prefferedDelimiter << "frameIdx";
        }
    }

    if (output_timestamp){
        if (firstFlag) {
            *output_file << "timestamp";
            firstFlag = false;
        }else{
            *output_file << prefferedDelimiter << "timestamp";
        }
    }

    if (output_confidence){
        if (firstFlag) {
            *output_file << "confidence";
            firstFlag = false;
        }else{
            *output_file << prefferedDelimiter << "confidence";
        }
    }

    if (output_success){
        if (firstFlag) {
            *output_file << "success";
        }else{
            *output_file << prefferedDelimiter << "success";
        }
    }

//    *output_file << "frame, timestamp, confidence, success";

    if (output_gaze)
    {
        *output_file << prefferedDelimiter << "gaze_0_x" << prefferedDelimiter << "gaze_0_y" << prefferedDelimiter << "gaze_0_z"
                     << prefferedDelimiter << "gaze_1_x" << prefferedDelimiter << "gaze_1_y" << prefferedDelimiter << "gaze_2_z";
    }

    if (output_head_position)
    {
        *output_file << prefferedDelimiter << "pose_Tx" << prefferedDelimiter << "pose_Ty" << prefferedDelimiter << "pose_Tz";
    }

    if (output_head_pose)
    {
        *output_file << prefferedDelimiter << "Yaw (Ry)" << prefferedDelimiter << "Tilt (Rx)" << prefferedDelimiter << "Roll (Rz)";
    }

    if (output_2D_landmarks)
    {
        for (int i = 0; i < num_landmarks; ++i)
        {
            *output_file << prefferedDelimiter << "x_" << i;
        }
        for (int i = 0; i < num_landmarks; ++i)
        {
            *output_file << prefferedDelimiter << "y_" << i;
        }
    }

    if (output_3D_landmarks)
    {
        for (int i = 0; i < num_landmarks; ++i)
        {
            *output_file << prefferedDelimiter << "X_" << i;
        }
        for (int i = 0; i < num_landmarks; ++i)
        {
            *output_file << prefferedDelimiter << "Y_" << i;
        }
        for (int i = 0; i < num_landmarks; ++i)
        {
            *output_file << prefferedDelimiter << "Z_" << i;
        }
    }

    // Outputting model parameters (rigid and non-rigid), the first parameters are the 6 rigid shape parameters, they are followed by the non rigid shape parameters
    if (output_model_params)
    {
        *output_file << prefferedDelimiter << "p_scale"
                     << prefferedDelimiter << "p_rx" << prefferedDelimiter << "p_ry" << prefferedDelimiter << "p_rz"
                     << prefferedDelimiter << "p_tx" << prefferedDelimiter << "p_ty";
        for (int i = 0; i < num_model_modes; ++i)
        {
            *output_file << prefferedDelimiter << "p_" << i;
        }
    }

    if (output_AUs_reg) {
        std::sort(au_names_reg.begin(), au_names_reg.end());
        for (string reg_name : au_names_reg) {
            *output_file << prefferedDelimiter << reg_name << "_r";
        }
    }
    if (output_AUs_class){
        std::sort(au_names_class.begin(), au_names_class.end());
        for (string class_name : au_names_class)
        {
            *output_file << prefferedDelimiter << class_name << "_c";
        }
    }
    if (output_pain_level){
        *output_file << prefferedDelimiter << "pain_level";
    }

    *output_file << endl;

}

// Output all of the information into one file in one go (quite a few parameters, but simplifies the flow)
void outputAllFeatures(std::ofstream *output_file, bool output_2D_landmarks, bool output_3D_landmarks,
                       bool output_model_params, bool output_frame_idx, bool output_timestamp, bool output_confidence,
                       bool output_success, bool output_head_position, bool output_head_pose, bool output_AUs_reg, bool output_AUs_class,
                       bool output_gaze, bool output_pain_level, bool output_valence, const LandmarkDetector::CLNF &face_model, int frame_count, double time_stamp,
                       bool detection_success, cv::Point3f gazeDirection0, cv::Point3f gazeDirection1,
                       const cv::Vec6d &pose_estimate, double fx, double fy, double cx, double cy, double painLevel, double valenceLevel,
                       const FaceAnalysis::FaceAnalyser &face_analyser)
{
    string prefferedDelimiter = ";";

    double confidence = 0.5 * (1 - face_model.detection_certainty);
    bool firstFlag = true;
    if (output_frame_idx){
        if (firstFlag) {
            *output_file << boost::format("%06d") % (frame_count + 1);
            firstFlag = false;
        }else{
            *output_file << prefferedDelimiter << boost::format("%06d") % (frame_count + 1);
        }
    }

    if (output_timestamp){
        if (firstFlag) {
            *output_file << boost::format("%04.3f") % time_stamp;
            firstFlag = false;
        }else{
            *output_file << prefferedDelimiter << boost::format("%04.3f") % time_stamp;
        }
    }

    if (output_confidence){
        if (firstFlag) {
            *output_file << boost::format("%.3f") % confidence;
            firstFlag = false;
        }else{
            *output_file << prefferedDelimiter << boost::format("%.3f") % confidence;
        }
    }

    if (output_success){
        if (firstFlag) {
            *output_file << detection_success;
        }else{
            *output_file << prefferedDelimiter << detection_success;
        }
    }

//    *output_file << frame_count + 1 << ", " << time_stamp << ", " << confidence << ", " << detection_success;

    // Output the estimated gaze
    if (output_gaze)
    {
        *output_file << prefferedDelimiter << gazeDirection0.x << prefferedDelimiter << gazeDirection0.y << prefferedDelimiter << gazeDirection0.z
                     << prefferedDelimiter << gazeDirection1.x << prefferedDelimiter << gazeDirection1.y << prefferedDelimiter << gazeDirection1.z;
    }

    // Output the estimated head pose
    if (output_head_position)
    {
        if(face_model.tracking_initialised)
        {
            *output_file << prefferedDelimiter << boost::format("%04d") % pose_estimate[0]
                         << prefferedDelimiter << boost::format("%04d") % pose_estimate[1]
                         << prefferedDelimiter << boost::format("%04d") % pose_estimate[2];
        }
        else
        {
            *output_file << prefferedDelimiter << "0" << prefferedDelimiter << "0" << prefferedDelimiter << "0";
        }
    }

    if (output_head_pose)
    {
        if(face_model.tracking_initialised)
        {
            *output_file << prefferedDelimiter << boost::format("%03.3f") % (pose_estimate[4] * 180 / CV_PI)
                         << prefferedDelimiter << boost::format("%03.3f") % ((-1.0) * pose_estimate[3] * 180 / CV_PI)
                         << prefferedDelimiter << boost::format("%03.3f") % (pose_estimate[5] * 180 / CV_PI);
        }
        else
        {
            *output_file << prefferedDelimiter << "0" << prefferedDelimiter << "0" << prefferedDelimiter << "0";
        }
    }

    // Output the detected 2D facial landmarks
    if (output_2D_landmarks)
    {
        for (int i = 0; i < face_model.pdm.NumberOfPoints() * 2; ++i)
        {
            if(face_model.tracking_initialised)
            {
                *output_file << prefferedDelimiter << face_model.detected_landmarks.at<double>(i);
            }
            else
            {
                *output_file << prefferedDelimiter << "0";
            }
        }
    }

    // Output the detected 3D facial landmarks
    if (output_3D_landmarks)
    {
        cv::Mat_<double> shape_3D = face_model.GetShape(fx, fy, cx, cy);
        for (int i = 0; i < face_model.pdm.NumberOfPoints() * 3; ++i)
        {
            if (face_model.tracking_initialised)
            {
                *output_file << prefferedDelimiter << shape_3D.at<double>(i);
            }
            else
            {
                *output_file << prefferedDelimiter << "0";
            }
        }
    }

    if (output_model_params)
    {
        for (int i = 0; i < 6; ++i)
        {
            if (face_model.tracking_initialised)
            {
                *output_file << prefferedDelimiter << face_model.params_global[i];
            }
            else
            {
                *output_file << prefferedDelimiter << "0";
            }
        }
        for (int i = 0; i < face_model.pdm.NumberOfModes(); ++i)
        {
            if(face_model.tracking_initialised)
            {
                *output_file << prefferedDelimiter << face_model.params_local.at<double>(i, 0);
            }
            else
            {
                *output_file << prefferedDelimiter << "0";
            }
        }
    }

    if (output_AUs_reg) {
        auto aus_reg = face_analyser.GetCurrentAUsReg();

        vector <string> au_reg_names = face_analyser.GetAURegNames();
        std::sort(au_reg_names.begin(), au_reg_names.end());

        // write out ar the correct index
        for (string au_name : au_reg_names) {
            for (auto au_reg : aus_reg) {
                if (au_name.compare(au_reg.first) == 0) {
                    *output_file << prefferedDelimiter << au_reg.second;
                    break;
                }
            }
        }

        if (aus_reg.size() == 0) {
            for (size_t p = 0; p < face_analyser.GetAURegNames().size(); ++p) {
                *output_file << prefferedDelimiter << "0";
            }
        }
    }

    if (output_AUs_class){
        auto aus_class = face_analyser.GetCurrentAUsClass();

        vector<string> au_class_names = face_analyser.GetAUClassNames();
        std::sort(au_class_names.begin(), au_class_names.end());

        // write out ar the correct index
        for (string au_name : au_class_names)
        {
            for (auto au_class : aus_class)
            {
                if (au_name.compare(au_class.first) == 0)
                {
                    *output_file << prefferedDelimiter << au_class.second;
                    break;
                }
            }
        }

        if (aus_class.size() == 0)
        {
            for (size_t p = 0; p < face_analyser.GetAUClassNames().size(); ++p)
            {
                *output_file << prefferedDelimiter << "0";
            }
        }
    }

    if (output_pain_level)
    {
        *output_file << prefferedDelimiter << boost::format("%03.3f") % (painLevel);
    }

    if (output_valence)
    {
        *output_file << prefferedDelimiter << boost::format("%03.3f") % (valenceLevel);
    }

    *output_file << endl;
}


void get_output_feature_params(vector<string> &output_similarity_aligned, vector<string> &output_hog_aligned_files, double &similarity_scale,
                               int &similarity_size, bool &grayscale, bool& verbose, bool& dynamic, bool &output_2D_landmarks, bool &output_3D_landmarks,
                               bool &output_model_params, bool &output_frame_idx, bool &output_timestamp, bool &output_confidence,
                               bool &output_success, bool &output_head_position, bool &output_head_pose, bool &output_AUs_reg, bool &output_AUs_class,
                               bool &output_gaze, bool &output_pain_level, bool &output_valence, vector<string> &arguments)
{
    output_similarity_aligned.clear();
    output_hog_aligned_files.clear();

    bool* valid = new bool[arguments.size()];

    for (size_t i = 0; i < arguments.size(); ++i)
    {
        valid[i] = true;
    }

    string output_root = "";

    // By default the model is dynamic
    dynamic = true;

    string separator = string(1, boost::filesystem::path::preferred_separator);

    // First check if there is a root argument (so that videos and outputs could be defined more easilly)
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        if (arguments[i].compare("-root") == 0)
        {
            output_root = arguments[i + 1] + separator;
            i++;
        }
        if (arguments[i].compare("-outroot") == 0)
        {
            output_root = arguments[i + 1] + separator;
            i++;
        }
    }

    for (size_t i = 0; i < arguments.size(); ++i)
    {
        if (arguments[i].compare("-simalign") == 0)
        {
            output_similarity_aligned.push_back(output_root + arguments[i + 1]);
            create_directory(output_root + arguments[i + 1]);
            valid[i] = false;
            valid[i + 1] = false;
            i++;
        }
        else if (arguments[i].compare("-hogalign") == 0)
        {
            output_hog_aligned_files.push_back(output_root + arguments[i + 1]);
            create_directory_from_file(output_root + arguments[i + 1]);
            valid[i] = false;
            valid[i + 1] = false;
            i++;
        }
        else if (arguments[i].compare("-verbose") == 0)
        {
            verbose = true;
        }
        else if (arguments[i].compare("-au_static") == 0)
        {
            dynamic = false;
        }
        else if (arguments[i].compare("-g") == 0)
        {
            grayscale = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-simscale") == 0)
        {
            similarity_scale = stod(arguments[i + 1]);
            valid[i] = false;
            valid[i + 1] = false;
            i++;
        }
        else if (arguments[i].compare("-simsize") == 0)
        {
            similarity_size = stoi(arguments[i + 1]);
            valid[i] = false;
            valid[i + 1] = false;
            i++;
        }
        else if (arguments[i].compare("-add2Dfp") == 0)
        {
            output_2D_landmarks = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-add3Dfp") == 0)
        {
            output_3D_landmarks = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-addMparams") == 0)
        {
            output_model_params = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-addHeadPosition") == 0)
        {
            output_head_position = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-addHeadPose") == 0)
        {
            output_head_pose = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-addAUs_reg") == 0)
        {
            output_AUs_reg = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-addAUs_class") == 0)
        {
            output_AUs_class = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-addGaze") == 0)
        {
            output_gaze = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-addFrameIdx") == 0)
        {
            output_frame_idx = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-addTimestamp") == 0)
        {
            output_timestamp = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-addConfidence") == 0)
        {
            output_confidence = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-addSuccess") == 0)
        {
            output_success = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-addPainLevel") == 0)
        {
            output_pain_level = true;
            valid[i] = false;
        }
        else if (arguments[i].compare("-addValence") == 0)
        {
            output_valence = true;
            valid[i] = false;
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

// Can process images via directories creating a separate output file per directory
void get_image_input_output_params_feats(vector<vector<string> > &input_image_files, bool& as_video, vector<string> &arguments)
{
    bool* valid = new bool[arguments.size()];

    for (size_t i = 0; i < arguments.size(); ++i)
    {
        valid[i] = true;
        if (arguments[i].compare("-fdir") == 0)
        {

            // parse the -fdir directory by reading in all of the .png and .jpg files in it
            path image_directory(arguments[i + 1]);

            try
            {
                // does the file exist and is it a directory
                if (exists(image_directory) && is_directory(image_directory))
                {

                    vector<path> file_in_directory;
                    copy(directory_iterator(image_directory), directory_iterator(), back_inserter(file_in_directory));

                    // Sort the images in the directory first
                    sort(file_in_directory.begin(), file_in_directory.end());

                    vector<string> curr_dir_files;

                    for (vector<path>::const_iterator file_iterator(file_in_directory.begin()); file_iterator != file_in_directory.end(); ++file_iterator)
                    {
                        // Possible image extension .jpg and .png
                        if (file_iterator->extension().string().compare(".jpg") == 0 || file_iterator->extension().string().compare(".png") == 0)
                        {
                            curr_dir_files.push_back(file_iterator->string());
                        }
                    }

                    input_image_files.push_back(curr_dir_files);
                }
            }
            catch (const filesystem_error& ex)
            {
                cout << ex.what() << '\n';
            }

            valid[i] = false;
            valid[i + 1] = false;
            i++;
        }
        else if (arguments[i].compare("-asvid") == 0)
        {
            as_video = true;
        }
    }

    // Clear up the argument list
    for (int i = arguments.size() - 1; i >= 0; --i)
    {
        if (!valid[i])
        {
            arguments.erase(arguments.begin() + i);
        }
    }

}

void output_HOG_frame(std::ofstream* hog_file, bool good_frame, const cv::Mat_<double>& hog_descriptor, int num_rows, int num_cols)
{

    // Using FHOGs, hence 31 channels
    int num_channels = 31;

    hog_file->write((char*)(&num_cols), 4);
    hog_file->write((char*)(&num_rows), 4);
    hog_file->write((char*)(&num_channels), 4);

    // Not the best way to store a bool, but will be much easier to read it
    float good_frame_float;
    if (good_frame)
        good_frame_float = 1;
    else
        good_frame_float = -1;

    hog_file->write((char*)(&good_frame_float), 4);

    cv::MatConstIterator_<double> descriptor_it = hog_descriptor.begin();

    for (int y = 0; y < num_cols; ++y)
    {
        for (int x = 0; x < num_rows; ++x)
        {
            for (unsigned int o = 0; o < 31; ++o)
            {

                float hog_data = (float)(*descriptor_it++);
                hog_file->write((char*)&hog_data, 4);
            }
        }
    }
}

double get_pain_level_from_face(FaceAnalysis::FaceAnalyser face_analyser, vector<fer::randomTree> forest, fer::paramList params){

    auto aus_reg = face_analyser.GetCurrentAUsReg();

    Mat featData(1, aus_reg.size(), CV_64FC1);
    featData.setTo(0);
    int featIdx = 0;

    vector<string> au_reg_names = face_analyser.GetAURegNames();
    std::sort(au_reg_names.begin(), au_reg_names.end());

    // write out ar the correct index
    for (string au_name : au_reg_names)
    {
        for (auto au_reg : aus_reg)
        {
            if (au_name.compare(au_reg.first) == 0)
            {
                featData.at<double>(0, featIdx) = au_reg.second;
                featIdx ++;
                break;
            }
        }
    }

    vector<double> pred = RV_testForest(featData, forest, params, 2);
    double painLevel = pred[1]; //computePainLevel(pred);
    return painLevel;
}
vector<double> get_valence_from_face(FaceAnalysis::FaceAnalyser face_analyser, vector<fer::randomTree> forest, fer::paramList params){

    auto aus_reg = face_analyser.GetCurrentAUsReg();

    Mat featData(1, aus_reg.size(), CV_64FC1);
    featData.setTo(0);
    int featIdx = 0;

    vector<string> au_reg_names = face_analyser.GetAURegNames();
    std::sort(au_reg_names.begin(), au_reg_names.end());

    // write out ar the correct index
    for (string au_name : au_reg_names)
    {
        for (auto au_reg : aus_reg)
        {
            if (au_name.compare(au_reg.first) == 0)
            {
                featData.at<double>(0, featIdx) = au_reg.second;
                featIdx ++;
                break;
            }
        }
    }

    vector<double> pred = RV_testForest(featData, forest, params, 7);
//    double painLevel = pred[1]; //computePainLevel(pred);
    return pred;
}