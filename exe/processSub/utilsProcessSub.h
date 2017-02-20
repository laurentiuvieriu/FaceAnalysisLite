//
// Created by radu on 10/11/16.
//

#ifndef FACEANALYSIS_UTILSPROCESSSUB_H
#define FACEANALYSIS_UTILSPROCESSSUB_H

#endif //FACEANALYSIS_UTILSPROCESSSUB_H

#include "opencv2/opencv.hpp"
using namespace cv;

// Boost includes
#include <filesystem.hpp>
#include <filesystem/fstream.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

// local includes
#include "LandmarkCoreIncludes.h"
#include <Face_utils.h>
#include <FaceAnalyser.h>
#include <GazeEstimation.h>
#include "ferLocalFunctions.h"

using namespace std;
using namespace boost::filesystem;

#define INFO_STREAM( stream ) \
std::cout << stream << std::endl

#define WARN_STREAM( stream ) \
std::cout << "Warning: " << stream << std::endl

static void printErrorAndAbort( const std::string & error )
{
    std::cout << error << std::endl;
}

#define FATAL_STREAM( stream ) \
printErrorAndAbort( std::string( "Fatal error: " ) + stream )


vector<string> get_arguments(int argc, char **argv);

vector<string> get_arguments_from_file(const string fileName);

// Useful utility for creating directories for storing the output files
void create_directory_from_file(string output_path);

void create_directory(string output_path);

void get_output_feature_params(vector<string> &output_similarity_aligned, vector<string> &output_hog_aligned_files, double &similarity_scale,
                               int &similarity_size, bool &grayscale, bool& verbose, bool& dynamic, bool &output_2D_landmarks, bool &output_3D_landmarks,
                               bool &output_model_params, bool &output_frame_idx, bool &output_timestamp, bool &output_confidence,
                               bool &output_success, bool &output_head_position, bool &output_head_pose, bool &output_AUs_reg, bool &output_AUs_class,
                               bool &output_gaze, bool &output_pain_level, bool &output_valence, bool &output_arousal, vector<string> &arguments);

void get_image_input_output_params_feats(vector<vector<string> > &input_image_files, bool& as_video, vector<string> &arguments);

void output_HOG_frame(std::ofstream* hog_file, bool good_frame, const cv::Mat_<double>& hog_descriptor, int num_rows, int num_cols);

// Some globals for tracking timing information for visualisation
//double fps_tracker = -1.0;
//int64 t0 = 0;

// Visualising the results
void visualise_tracking(cv::Mat& captured_image, const LandmarkDetector::CLNF& face_model, const LandmarkDetector::FaceModelParameters& det_parameters,
                        cv::Point3f gazeDirection0, cv::Point3f gazeDirection1, int frame_count, double fx, double fy, double cx, double cy,
                        const FaceAnalysis::FaceAnalyser& face_analyser, double painLevel, double valenceLevel, double arousalLevel, bool output_head_pose, bool output_AUs_class);


void prepareOutputFile(std::ofstream* output_file, bool output_2D_landmarks, bool output_3D_landmarks, bool output_model_params,
                       bool output_frame_idx, bool output_timestamp, bool output_confidence, bool output_success,
                       bool output_head_position, bool output_head_pose, bool output_AUs_reg, bool output_AUs_class, bool output_gaze, bool output_pain_level,
                       bool output_valence, bool output_arousal, int num_landmarks, int num_model_modes, vector<string> au_names_class, vector<string> au_names_reg);

// Output all of the information into one file in one go (quite a few parameters, but simplifies the flow)
void outputAllFeatures(std::ofstream *output_file, bool output_2D_landmarks, bool output_3D_landmarks,
                       bool output_model_params, bool output_frame_idx, bool output_timestamp, bool output_confidence,
                       bool output_success, bool output_head_position, bool output_head_pose, bool output_AUs_reg, bool output_AUs_class,
                       bool output_gaze, bool output_pain_level, bool output_valence, bool output_arousal, const LandmarkDetector::CLNF &face_model, int frame_count, double time_stamp,
                       bool detection_success, cv::Point3f gazeDirection0, cv::Point3f gazeDirection1,
                       const cv::Vec6d &pose_estimate, double fx, double fy, double cx, double cy, double painLevel, double valenceLevel, double arousalLevel,
                       const FaceAnalysis::FaceAnalyser &face_analyser);

void post_process_output_file(FaceAnalysis::FaceAnalyser& face_analyser, string output_file, bool dynamic);

string get_key_from_arguments(vector<string> &arguments, const string key);

double get_pain_level_from_face(FaceAnalysis::FaceAnalyser face_analyser, vector<fer::randomTree> forest, fer::paramList params);
vector<double> get_valence_from_face(FaceAnalysis::FaceAnalyser face_analyser, vector<fer::randomTree> forest, fer::paramList params);