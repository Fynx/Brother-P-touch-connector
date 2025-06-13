#!/usr/bin/python3

import argparse
import os
import subprocess
from time import sleep


def request_status(args):
	request_status_cmd = ["./make_request", "status", "-o", args.device_path]
	if args.verbose:
		print(" ".join(request_status_cmd))
	subprocess.check_output(request_status_cmd)


def read_status(args):
	read_status_cmd = ["./read_status", args.device_path]
	error = None
	output = None
	try:
		if args.verbose:
			print(" ".join(read_status_cmd))
		output = subprocess.check_output(read_status_cmd, stderr=subprocess.STDOUT).decode("utf-8").strip().split("\n")
	except subprocess.CalledProcessError as e:
		error = e.output.decode().strip()
		return None, error

	return {
		e[0].strip(): ":".join(e[1:]).strip()
		for line in output if (e := line.split(":"))
	}, None


def get_status(args, attempts=5, wait=1):
	for i in range(0, attempts):
		request_status(args)
		sleep(wait)
		output, error = read_status(args)
		if error and args.verbose:
			print("failed")
		if not error:
			return output
	else:
		raise Exception(f"Failed to read printer status: '{error}'")


def verify_args(args, status):
	if args.image != "test" and not os.path.exists(args.image):
		raise Exception(f"Path {args.image} does not exist")
	if status["error information"]:
		raise Exception(f"Error: {status['error information']}")
	if status["phase"] != "editing state":
		raise Exception("Printer must be in 'editing state' phase")
	if args.tape_width not in status["media width"].split(" / "):
		raise Exception(f"Mismatched tape width: {args.tape_width}, expected {status['media width']}")
	if args.tape_type != status["media type"]:
		raise Exception(f"Mismatched tape type: {args.tape_type}, expected {status['media type']}")
	if args.tape_colour != status["tape colour"]:
		raise Exception(f"Mismatched tape colour: {args.tape_colour}, expected {status['tape colour']}")
	if args.text_colour != status["text colour"]:
		raise Exception(f"Mismatched text colour: {args.text_colour}, expected {status['text colour']}")


def make_request(args):
	if args.remove_output:
		try:
			print(f"Removing {args.output_path}")
			os.remove(args.output_path)
		except FileNotFoundError:
			pass

	subprocess.check_output(["./make_request", "initialise", "-o", args.output_path])
	subprocess.check_output(["./make_request", "status", "-o", args.output_path])

	command = [
		"./make_request", "print",
		"-i", f"'{args.image}'",
		"-o", f"'{args.output_path}'",
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

	if args.verbose:
		print(" ".join(command))
	subprocess.check_output(command)

	if args.output_path != args.device_path:
		print(f"Write request to: {args.output_path}")


def parse_args():
	parser = argparse.ArgumentParser()
	subparsers = parser.add_subparsers(dest="group", required=True)

	parser_status = subparsers.add_parser("status")
	parser_status.add_argument("--verbose", "-v", action="store_true")
	parser_status.add_argument("--device-path", "-d", required=True, default="/dev/usb/lp1", help="file to communicate status request and response with")

	parser_print = subparsers.add_parser("print")
	parser_print.add_argument("--verbose", "-v", action="store_true")
	parser_print.add_argument("--device-path", "-d", required=True, help="file to communicate status request and response with")
	parser_print.add_argument("--output-path", "-o", required=False, default="/tmp/request.prn", help="file to APPEND print request to, for practical purposes same as device path")
	parser_print.add_argument("--remove-output", action="store_true", help="removes the output file before creation")
	parser_print.add_argument("--tape-colour", required=True)
	parser_print.add_argument("--tape-width", required=True)
	parser_print.add_argument("--tape-type", required=True)
	parser_print.add_argument("--text-colour", required=True)
	parser_print.add_argument("--image", "-i", required=True, help="use 'test' for test page printing")
	parser_print.add_argument("--compression", required=False, choices=["no compression", "tiff"], default="tiff")
	parser_print.add_argument("--copies", required=False, default=1, type=int)
	parser_print.add_argument("--set-length-margin", required=False, type=int, default=14)
	parser_print.add_argument("--no-auto-cut", action="store_true")
	parser_print.add_argument("--no-half-cut", action="store_true")
	parser_print.add_argument("--chain-printing", action="store_true")
	parser_print.add_argument("--img-scale-up", action="store_true", help="scales the image up if it's way too small")
	parser_print.add_argument("--img-center", action="store_true", help="centers the image if it's too small")
	parser_print.add_argument("--img-scale-down", action="store_true", help="scales the image down if it's too big")

	return parser.parse_args()


def main():
	args = parse_args()

	# get rid of data in the device
	error = None
	while not error:
		_, error = read_status(args)
	if not error.startswith("Empty file"):
		raise Exception(f"Expected '{args.device_path}' to be empty, but got '{error}' when attempting to read")

	status = get_status(args)

	if args.group == "status" or args.verbose:
		print("")
		for k, v in status.items():
			print(f"{k}: {v}")
		print("")

	if args.group == "print":
		verify_args(args, status)
		make_request(args)


if __name__ == "__main__":
	main()
