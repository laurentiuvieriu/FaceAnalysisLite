# README #

This projects implements several face analysis components, such as:
1. facial landmark tracking
 	- head pose estimation
 	- facial action unit spotting and intensity estimation
 	- pain level seen on face
 	- emotional valence

The project is based on [1], which in turn uses DLib to initialize the CLNF tracking model. The CLNF model tracks in real-time 68 facial landmarks and uses a 3D representation of these landmarks, from which the head position and orientation are derived. Using HOG representation, [1] also provides estimates of presence and intensity for 18 facial action units. In addition to [1] estimators for the pain level read on the face (probabilistic scale), as well as for emotional valence (continuous scale between -1 and 1) were developed.

The whole system runs on a simple RGB camera in real-time and requires no calibration (altough providing one could help increase performance)

Dependencies (Ubuntu 14.04):
	- GCC
		sudo apt-get update
		sudo apt-get install build-essential
	- CMake
		sudo apt-get install cmake
	- OpenCV 3.1.0
		I used this source: http://docs.opencv.org/3.0-beta/doc/tutorials/introduction/linux_install/linux_install.html
		If you use the same, make sure to install the optionals too (TBB is there and is required)
	- Boost
		sudo apt-get install libboost1.55-all-dev
	- BLAS (for DLib)
		sudo apt-get install libopenblas-dev liblapack-dev

Compiling the code:
	- get the source:
		git clone https://github.com/TadasBaltrusaitis/OpenFace.git
	- compile (using CMake):
		cd FaceAnalysisLite
		mkdir build
		cd build
		cmake -D CMAKE_BUILD_TYPE=RELEASE ..
		make -j2

[1] Baltru, Tadas, Peter Robinson, and Louis-Philippe Morency. "OpenFace: an open source facial behavior analysis toolkit." 2016 IEEE Winter Conference on Applications of Computer Vision (WACV). IEEE, 2016.