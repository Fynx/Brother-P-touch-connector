#!/usr/bin/python3

import argparse
import subprocess
from sys import stdin
from time import sleep


def parse_args():
	parser = argparse.ArgumentParser()

	parser.add_argument("--initialise", action='store_true')
	parser.add_argument("--device-path", required=False, default="/dev/usb/lp1")
	parser.add_argument("--tape-colour", required=True)
	parser.add_argument("--tape-width", required=True)
	parser.add_argument("--tape-type", required=True)
	parser.add_argument("--text-colour", required=True)

	return parser.parse_args()


def request_status(device_path, attempts=5, wait=1):
	for i in range(0, attempts):
		print("requesting status...")
		subprocess.check_output(["./wr", "status", "-o", device_path])

		sleep(wait)

		try:
			output = subprocess.check_output(["./read_status", "-i", device_path]).decode("utf-8").strip().split("\n")
			break
		except subprocess.CalledProcessError:
			print("failed")
	else:
		raise "Failed to read printer status"

	return {e[0].strip(): ":".join(e[1:]).strip() for line in output if (e := line.split(":"))}


def verify_args(args, status):
	if status["error information"]:
		raise Exception(f"Error: {status['error information']}")
	if status["phase"] != "editing state":
		raise Exception("Printer must be in 'editing state' phase")
	if args.tape_width not in status["media width"].split(" / "):
		raise Exception(f"Mismatched tape width: {args.tape_width}")
	if args.tape_type != status["media type"]:
		raise Exception(f"Mismatched tape type: {args.tape_type}")
	if args.tape_colour != status["tape colour"]:
		raise Exception(f"Mismatched tape colour: {args.tape_colour}")
	if args.text_colour != status["text colour"]:
		raise Exception(f"Mismatched text colour: {args.text_colour}")


def main():
	args = parse_args()

	if args.initialise:
		subprocess.check_output(["./wr", "initialise", "-o", args.device_path])

	status = request_status(args.device_path)

	print("")
	for k, v in status.items():
		print(f"{k}: {v}")
	print("")

	verify_args(args, status)


if __name__ == "__main__":
	main()
