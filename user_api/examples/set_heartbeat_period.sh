#/bin/bash
#
# this example script sets heartbeat period as one half of timeout
#
if [ -z $2 ] ; then
	echo usage:
	echo  -e "\t" $0 "<card> <segment>"
	exit 1
fi
if ! [ -d /proc/niagara/$1/$2 ] ; then
	echo Cannot find /proc/niagara/$1/$2
	exit 1
fi
timeout=`cat /proc/niagara/$1/$2/timeout_scaler`
echo TIMEOUT $timeout
period=`cat /proc/niagara/$1/$2/heartbeat_period`
echo OLD PERIOD $period
period=`echo "obase=16;ibase=16;$timeout/2" | bc -q`
echo NEW PERIOD $period
echo $period > /proc/niagara/$1/$2/heartbeat_period
