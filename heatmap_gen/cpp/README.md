# Heatmap Generator C++


![Sample output image using Spectral_mixed color scheme](https://cdn.discordapp.com/attachments/817277145251971085/874092369094512691/result1.png)

This project uses a high-performance C heatmap generation library by lucasb-eyer/Github. For more information on the library or the author, click [\[here\]](https://github.com/lucasb-eyer/libheatmap).
## How it works
Heatmap generator takes two inputs:

 - a png reference image file
 - a csv log from *pedestrian_tracker*
 
and outputs a png single image which is a heatmap layer overlayed on top of the given reference image.
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
```
usage: ./heatmap_gen reference.png output.png [STAMP_RADIUS [COLORSCHEME]] < logfile-people-track.csv


Running the application yields the following usage message:

Invalid number of arguments!
Usage:
./heatmap_gen reference.png output.png [STAMP_RADIUS [COLORSCHEME]] < data.csv

output.png is the name of the output image file.

data.csv should follow output format of pedestrian_tracker and it should contain coordinates (x, y) as well as (width, height) in column 4, 5, 6, 7 respectively

This module calculates feet coordinates from input data using the following formulas: feet_x = x + width/2 || feet_y = y + height. 
(feet_x, feet_y) will then be used to create points to put onto the heatmap.

The default STAMP_RADIUS is 1/10th of the smallest heatmap dimension.
For instance, for a 512x1024 heatmap, the default stamp_radius is 51,
resulting in a stamp of 102x102 pixels.

To get a list of available colorschemes, run
./heatmap_gen -l
The default colorscheme is Spectral_mixed.
```

Note:

 - There are more than 110 different color schemes to choose from.
 - Running the application with the `-l` option shows all available colorschemes. 
 - Out-of-range coordinates in the input log file will be ignored. 
 - Default opacity level is 30%. Custom opacity level for heatmap layer can be changed by making change to the code and re-build the project.

