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
	parser.add_argument("--set-length-margin", required=False, type=int, default=14)
	parser.add_argument("--no-auto-cut", action="store_true")
	parser.add_argument("--no-half-cut", action="store_true")
	parser.add_argument("--chain-printing", action="store_true")
	parser.add_argument("--img-scale-up", action="store_true", help="scales the image up if it's way too small")
	parser.add_argument("--img-center", action="store_true", help="centers the image if it's too small")
	parser.add_argument("--img-scale-down", action="store_true", help="scales the image down if it's too big")

	return parser.parse_args()


def request_status(device_path, attempts=5, wait=1):
	for i in range(0, attempts):
		request_status_cmd = ["./make_request", "status", "-o", device_path]
		print(" ".join(request_status_cmd))
		subprocess.check_output(request_status_cmd)

		sleep(wait)

		read_status_cmd = ["./read_status", device_path]
		try:
			print(" ".join(read_status_cmd))
			output = subprocess.check_output(read_status_cmd).decode("utf-8").strip().split("\n")
			break
		except subprocess.CalledProcessError:
			print("failed")
	else:
		raise Exception("Failed to read printer status")

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

	command = [
		"./make_request", "print",
		"-i", f"'{args.image}'",
		"-o", f"'{request_path}'",
		"--copies", str(args.copies),
		"--compression", "tiff",
		"--tape-type", f"'{args.tape_type}'",
		"--tape-width", f"'{args.tape_width}'",
		"--set-length-margin", str(args.set_length_margin),
	]
	if args.no_auto_cut:
		command.append("--no-auto-cut")
	if args.no_half_cut:
		command.append("--no-half-cut")
	if args.chain_printing:
		command.append("--chain-printing")
	if args.img_scale_down:
		command.append("--scale-down")
	if args.img_scale_up:
		command.append("--scale-up")
	if args.img_center:
		command.append("--center")

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
