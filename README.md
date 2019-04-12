# SWE40001-Bus-Passenger-Count

Counts passengers entering and exiting a bus.

## Building

### Dependencies
I have only tested the program in Linux. I think it should work fine in Windows.

You will need to install OpenCV 4.0 and cmake.

If you want to run in on a Neural Compute Stick, you will also need to install OpenVino.

OpenCV should be installed automatically when OpenVino is installed, so you probably do not have to build/install it manually. However, if you do, then make sure you [enable the Inference Engine](https://github.com/opencv/opencv/wiki/Intel%27s-Deep-Learning-Inference-Engine-backend)

### Building
```
git clone https://gitlab.com/buspassengercount/dnn_counting.git
cd dnn_counting
cmake .
make
```
Note: if using the Inference Engine, then you will have to run the `setupvars` script from OpenVino before you can build or run. 

### Running
```
./bin/bus_count
```
