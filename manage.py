import subprocess
from sys import stdin
from time import sleep

status_request_path = "/tmp/status_request.bin"
device_path = "/dev/usb/lp1"
status_path = "/tmp/status_output.bin"

subprocess.check_output(["./wr", "initialise", "-o", device_path])

attempts = 5
for i in range(0, attempts):
	print("requesting status...")
	subprocess.check_output(["./wr", "status", "-o", device_path])

	sleep(5)

	try:
		subprocess.check_output(f"cat {device_path} > {status_path}", shell=True)
		output = subprocess.check_output(["./read_status", "-i", status_path]).decode("utf-8")
		break
	except subprocess.CalledProcessError:
		print("failed")
else:
	raise "Failed to read printer status"

print("")
print(output)
