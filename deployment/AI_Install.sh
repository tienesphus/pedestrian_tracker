#!/bin/bash
# Installs OpenVINO and configures the relivant directory strcuture. To be run on Rasbian Or Ubuntu 20.04


if [ $(id -u) != "0" ]; then
echo "You must be the superuser (sudo) to run this script" >&2
exit 1
fi

## General Dependancies
apt-get update
apt-get -y install curl

# Install the build tools (dpkg-dev g++ gcc libc6-dev make, not required but recomended)
apt-get -y install build-essential cmake unzip pkg-config

#Collection of Video Codec Packages to enable the viewing of videos
apt-get -y install libjpeg-dev libpng-dev libtiff-dev
apt-get -y install libavcodec-dev libavformat-dev libswscale-dev libv4l-dev
apt-get -y install libxvidcore-dev libx264-dev


#Install OpenVINO
mkdir -p /opt/intel/openvino_2021


#Get the most recent copy of OpenVINO
wget https://storage.openvinotoolkit.org/repositories/openvino/packages/2021.4/l_openvino_toolkit_runtime_raspbian_p_2021.4.582.tgz

#Extract OpenVINO
sudo tar -xf  l_openvino_toolkit_runtime_raspbian_p_2021.4.582.tgz --strip 1 -C /opt/intel/openvino_2021

echo "/opt/intel/openvino_2021/bin/setupvars.sh" >> ./bashrc


