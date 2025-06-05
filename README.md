# Modest Brother P-touch printer connector

## Description

Simple software communicating directly with a linux printer device without cups. Usually that means a USB connection.

Communication with the printer is done using a *manage.py* script that uses binaries. The script requires parameters such as tape type for validation (to force you to know what you're printing). By default the height of the image has to fit exactly, but that can be adjusted with relevant options.

#### make_request

Writes an initialisation/status/print request to destination. Mind that the content is _appended_ to the destination.

#### read_status

Reads 32 bytes of status data from the device, then prints the interpretation of it.

#### parse_request

Can read and interpret a print request. Diagnostics only.


## Requirements

* gcc 13
* Brother P-touch P900/P900W/P950NW printer
* printer linux device (such as /dev/usb/lp1)


## Building

`make`

For development use:

`make BUILD_TYPE="debug"`

And that's it. No additional packages are required for *manage.py*.


## Usage

Get printer status.

```
./manage.py status -d /dev/usb/lp1
```

Print a test page.

```
./manage.py print -d /dev/usb/lp1 -o /dev/usb/lp1 -i test --tape-colour white --tape-width '12 mm' --tape-type 'non-laminated tape' --text-colour black
```

Print a single local images/cat0.png image after verifying input parameters.

```
./manage.py print -d /dev/usb/lp1 -o /dev/usb/lp1 -i images/cat0.png --tape-colour white --tape-width '12 mm' --tape-type 'non-laminated tape' --text-colour black
```

Save an image printing request to default /tmp/request.prn by overwriting. Then, send it to the printer.

```
./manage.py print -d /dev/usb/lp1 -i images/cat.png --remove-output --tape-colour white --tape-width '12 mm' --tape-type 'non-laminated tape' --text-colour black
cat /tmp/request.prn > /dev/usb/lp1
```

Print three images and be verbose about all of it.

```
./manage.py print -v -d /dev/usb/lp1 -i images/cat0.png -o /dev/usb/lp1 --copies 3 --tape-colour white --tape-width '32 mm' --tape-type 'laminated tape' --text-colour black
```

Print with chain cutting on (saving tape when printing many images), half-cutting off and auto-cutting off. Also set 10 mm length margin instead of default 14 mm.

```
./manage.py print -d /dev/usb/lp1 -o /dev/usb/lp1 -i images/cat0.png --tape-colour white --tape-width '32 mm' --tape-type 'laminated tape' --text-colour black --set-length-margin 10 --chain-printing --no-auto-cut --no-half-cut
```

Scale an image down (lanczos) so the height fits, then print it.

```
./manage.py print -d /dev/usb/lp1 -o /dev/usb/lp1 -i images/cat0.png --tape-colour white --tape-width '32 mm' --tape-type 'laminated tape' --text-colour black --set-length-margin 10 --img-scale-down
```

There are also *--img-center* and *--img-scale-up* options in this particular regard.

#### Using the raw port

Print request can be also sent directly to the raw port of the printer. For that, use *make_request* directly. However, printer status will not be checked as the communication is very one-sided.

```
./make_request print -i images/cat0.png -o /tmp/request.prn --copies 1 --compression tiff --tape-type 'non-laminated tape' --tape-width '12 mm'
cat /tmp/request.prn | nc 192.168.0.90 9100 -w1
```


## Credits

This project contains png++ library (without tests). Also, the code for scaling images is based on the relevant code from boost.


## Licence

BSD 3-Clause Licence is used.
