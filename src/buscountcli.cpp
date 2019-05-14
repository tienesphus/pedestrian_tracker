#include <iostream>
#include <vector>

#include <tbb/flow_graph.h>
#include <tbb/tick_count.h>
#include <tbb/concurrent_queue.h>

#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include "detection.hpp"
#include "world.hpp"
#include "tracker.hpp"
#include "utils.hpp"


int main() {
    
    NetConfigIR net_config {
        0.5f,               // thresh
        15,                 // clazz
        cv::Size(300, 300), // size
        2/255.0,            // scale
        cv::Scalar(127.5, 127.5, 127.5),     // mean
        "../models/MobileNetSSD_IE/MobileNetSSD.xml", // xml
        "../models/MobileNetSSD_IE/MobileNetSSD.bin"  // bin
    };
    
    WorldConfig world_config = WorldConfig::from_file("../config.csv");
    
    std::string input = "../../samplevideos/pi3_20181213/2018-12-13--08-26-02--snippit-1.mp4";
    
    std::cout << input << std::endl;
    
    
    run_parallel(input, net_config, world_config, true);
    //run_series(input, net_config, world_config, true);
    
    return 0;
} 
