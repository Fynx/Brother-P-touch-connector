#!/usr/bin/python3

import subprocess
from sys import stdin
from time import sleep

device_path = "/dev/usb/lp1"

subprocess.check_output(["./wr", "initialise", "-o", device_path])

attempts = 5
for i in range(0, attempts):
	print("requesting status...")
	subprocess.check_output(["./wr", "status", "-o", device_path])

	sleep(5)

	try:
		output = subprocess.check_output(["./read_status", "-i", device_path]).decode("utf-8")
		break
	except subprocess.CalledProcessError:
		print("failed")
else:
	raise "Failed to read printer status"

print("")
print(output)
