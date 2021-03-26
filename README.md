# SWE40001-Bus-Passenger-Count

Counts passengers entering and exiting a bus.

## Building

This has only been tested on:
- Ubuntu 18.04
- Raspberrian Stretch
- Rasperrian Buster

### Install system libraries
Install these libraries:

```
sudo apt install make cmake
sudo apt install libtbb-dev libtbb2
sudo apt install libgstrtspserver-1.0-dev
sudo apt install gstreamer1.0-omx-rpi # on Pi only
sudo apt install gstreamer1.0-plugins-base
sudo apt install libgstreamermm-1.0-dev
sudo apt install libgstrtspserver-1.0-dev gstreamer1.0-rtsp
sudo apt install libsqlite3-dev
sudo apt install libjsoncpp-dev
sudo ln -s /usr/include/jsoncpp/json/ /usr/include/json
```

### Install OpenCV/OpenVino
You will need to install OpenVino and OpenCV. You do not need to install OpenCV from source; it is fine to just use the OpenCV binaries that come with OpenVino. However, if you do install OpenCV from source, make sure it includes the Inference Engine.

Install instructions:
- Pi: https://docs.openvinotoolkit.org/latest/_docs_install_guides_installing_openvino_raspbian.html
- Ubuntu: https://docs.openvinotoolkit.org/latest/_docs_install_guides_installing_openvino_linux.html

Make sure OpenVino is setup with:
```
. /opt/intel/openvino/bin/setupvars.sh 
```

### Install Drogon
See instructions here:
https://github.com/an-tao/drogon/wiki/02-Installation

### Download Related repos
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

### Building
```
git clone https://gitlab.com/buspassengercount/cpp_counting.git
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

To run the rtsp daemon:
```
./bin/buscountd
```

To run the web server:
```
./bin/buscountserver
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


### Installation
To install the system so it runs on boot:

```
sudo make install

# Enable each service
sudo systemctl enable buscountd.service 
sudo systemctl enable buswebserver.service 
sudo systemctl enable home-pi-code-cpp_counting-ram_disk.mount
```

Reboot the Pi. The system can be monitored through 
```
journalctl -u buscountd.service
journalctl -u buswebserver.service
```
