#!/bin/sh

start_gps()
{
	sudo modprobe cdc-wdm
	sleep 1
	sudo mbm_gpsd -d 2
	sleep 1
	sudo gpsd -D 2 -b /tmp/gps0
}

stop_gps()
{
	sudo killall gpsd
	sleep 1
	sudo killall mbm_gpsd
}

case "$1" in
	start)
		start_gps
		;;
	stop)
		stop_gps
		;;
	sleep)
		sudo killall -USR1 mbm_gpsd
		;;
	wakeup)
		sudo killall -USR2 mbm_gpsd
		;;
	*)
		echo "Usage: $0 {start|stop}"
		;;
esac

exit 0
