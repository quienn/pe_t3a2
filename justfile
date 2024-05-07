port := "/dev/ttyUSB0"

default: compile upload

compile:
  arduino-cli compile --verify

upload:
  arduino-cli upload -p {{port}}

monitor:
  arduino-cli monitor -p {{port}} -c baudrate=115200
