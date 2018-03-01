#!/bin/sh
#rsync -avu --delete --exclude=mbed --exclude=.temp --exclude=BUILD . ~/Dropbox/sketch/mbed/keyboard-usb
# fswatch -0  ~/tmp/keyboard-usb ~/Dropbox/sketch/mbed/keyboard-usb | xargs -0 -n 1 -I {} sync

sync() {
	unison -batch -prefer newer -ignore "Path {mbed,.temp,BUILD}" ~/tmp/keyboard-usb ~/Dropbox/sketch/mbed/keyboard-usb
}

while true
do
	sync
	sleep 10
done
