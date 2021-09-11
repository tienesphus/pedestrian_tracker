# Pedestrian Tracker C++
![Sample run](https://cdn.discordapp.com/attachments/623044410287849492/886126909090594826/samplegif2.gif)

This module is adapted from pedestrian_tracker_demo module from Intel. For information on the original module and how it works, click [\[here\]](https://docs.openvinotoolkit.org/latest/omz_demos_pedestrian_tracker_demo_cpp.html).

## Dependencies
OpenVino 2021.4 and its dependencies.
OpenVino 2021.4: [\[Link\]](https://software.seek.intel.com/openvino-toolkit) [\[Installation instruction\]](https://docs.openvinotoolkit.org/2021.4/omz_demos_pedestrian_tracker_demo_cpp.html)

## Supported Models
person-detection-retail-xxxx.
person-reidentification-retail-xxxx.
These models can be downloaded using [Intel Model Downloader](https://docs.openvinotoolkit.org/latest/omz_tools_downloader.html) or from [Intel Open Model Zoo.](https://github.com/openvinotoolkit/open_model_zoo)

## Building
```
#from pedestrian_tracker root
mkdir build && cd build
cmake .
```
For linux based OS
```
make
```
For Windows
```
Open the generated .sln files and build via Microsoft Visual Studio 2019 or newer/MSBuild  
```
NOTE: `cmake .` will generate files for all submodules. Users can use cmake build flag to set build target to one sub-module only. See [\[here\]](https://cmake.org/cmake/help/latest/command/build_command.html).

## Running
Running the application with the  `-h`  option yields the following usage message:

```
InferenceEngine:
    API version ............ <version>
    Build .................. <number>

pedestrian_tracker_demo [OPTION]
Options:

    -h                           Print a usage message.
    -i                           Required. An input to process. The input must be a single image, a folder of images, video file or camera id.
    -loop                        Optional. Enable reading the input in a loop.
    -first                       Optional. The index of the first frame of the input to process. The actual first frame captured depends on cv::VideoCapture implementation and may have slightly different number.
    -read_limit                  Optional. Read length limit before stopping or restarting reading the input.
    -o "<path>"                  Optional. Name of the output file(s) to save.
    -limit "<num>"               Optional. Number of frames to store in output. If 0 is set, all frames are stored.
    -m_det "<path>"              Required. Path to the Pedestrian Detection Retail model (.xml) file.
    -m_reid "<path>"             Required. Path to the Pedestrian Reidentification Retail model (.xml) file.
    -l "<absolute_path>"         Optional. For CPU custom layers, if any. Absolute path to a shared library with the kernels implementation.
          Or
    -c "<absolute_path>"         Optional. For GPU custom kernels, if any. Absolute path to the .xml file with the kernels description.
    -d_det "<device>"            Optional. Specify the target device for pedestrian detection (the list of available devices is shown below). Default value is CPU. Use "-d HETERO:<comma-separated_devices_list>" format to specify HETERO plugin.
    -d_reid "<device>"           Optional. Specify the target device for pedestrian reidentification (the list of available devices is shown below). Default value is CPU. Use "-d HETERO:<comma-separated_devices_list>" format to specify HETERO plugin.
    -r                           Optional. Output pedestrian tracking results in a raw format 
    -pc                          Optional. Enable per-layer performance statistics.
    -no_show                     Optional. Don't show output.
    -delay                       Optional. Delay between frames used for visualization. If negative, the visualization is turned off (like with the option 'no_show'). If zero, the visualization is made frame-by-frame.
    -out "<path>"                Optional. The file name to write output log file with results of pedestrian tracking. The format of the log file is 
    -location                    Required when -out flag is present. Specify value for location field in log file. 
    -u                           Optional. List of monitors to show initially.
    -config "<config.txt>"       Optional. Enable distance estimation feature. The data in [config.txt] is used as calibration data for distance estimation algorithm. For more info on how to modify config.txt or to generate one, click [here].
    -reconfig                    Optional. Generate a new config.txt file using interactive GUI. Please refer to aforementioned document on -config flag above.
    -out_a						 Optional. Generate an additional log file which contains average time each detected person spent inside the region of interest.

```
## Logs

-out flag:
```
frame_no | real_time | person_id | x_coord | y_coord | width | height | confidence_level | location | total_count
```

**real_time**: real time data when that particular data entry is logged.
**person_id**: an integer id that uniquely identifies a tracked person.  **person_id** data transfers across frames which means if a tracked person with id 1 goes out of frame, id 1 will never be used again or be assigned to any other person. 
**confidence_level**: a float number (0 to 1) detection model gives every tracked person that indicates the detection confidence level for that particular person.
**location**: contains the value that is passed to the -location flag.
**total_count**: total number of people detected in that frame

-out_a flag:
```
person_id | initial_time | time_spent_in_roi
```
**person_id**: an integer id that uniquely identifies a tracked person. 
**initial_time**: Real time data when that person enters ROI.

The program writes to its log file every 100 frames.

## Re-configure the AI
This AI has been configured to maximise the accuracy for one specific counting context: Beswick Square.
It is **highly recommended**  for users to re-configure thresholds and sensitivities of the AI (by trial and error) when this project is used for a different counting context to achieve optimal accuracy level.
To re-configure thresholds and sensitivities of the AI, change the following:
In `tracker.cpp`:
```
min_track_duration

forget_delay

aff_thr_fast

aff_thr_strong

shape_affinity_w

motion_affinity_w

time_affinity_w

min_det_conf

bbox_aspect_ratios_range

bbox_heights_range

predict

strong_affinity_thr

reid_thr

drop_forgotten_tracks

max_num_objects_in_track.
``` 
In `detector.hpp`:
```
confidence_threshold
```
Instruction on how these variables affect the way this AI detects can be found in `tracker.hpp` and `detector.hpp` respectively.




