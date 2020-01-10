#!/bin/bash
#
# This script parses /proc/bus/input/devices for the device in the
# environment variable DEVPATH
# It extracts the bustype and if it matches bustype 0006 (defined by wdaemon
# as virtual bus) returns <vendor id>-<product id> of the device.
# Returns true if a match is found or false otherwise.
#
# The udev rules provided by wdaemon rely on this output to create symlinks
# for the device.
#
filename=$(basename "$DEVPATH");
header=$(egrep -B 5 -e "$filename\ |$filename\$" /proc/bus/input/devices |
		grep "Bus=" | tail -n 1);
bus=$(echo $header | cut -f 2 -d '=' | cut -f 1 -d ' ');
vendor=$(echo $header | cut -f 3 -d '=' | cut -f 1 -d ' ');
product=$(echo $header | cut -f 4 -d '=' | cut -f 1 -d ' ');
if [ ! "$bus" = "0006" ]; then
	exit 1;
fi

if [ "$1" = "wacom" -a "$vendor" -ne "056a" ]; then
	exit 1;
fi

case "$vendor-$product" in
	"056a-0000") echo "penpartner"; ;;
	"056a-0010") echo "graphire"; ;;
	"056a-0011") echo "graphire2-4x5"; ;;
	"056a-0012") echo "graphire2-5x7"; ;;
	"056a-0013") echo "graphire3"; ;;
	"056a-0014") echo "graphire3-6x8"; ;;
	"056a-0015") echo "graphire4-4x5"; ;;
	"056a-0016") echo "graphire4-6x8"; ;;
	"056a-0060") echo "volito"; ;;
	"056a-0061") echo "penstation2"; ;;
	"056a-0062") echo "volito2-4x5"; ;;
	"056a-0063") echo "volito2-2x3"; ;;
	"056a-0064") echo "penpartner2"; ;;
	"056a-0020") echo "intuos-4x5"; ;;
	"056a-0021") echo "intuos-6x8"; ;;
	"056a-0022") echo "intuos-9x12"; ;;
	"056a-0023") echo "intuos-12x12"; ;;
	"056a-0024") echo "intuos-12x18"; ;;
	"056a-0030") echo "pl400"; ;;
	"056a-0031") echo "pl500"; ;;
	"056a-0032") echo "pl600"; ;;
	"056a-0033") echo "pl600sx"; ;;
	"056a-0034") echo "pl550"; ;;
	"056a-0035") echo "pl800"; ;;
	"056a-0037") echo "pl700"; ;;
	"056a-0038") echo "pl510"; ;;
	"056a-0039") echo "dtu710"; ;;
	"056a-00c0") echo "dtf521"; ;;
	"056a-00c4") echo "dtf720"; ;;
	"056a-0003") echo "cintiq_partner"; ;;
	"056a-0041") echo "intuos2-4x5"; ;;
	"056a-0042") echo "intuos2-6x8"; ;;
	"056a-0043") echo "intuos2-9x12"; ;;
	"056a-0044") echo "intuos2-12x12"; ;;
	"056a-0045") echo "intuos2-12x18"; ;;
	"056a-00b0") echo "intuos3-4x5"; ;;
	"056a-00b1") echo "intuos3-6x8"; ;;
	"056a-00b2") echo "intuos3-9x12"; ;;
	"056a-00b3") echo "intuos3-12x12"; ;;
	"056a-00b4") echo "intuos3-12x19"; ;;
	"056a-00b5") echo "intuos3-6x11"; ;;
	"056a-003f") echo "cintiq21ux"; ;;
	"056a-0047") echo "intuos2-6x8"; ;;
	"056a-00b7") echo "intuos3-4x6"; ;;
	"056a-0065") echo "bamboo"; ;;
	"056a-00c5") echo "cintiq20wsx"; ;;
	"056a-00c6") echo "cintiq12wx"; ;;
	"056a-0017") echo "bamboofun-4x5"; ;;
	"056a-0018") echo "bamboofun-6x8"; ;;
	"056a-0069") echo "bamboo1"; ;;
	"056a-0019") echo "bamboomedium"; ;;
	"056a-00C7") echo "dtu1931"; ;;
	"056a-0090") echo "isdv4-90"; ;;
	"056a-0093") echo "isdv4-93"; ;;
	"056a-009A") echo "isdv4-9A"; ;;
	"056a-00B8") echo "intuos4-4x6"; ;;
	"056a-00B9") echo "intuos4-6x9"; ;;
	"056a-00BA") echo "intuos4-8x13"; ;;
	"056a-00BB") echo "intuos4-12x19"; ;;
	"056a-00CC") echo "cintiq-21ux2"; ;;
	"056a-00F0") echo "dtu1631"; ;;
	"056a-00CE") echo "dtu2231"; ;;
	"056a-00CC") echo "cintiq-21ux2"; ;;

	*) echo "unknown"; ;;
esac;
true;

