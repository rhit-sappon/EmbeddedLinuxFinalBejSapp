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
# P9.16 <--> lite (pwm)
# P9.25 <--> reset
# P9.27 <--> dc
# P9.28 <--> tft_cs
# P9.29 <--> miso
# P9.30 <--> mosi
# P9.31 <--> clk
#
# Touch (ADS7846/XPT2046):
# P9.23 <--> T_IRQ (gpio)
# P9.18 <--> T_DO
# P9.21 <--> T_DIN
# P9.17 <--> T_CS
# P9.22 <--> T_CLK
#
# I2C Gyro (ADXL345):
#


FIRMPATH='/lib/firmware/'
OVERLAYPATH='./bb.org-overlays/'
DTSPATH='src/arm/'
LCD='BB-LCD-ADAFRUIT-24-SPI1-00A0'
TOUCH='BB-ADS7856-00A0'

./init/setupgyro.sh

# Download BB Overlays Repo and compile *.dtbo files
git clone git@github.com:beagleboard/bb.org-overlays.git
cp ./init/${TOUCH}.dts ${OVERLAYPATH}${DTSPATH} 
cd ${OVERLAYPATH}
make ${LCD}.dtbo
make ${TOUCH}.dtbo
cp ./${DTSPATH}${LCD} ${FIRMPATH}
cp ./${DTSPATH}${TOUCH} ${FIRMPATH}
make clean
cd ../

# Install overlays - MODIFIES /boot/uEnv.txt!!!


# Compile Splash
gcc -o ./splash/splash ./splash/splash.c

