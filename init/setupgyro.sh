#!/usr/bin/env bash
# //////////////////////////////////////
#	setup.sh
#	Enables the ADXL345 on i2c-2
#//////////////////////////////////////
I2C2PATH='/sys/class/i2c-adapter/i2c-2/'

cd ${I2C2PATH}
sudo chown debian:debian *
sudo echo adxl345 0x53 > new_device
sudo chown debian:debian *
cd
