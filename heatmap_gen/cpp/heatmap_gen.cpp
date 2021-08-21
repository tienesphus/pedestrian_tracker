/* heatmap - High performance heatmap creation in C.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 Lucas Beyer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <algorithm>
#include <iostream>
#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

#include "lodepng.h"
#include "heatmap.h"

#include "gray.h"
#include "Blues.h"
#include "BrBG.h"
#include "BuGn.h"
#include "BuPu.h"
#include "GnBu.h"
#include "Greens.h"
#include "Greys.h"
#include "Oranges.h"
#include "OrRd.h"
#include "PiYG.h"
#include "PRGn.h"
#include "PuBuGn.h"
#include "PuBu.h"
#include "PuOr.h"
#include "PuRd.h"
#include "Purples.h"
#include "RdBu.h"
#include "RdGy.h"
#include "RdPu.h"
#include "RdYlBu.h"
#include "RdYlGn.h"
#include "Reds.h"
#include "Spectral.h"
#include "YlGnBu.h"
#include "YlGn.h"
#include "YlOrBr.h"
#include "YlOrRd.h"

// Too bad C++ doesn't have nice mad reflection skillz!
std::map<std::string, const heatmap_colorscheme_t*> g_schemes = {
    {"b2w", heatmap_cs_b2w},
    {"b2w_opaque", heatmap_cs_b2w_opaque},
    {"w2b", heatmap_cs_w2b},
    {"w2b_opaque", heatmap_cs_w2b_opaque},
    {"Blues_discrete", heatmap_cs_Blues_discrete},
    {"Blues_soft", heatmap_cs_Blues_soft},
    {"Blues_mixed", heatmap_cs_Blues_mixed},
    {"Blues_mixed_exp", heatmap_cs_Blues_mixed_exp},
    {"BrBG_discrete", heatmap_cs_BrBG_discrete},
    {"BrBG_soft", heatmap_cs_BrBG_soft},
    {"BrBG_mixed", heatmap_cs_BrBG_mixed},
    {"BrBG_mixed_exp", heatmap_cs_BrBG_mixed_exp},
    {"BuGn_discrete", heatmap_cs_BuGn_discrete},
    {"BuGn_soft", heatmap_cs_BuGn_soft},
    {"BuGn_mixed", heatmap_cs_BuGn_mixed},
    {"BuGn_mixed_exp", heatmap_cs_BuGn_mixed_exp},
    {"BuPu_discrete", heatmap_cs_BuPu_discrete},
    {"BuPu_soft", heatmap_cs_BuPu_soft},
    {"BuPu_mixed", heatmap_cs_BuPu_mixed},
    {"BuPu_mixed_exp", heatmap_cs_BuPu_mixed_exp},
    {"GnBu_discrete", heatmap_cs_GnBu_discrete},
    {"GnBu_soft", heatmap_cs_GnBu_soft},
    {"GnBu_mixed", heatmap_cs_GnBu_mixed},
    {"GnBu_mixed_exp", heatmap_cs_GnBu_mixed_exp},
    {"Greens_discrete", heatmap_cs_Greens_discrete},
    {"Greens_soft", heatmap_cs_Greens_soft},
    {"Greens_mixed", heatmap_cs_Greens_mixed},
    {"Greens_mixed_exp", heatmap_cs_Greens_mixed_exp},
    {"Greys_discrete", heatmap_cs_Greys_discrete},
    {"Greys_soft", heatmap_cs_Greys_soft},
    {"Greys_mixed", heatmap_cs_Greys_mixed},
    {"Greys_mixed_exp", heatmap_cs_Greys_mixed_exp},
    {"Oranges_discrete", heatmap_cs_Oranges_discrete},
    {"Oranges_soft", heatmap_cs_Oranges_soft},
    {"Oranges_mixed", heatmap_cs_Oranges_mixed},
    {"Oranges_mixed_exp", heatmap_cs_Oranges_mixed_exp},
    {"OrRd_discrete", heatmap_cs_OrRd_discrete},
    {"OrRd_soft", heatmap_cs_OrRd_soft},
    {"OrRd_mixed", heatmap_cs_OrRd_mixed},
    {"OrRd_mixed_exp", heatmap_cs_OrRd_mixed_exp},
    {"PiYG_discrete", heatmap_cs_PiYG_discrete},
    {"PiYG_soft", heatmap_cs_PiYG_soft},
    {"PiYG_mixed", heatmap_cs_PiYG_mixed},
    {"PiYG_mixed_exp", heatmap_cs_PiYG_mixed_exp},
    {"PRGn_discrete", heatmap_cs_PRGn_discrete},
    {"PRGn_soft", heatmap_cs_PRGn_soft},
    {"PRGn_mixed", heatmap_cs_PRGn_mixed},
    {"PRGn_mixed_exp", heatmap_cs_PRGn_mixed_exp},
    {"PuBuGn_discrete", heatmap_cs_PuBuGn_discrete},
    {"PuBuGn_soft", heatmap_cs_PuBuGn_soft},
    {"PuBuGn_mixed", heatmap_cs_PuBuGn_mixed},
    {"PuBuGn_mixed_exp", heatmap_cs_PuBuGn_mixed_exp},
    {"PuBu_discrete", heatmap_cs_PuBu_discrete},
    {"PuBu_soft", heatmap_cs_PuBu_soft},
    {"PuBu_mixed", heatmap_cs_PuBu_mixed},
    {"PuBu_mixed_exp", heatmap_cs_PuBu_mixed_exp},
    {"PuOr_discrete", heatmap_cs_PuOr_discrete},
    {"PuOr_soft", heatmap_cs_PuOr_soft},
    {"PuOr_mixed", heatmap_cs_PuOr_mixed},
    {"PuOr_mixed_exp", heatmap_cs_PuOr_mixed_exp},
    {"PuRd_discrete", heatmap_cs_PuRd_discrete},
    {"PuRd_soft", heatmap_cs_PuRd_soft},
    {"PuRd_mixed", heatmap_cs_PuRd_mixed},
    {"PuRd_mixed_exp", heatmap_cs_PuRd_mixed_exp},
    {"Purples_discrete", heatmap_cs_Purples_discrete},
    {"Purples_soft", heatmap_cs_Purples_soft},
    {"Purples_mixed", heatmap_cs_Purples_mixed},
    {"Purples_mixed_exp", heatmap_cs_Purples_mixed_exp},
    {"RdBu_discrete", heatmap_cs_RdBu_discrete},
    {"RdBu_soft", heatmap_cs_RdBu_soft},
    {"RdBu_mixed", heatmap_cs_RdBu_mixed},
    {"RdBu_mixed_exp", heatmap_cs_RdBu_mixed_exp},
    {"RdGy_discrete", heatmap_cs_RdGy_discrete},
    {"RdGy_soft", heatmap_cs_RdGy_soft},
    {"RdGy_mixed", heatmap_cs_RdGy_mixed},
    {"RdGy_mixed_exp", heatmap_cs_RdGy_mixed_exp},
    {"RdPu_discrete", heatmap_cs_RdPu_discrete},
    {"RdPu_soft", heatmap_cs_RdPu_soft},
    {"RdPu_mixed", heatmap_cs_RdPu_mixed},
    {"RdPu_mixed_exp", heatmap_cs_RdPu_mixed_exp},
    {"RdYlBu_discrete", heatmap_cs_RdYlBu_discrete},
    {"RdYlBu_soft", heatmap_cs_RdYlBu_soft},
    {"RdYlBu_mixed", heatmap_cs_RdYlBu_mixed},
    {"RdYlBu_mixed_exp", heatmap_cs_RdYlBu_mixed_exp},
    {"RdYlGn_discrete", heatmap_cs_RdYlGn_discrete},
    {"RdYlGn_soft", heatmap_cs_RdYlGn_soft},
    {"RdYlGn_mixed", heatmap_cs_RdYlGn_mixed},
    {"RdYlGn_mixed_exp", heatmap_cs_RdYlGn_mixed_exp},
    {"Reds_discrete", heatmap_cs_Reds_discrete},
    {"Reds_soft", heatmap_cs_Reds_soft},
    {"Reds_mixed", heatmap_cs_Reds_mixed},
    {"Reds_mixed_exp", heatmap_cs_Reds_mixed_exp},
    {"Spectral_discrete", heatmap_cs_Spectral_discrete},
    {"Spectral_soft", heatmap_cs_Spectral_soft},
    {"Spectral_mixed", heatmap_cs_Spectral_mixed},
    {"Spectral_mixed_exp", heatmap_cs_Spectral_mixed_exp},
    {"YlGnBu_discrete", heatmap_cs_YlGnBu_discrete},
    {"YlGnBu_soft", heatmap_cs_YlGnBu_soft},
    {"YlGnBu_mixed", heatmap_cs_YlGnBu_mixed},
    {"YlGnBu_mixed_exp", heatmap_cs_YlGnBu_mixed_exp},
    {"YlGn_discrete", heatmap_cs_YlGn_discrete},
    {"YlGn_soft", heatmap_cs_YlGn_soft},
    {"YlGn_mixed", heatmap_cs_YlGn_mixed},
    {"YlGn_mixed_exp", heatmap_cs_YlGn_mixed_exp},
    {"YlOrBr_discrete", heatmap_cs_YlOrBr_discrete},
    {"YlOrBr_soft", heatmap_cs_YlOrBr_soft},
    {"YlOrBr_mixed", heatmap_cs_YlOrBr_mixed},
    {"YlOrBr_mixed_exp", heatmap_cs_YlOrBr_mixed_exp},
    {"YlOrRd_discrete", heatmap_cs_YlOrRd_discrete},
    {"YlOrRd_soft", heatmap_cs_YlOrRd_soft},
    {"YlOrRd_mixed", heatmap_cs_YlOrRd_mixed},
    {"YlOrRd_mixed_exp", heatmap_cs_YlOrRd_mixed_exp},
};

std::vector<int> praise_csv(std::string line) {
    std::vector<int> result;
    std::vector<std::string> temp;
    std::stringstream s_stream(line);
        while(s_stream.good()) {
            std::string substr;
            std::getline(s_stream, substr, ',');
            temp.push_back(substr);
        }
    try {
        result.push_back(std::stoi(temp[3]) + std::stoi(temp[5])/2);
        result.push_back(std::stoi(temp[4]) + std::stoi(temp[6]));
    }
    catch (std::exception e) {
        std::cerr << "Error praising input data." << std::endl;
    }
    return result;
}
void blend(cv::Mat background, cv::Mat foreground, float opacity) {
    if(background.empty())
        std::cerr << "Background is empty!" << std::endl;

    if(foreground.empty())
        std::cerr << "Foreground is empty!" << std::endl;

    if(background.size() != foreground.size())
        std::cerr << "Dimension Mismatch: background and foreground must have the same dimensions" << std::endl;


    int h = foreground.size().height;
    int w = foreground.size().width;

    int rows = background.size().height; 
    int columns = background.size().width;

    for(int i = 0; i < h; i++){
        for(int j = 0; j < w; j++){
            if(i >= rows || j >= columns)
                continue;

            float alpha = (foreground.at<cv::Vec4b>(i,j)[3]/255.0)*opacity;
            for(int z = 0; z < 3; z++){
                background.at<cv::Vec3b>(i,j)[z] = alpha*foreground.at<cv::Vec4b>(i,j)[z]+(1-alpha)*background.at<cv::Vec3b>(i,j)[z];
            }
        }
    }
}

int main(int argc, char* argv[])
{
    if(argc == 2 && std::string(argv[1]) == "-l") {
        for(auto& scheme : g_schemes) {
            std::cout << "  " << scheme.first << std::endl;
        }
        return 0;
    }

    if(argc < 3 || 6 < argc) {
        std::cerr << "Invalid number of arguments!" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  " << argv[0] << " reference.png output.png [STAMP_RADIUS [COLORSCHEME]] < data.csv" << std::endl;
        std::cout << std::endl;
        std::cout << "  output.png is the name of the output image file." << std::endl;
        std::cout << std::endl;
        std::cout << "  data.csv should follow output format of pedestrian_tracker and it should contain" << std::endl;
        std::cout << "  coordinates (x, y) as well as (width, height) in " << std::endl;
        std::cout << "  column 4, 5, 6, 7 respectively" << std::endl;
        std::cout << std::endl;
        std::cout << "  This module calculates feet coordinates from input data using the following formulas:" << std::endl;
        std::cout << "  feet_x = x + width/2 || feet_y = y + height" << std::endl;
        std::cout << "  Then (feet_x, feet_y) will be used to create points to put onto the heatmap." << std::endl;
        std::cout << std::endl;
        std::cout << "  The default STAMP_RADIUS is 1/10th of the smallest heatmap dimension." << std::endl;
        std::cout << "  For instance, for a 512x1024 heatmap, the default stamp_radius is 51," << std::endl;
        std::cout << "  resulting in a stamp of 102x102 pixels." << std::endl;
        std::cout << std::endl;
        std::cout << "  To get a list of available colorschemes, run" << std::endl;
        std::cout << "  " << argv[0] << " -l" << std::endl;
        std::cout << "  The default colorscheme is Spectral_mixed." << std::endl;

        return 1;
    }

    cv::Mat background = cv::imread(argv[1]);
    if (background.empty()) {
        std::cerr << "Can't read " << argv[1];
        return 1;
    }
        
    size_t w = background.size().width, h = background.size().height;
    heatmap_t* hm = heatmap_new(w, h);

    const size_t r = argc >= 4 ? atoi(argv[3]) : std::min(w, h)/10;
    heatmap_stamp_t* stamp = heatmap_stamp_gen(r);

    if(argc >= 5 && g_schemes.find(argv[4]) == g_schemes.end()) {
        std::cerr << "Unknown colorscheme. Run " << argv[0] << " -l for a list of valid ones." << std::endl;
        return 1;
    }
    const heatmap_colorscheme_t* colorscheme = argc == 5 ? g_schemes[argv[4]] : heatmap_cs_default;

    std::vector<int> praised_result;
    std::string line;
    unsigned int x, y;
    float weight = 1.0f;

    while( std::getline(std::cin, line) ) {
        praised_result = praise_csv(line);
        x = praised_result[0];
        y = praised_result[1];
        if(x <= w && y <= h) {
            heatmap_add_weighted_point_with_stamp(hm, x, y, weight, stamp);
        } else {
            std::cerr << "[Warning]: Skipping out-of-bound input coordinate: (" << x << "," << y << ")." << std::endl;
        }
    }
    heatmap_stamp_free(stamp);

    std::vector<unsigned char> image(w*h*4);
    heatmap_render_to(hm, colorscheme, &image[0]);
    heatmap_free(hm);
    
    std::vector<unsigned char> png;
    if(unsigned error = lodepng::encode(png, image, w, h)) {
       std::cerr << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
       return 1;
    }
    lodepng::save_file(png, argv[2]);
    
    cv::Mat foreground = cv::imread(argv[2], cv::IMREAD_UNCHANGED);
    //blend background and foreground
    blend(background, foreground, 0.30);
    cv::imwrite(argv[2], background);

    return 0;
}

