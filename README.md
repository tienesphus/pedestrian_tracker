# SWE40001-Bus-Passenger-Count

Counts passengers entering and exiting a bus.

## Building

### Setup
I have only tested the program in Linux. I think it should work fine in Windows.

You will need to install OpenCV 4.0 and cmake and tbb.
```
sudo apt install make cmake
sudo apt install libtbb-dev libtbb2 libtbb2-dbg
```

If you want to run in on a Neural Compute Stick, you will also need to install OpenVino.

OpenCV should be installed automatically when OpenVino is installed, so you probably do not have to build/install it manually. However, if you do, then make sure you enable the inference engine:

```
. /opt/intel/openvino/bin/setupvars.sh 
```

Only once, you will have to download the required Neural Network modules.

```
git submodule update --init models/<network>
# replace <network> with the folder that needs initialising
```
Note: All submodules are stored in private repos, hence will require logging in for each (git doesn't have any concept of gitlab groups, so can't share the same password for each repo). To circumvent this, we will probably need to modify the submodules to make use of SSH, which will allow us to use either `ssh-agent` or SSH public-private keys.

If the test videos are being used, it is assumed that the sample video repo has been downloaded to `../samplevideos`.
```
cd ..
git clone https://gitlab.com/buspassengercount/samplevideos.git
```

### External dependancies
There are other dependancies that are also required, these can be installed using apt get
```
sudo apt install make cmake

sudo apt install libtbb-dev libtbb2 libtbb2-dbg

sudo apt-get install libgstrtspserver-1.0-dev
sudo apt-get install gstreamer1.0-omx-rpi
sudo apt-get install gstreamer1.0-plugins-base
```

### Building
```
git clone https://gitlab.com/buspassengercount/dnn_counting.git
cd cpp_counting
mkdir build && cd build
cmake ..
make
```

### Running
From within the build directory:

```
./bin/bus_count
```

To run the daemon (currently only a simple RTSP server):
```
./bin/buscountd
```

Note: At this stage, the daemon does not yet make use of OpenCV, and is only a simple implementation of an RTSP server.
