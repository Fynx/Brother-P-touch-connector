#!/usr/bin/python3

import argparse
import os
import subprocess
from sys import stdin
from time import sleep


def parse_args():
	parser = argparse.ArgumentParser()

	parser.add_argument("--initialise", action="store_true")
	parser.add_argument("--device-path", "-d", required=False, default="/dev/usb/lp1")
	parser.add_argument("--tape-colour", required=True)
	parser.add_argument("--tape-width", required=True)
	parser.add_argument("--tape-type", required=True)
	parser.add_argument("--text-colour", required=True)
	parser.add_argument("--image", "-i", required=True, help="use 'test' for test page printing")
	parser.add_argument("--compression", required=False, choices=["no compression", "tiff"], default="tiff")
	parser.add_argument("--copies", required=False, default=1, type=int)
	parser.add_argument("--half-cut-off", action="store_true")
	parser.add_argument("--chain-printing", action="store_true")

	return parser.parse_args()


def request_status(device_path, attempts=5, wait=1):
	for i in range(0, attempts):
		print("requesting status...")
		subprocess.check_output(["./make_request", "status", "-o", device_path])

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
	if args.image != "test" and not os.path.exists(args.image):
		raise Exception(f"Path {args.image} does not exist")
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


def make_request(args):
	request_path = "/tmp/request.prn"
	try:
		os.remove(request_path)
	except FileNotFoundError:
		pass

	subprocess.check_output(["./make_request", "initialise", "-o", request_path])
	subprocess.check_output(["./make_request", "status", "-o", request_path])

	# TODO away with this
	tape_width = {
		"3.5 mm": 4,
		"6 mm": 6,
		"9 mm": 9,
		"12 mm": 12,
		"18 mm": 18,
		"24 mm": 24,
		"36 mm": 36,
		"HS 5.8 mm": 6,
		"HS 8.8 mm": 9,
		"HS 11.7 mm": 12,
		"HS 17.7 mm": 18,
		"HS 23.6 mm": 24,
		"FLe 21 mm x 45 mm": 21,
	}[args.tape_width]

	command = [
		"./make_request", "print",
		"-i", f"'{args.image}'",
		"-o", f"'{request_path}'",
		"--copies", str(args.copies),
		"--compression", "tiff",
		"--tape-type", f"'{args.tape_type}'",
		"--tape-width", f"'{args.tape_width}'",
		"--tape-width-num", str(tape_width),
	]
	if args.half_cut_off:
		command.append("--half-cut-off")
	if args.chain_printing:
		command.append("--chain-printing")

	print(" ".join(command))
	subprocess.check_output(command)


def main():
	args = parse_args()

	if args.initialise:
		subprocess.check_output(["./make_request", "initialise", "-o", args.device_path])

	status = request_status(args.device_path)

	print("")
	for k, v in status.items():
		print(f"{k}: {v}")
	print("")

	verify_args(args, status)
	make_request(args)


if __name__ == "__main__":
	main()
