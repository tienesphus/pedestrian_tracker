# SWE40001-Bus-Passenger-Count

Counts passengers entering and exiting a bus.

## Dependencies

This has only been tested on:
- Ubuntu 18.04
- Ubuntu 20.04.2.0 LTS

### Install system libraries
Install these libraries:
- Make (4.2.1-1.2)
- Cmake (3.16.3-1ubuntu1)
- Libsqlite3-dev (3.31.1-4ubuntu0.2)
- Libjsoncpp-dev (1.7.4-3.1ubuntu2)
```
sudo apt install make cmake
sudo apt install libsqlite3-dev
sudo apt install libjsoncpp-dev
```

### Install OpenCV/OpenVino and its dependencies
OpenVino 2021.3 [Direct download for Linux/Ubuntu](https://registrationcenter-download.intel.com/akdlm/irc_nas/17662/l_openvino_toolkit_p_2021.3.394.tgz) | [Installation Instruction](https://docs.openvinotoolkit.org/2021.3/openvino_docs_install_guides_installing_openvino_linux.html)

Make sure OpenVino is setup with:
```
. /opt/intel/openvino/bin/setupvars.sh 
```

It is recommended to run sample scripts come with OpenVino to verify OpenVino is installed correctly [Instruction](https://docs.openvinotoolkit.org/2021.3/openvino_docs_get_started_get_started_linux.html)

## Clone and build this repo
```
git clone https://github.com/tienesphus/cpp_counting
```
### Initialise network submodules
Only once, you will have to download the required Neural Network modules.

Initialise all submodules

```
git submodule update --init
```

Or initialise individual submodule
```
git submodule update --init models/<network>
# replace <network> with the folder that needs initialising
```
### Sample videos
If the test videos are being used, it is assumed that the sample video repo has been downloaded to `../samplevideos`.
```
cd ..
git clone https://gitlab.com/buspassengercount/samplevideos.git
```

### Build using cmake
```
cd cpp_counting
mkdir build && cd build
cmake ..
make
```

To build in debug mode, replace the cmake command with:
```
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

### Running
From within the build directory:

To run the development prompt:
```
./bin/buscountcli
```



