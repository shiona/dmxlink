USB=3

build:
	arduino-cli compile -b arduino:avr:nano --warnings all DMXtransmitter.ino

flash:
	arduino-cli upload -b arduino:avr:nano -p /dev/ttyUSB${USB} --board-options cpu=atmega328old

monitor:
	arduino-cli monitor -b arduino:avr:nano -p /dev/ttyUSB${USB} --board-options cpu=atmega328old
