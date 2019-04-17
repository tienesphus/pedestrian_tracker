# SWE40001-Bus-Passenger-Count

Counts passengers entering and exiting a bus.

## Building

### Setup
I have only tested the program in Linux. I think it should work fine in Windows.

You will need to install OpenCV 4.0 and cmake.

If you want to run in on a Neural Compute Stick, you will also need to install OpenVino.

OpenCV should be installed automatically when OpenVino is installed, so you probably do not have to build/install it manually. However, if you do, then make sure you [enable the Inference Engine](https://github.com/opencv/opencv/wiki/Intel%27s-Deep-Learning-Inference-Engine-backend)

Note: if using the Inference Engine, then you will have to run the `setupvars`:
```
. /opt/intel/openvino/bin/setupvars.sh 
```

Only once, you will have to download the required Neural Network modules.

```
git submodule update --init modules/<network>
# It asks me for my pasword 3 times. Not sure why?!?!
```

If the test videos are being used, it is assumed that the sample video repo has been downloaded to `../samplevideos`.
```
cd ..
git clone https://gitlab.com/buspassengercount/samplevideos.git
```

### Building
```
git clone https://gitlab.com/buspassengercount/dnn_counting.git
cd dnn_counting
mkdir build && cd build
cmake ..
make
```

### Running

```
./build/bin/bus_count
```
