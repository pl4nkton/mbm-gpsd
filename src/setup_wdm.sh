#!/bin/bash
sudo modprobe cdc-wdm
sleep 2
sudo chmod a+rwx /dev/cdc-wdm*
