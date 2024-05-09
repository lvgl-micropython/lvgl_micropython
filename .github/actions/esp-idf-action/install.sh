#!/bin/bash

# esp idf repository version
esp_idf_version="$1"

# Installing prerequisites
echo "## Install prerequisites"

export DEBIAN_FRONTEND=noninteractive

sudo apt-get install -y git wget flex bison gperf python3 python3-pip python3-setuptools cmake ninja-build ccache libffi-dev libssl-dev dfu-util

# Making Python 3 the default interpreter
sudo update-alternatives --install /usr/bin/python python /usr/bin/python3 10

# Create the esp directory for repo download
mkdir ~/esp
cd ~/esp

echo "## Download esp-idf repository"

case $esp_idf_version in
    latest)
        # Clone esp idf master branch repository
        git clone --recursive https://github.com/espressif/esp-idf.git
        ;;
    *release*)
        git clone --recursive --depth=1 --shallow-submodules -b $esp_idf_version https://github.com/espressif/esp-idf.git
        ;;
    *)
        # Download esp idf repository
        wget https://dl.espressif.com/github_assets/espressif/esp-idf/releases/download/$esp_idf_version/esp-idf-$esp_idf_version.zip

        # Extract the files and rename folder
        unzip -q esp-idf-$esp_idf_version.zip && mv esp-idf-$esp_idf_version esp-idf
        rm -f esp-idf-$esp_idf_version.zip
        ;;
esac

# Navigate to esp idf repository
cd ~/esp/esp-idf

# Installing tools
echo "## Install esp-idf tools"

# Install required tools
./install.sh
