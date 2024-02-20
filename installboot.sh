#!/usr/bin/env bash
# ////////////////////////////////////
# Running this script will compile and install the necessary device 
# tree overlays for the BeagleBone Black in order to run
# the 'Splash' water simulation toy.
# 
# If you wish to run the water simulation on boot, the file
# named 'installBOOT.sh' should be run instead.
#
# WARNING: This file will modify your /boot/uEnv.txt to install the needed
# overlays. Disable the associated lines and do the modification manually
# if you are worried about potential conflicts.
# 
# You have been warned.
# ////////////////////////////////////
#
# EXPECTED WIRING:
#
# LCD (ILI9341):
# P9.29 <--> MISO
# P9.16 <--> LED (pwm)
# P9.31 <--> SCK
# P9.30 <--> MOSI
# P9.27 <--> DC
# P9.25 <--> RESET
# P9.28 <--> CS
#
# Touch (ADS7846/XPT2046):
# P9.23 <--> T_IRQ (gpio)
# P9.18 <--> T_DO
# P9.21 <--> T_DIN
# P9.17 <--> T_CS
# P9.22 <--> T_CLK
#
# I2C Gyro (ADXL345):
# GND   <--> GND
# 3.3v  <--> VCC
# 3.3v  <--> CS
# GND   <--> SDO
# P9.20 <--> SDA
# P9.19 <--> SCL
#

FIRMPATH='/lib/firmware/'
OVERLAYPATH='./bb.org-overlays/'
DTSPATH='src/arm/'
LCD='BB-LCD-ADAFRUIT-24-SPI1-00A0'
TOUCH='BB-ADS7846-00A0'

./init/setupgyro.sh

# Download BB Overlays Repo and compile *.dtbo files
git clone git@github.com:beagleboard/bb.org-overlays.git
cp ./init/${TOUCH}.dts ${OVERLAYPATH}${DTSPATH} 
cd ${OVERLAYPATH}
make ${DTSPATH}${LCD}.dtbo
make ${DTSPATH}${TOUCH}.dtbo
sudo cp ./${DTSPATH}${LCD}.dtbo ${FIRMPATH}
sudo cp ./${DTSPATH}${TOUCH}.dtbo ${FIRMPATH}
cd ../

# Install overlays - MODIFIES /boot/uEnv.txt!!!
sudo sed -i -re 's/(#{1}?)(dis(.+)video).+?/\2=1/' /boot/uEnv.txt
sudo sed -i -re 's/(#{1}?)(u.+addr4=).+?/\2\/lib\/firmware\/BB\-LCD\-ADAFRUIT\-24\-SPI1\-00A0\.dtbo/' /boot/uEnv.txt
sudo sed -i -re 's/(#{1}?)(u.+addr5=).+?/\2\/lib\/firmware\/BB\-ADS7846\-00A0\.dtbo/' /boot/uEnv.txt

# Compile Splash
gcc -o ./splash/splash ./splash/splash.c

sudo cp ./splash.service /etc/systemd/system

sudo cp ./splash/splash /usr/sbin
sudo cp ./init/setupgyro.sh /usr/sbin
sudo systemctl enable splash
sudo systemctl start splash

