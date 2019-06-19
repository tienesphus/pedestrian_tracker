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

To build in debug mode, replace the cmake command with:
```
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

### Running
From within the build directory:

```
bin/buscountcli
```

To run the daemon:
```
bin/buscountd
```

The gstreamer buscount plugin may also be used outside of `buscountd`. You may use any gstreamer command (`gst-launch-1.0`, `gst-inspect-1.0`, etc) as follows:
```
gst-inspect-1.0 --gst-plugin-load=lib/libgstbuscountplugin.so buscountfilter
```

Verbosity of messages printed to the screen by the GStreamer portion of the daemon and any gstreamer related commands (`gst-inspect-1.0`, `gst-launch-1.0`, etc) can be increased by setting the `GST_DEBUG` environment variable. Verbosity can be increased for individual log categories, or for all categories. Multiple related categories may also be set by using a wildcard `*`. Here is a simple example:
```
GST_DEBUG=3,buscount:5 bin/buscountd
```

For more information on the `GST_DEBUG` environment variable, take a look at the [relevant gstreamer documentation](https://gstreamer.freedesktop.org/documentation/tutorials/basic/debugging-tools.html#basic-tutorial-11-debugging-tools)
