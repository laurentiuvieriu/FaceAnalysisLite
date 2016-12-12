#TBB library
include_directories(${TBB_ROOT_DIR}/include)

file(GLOB project_SRC
    "*.h"
    "*.cpp")

add_executable(processCam ${project_SRC} utilsProcessCam.h utilsProcessCam.cpp)

# Move LandmarkDetector model
file(GLOB files "../../lib/local/LandmarkDetector/model/*.txt")
foreach(file ${files})
	file(COPY ${file} DESTINATION ./model)
endforeach()

# Move the hierarchical LandmarkDetector models
file(GLOB files "../../lib/local/LandmarkDetector/model/model*")
foreach(file ${files})
	file(COPY ${file} DESTINATION ./model)
endforeach()

file(GLOB files "../../lib/local/LandmarkDetector/model/detection_validation/*.txt")
foreach(file ${files})
	file(COPY ${file} DESTINATION ./model/detection_validation)
endforeach()

file(GLOB files "../../lib/local/LandmarkDetector/model/patch_experts/*.txt")
foreach(file ${files})
	file(COPY ${file} DESTINATION ./model/patch_experts)
endforeach()

file(GLOB files "../../lib/local/LandmarkDetector/model/pdms/*.txt")
foreach(file ${files})
	file(COPY ${file} DESTINATION ./model/pdms)
endforeach()

# Move OpenCV classifiers
file(GLOB files "../../lib/3rdParty/OpenCV3.1/classifiers/*.xml")
foreach(file ${files})
	file(COPY ${file} DESTINATION ./classifiers)
endforeach()

# Move AU prediction modules
file(GLOB files "../../lib/local/FaceAnalyser/AU_predictors/*.txt")
foreach(file ${files})
	file(COPY ${file} DESTINATION ./AU_predictors)
endforeach()

# Move AU prediction modules
file(GLOB files "../../lib/local/FaceAnalyser/AU_predictors/svr*")
foreach(file ${files})
	file(COPY ${file} DESTINATION ./AU_predictors)
endforeach()

# Move AU prediction modules
file(GLOB files "../../lib/local/FaceAnalyser/AU_predictors/svm*")
foreach(file ${files})
	file(COPY ${file} DESTINATION ./AU_predictors)
endforeach()

# Local libraries
include_directories(${LandmarkDetector_SOURCE_DIR}/include)

include_directories(../../lib/local/LandmarkDetector/include)
include_directories(../../lib/local/FaceAnalyser/include)

target_link_libraries(processCam LandmarkDetector)
target_link_libraries(processCam FaceAnalyser)
target_link_libraries(processCam dlib)

target_link_libraries(processCam ${OpenCV_LIBS} ${Boost_LIBRARIES} ${TBB_LIBRARIES})
