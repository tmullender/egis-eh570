# EGIS Fingerprint Sensor EH570

On an Acer Swift 3 `lsusb` lists the device as: 

```
1c7a:0570 LighTuning Technology Inc. EgisTec Touch Fingerprint Sensor
```

## Method

Based on Patrick Marlier's [example](http://pmarlier.free.fr/etes603.html)

+ Capture USB traffic with Wireshark and [USBPcap](https://desowin.org/usbpcap/)
+ Extract fields into a CSV with tshark
  * `tshark -r <pcap> -T fields -e _ws.col.No. -e _ws.col.Info -e _ws.col.Length -eusb.capdata -Eseparator=,`
+ Analyse anyway you can...
+ Integrate with [libfprint](https://fprint.freedesktop.org/libfprint-dev/index.html)

## Requests

USB out traffic starts with EGIS followed by 3 bytes 

+ The first byte is either 00, 01 or 06
+ If the first byte is 06, the other two are 00 FE and the response contains 32512 bytes 
+ If the first byte is 01, the response is a rotation of the request (possibly setting registers?)
+ If the first byte is 00, the others are either 02 0f, 02 80 or 03 1e
  + 02 0f or 02 80 preceed all 06 requests
  + The response to 02 is always 02 0f 01
  + The response to 03 is always 03 80 01

### Patterns

Requests repeat 5 patterns over and over again

+ __A__ `01:02:2f 06:00:fe 01:02:0f`
+ __B__ `00:02:0f` + A
+ __C__ `01:10:00 01:11:38 01:12:00 01:13:71 01:20:41 01:58:41 01:21:09 01:57:09 01:22:02 01:56:02 01:23:01 01:55:01 01:24:00 01:54:00 01:25:00 01:53:00 01:15:00 01:16:41 01:09:0a 01:14:00 01:02:0f 01:03:80`
+ __D__ C + `00:02:80` + A
+ __E__ `01:10:1c 01:11:1c 01:12:31 01:13:40 01:09:05 01:14:00 01:15:00 01:16:3a 01:08:1c 00:03:1c 01:03:80 01:0b:00 01:0c:ff 01:0d:00 01:0e:ff 01:02:1d 01:04:00`
+ __F__ `01:0d:00 01:0e:00 01:02:1f 01:05:08 01:03:ff`

The majority of the traffic is a D followed by several Bs repeated, F starts the process and E appears 2 or three times

## Responses

USB in frames are either 34 or 32539 bytes, which is 7 or 32512 after removing headers

+ 7 byte responses start SIGE and then contain the appropriate three bytes detailed in Requests
+ 32512 byte responses contain 5 images, 114x57 (don't yet know about the remaining 22 bytes)

## Scripts

+ analyseSensorCapture.py -> loops through all the frames, prints all the unique requests and generates all the images as [Plain PGM](http://netpbm.sourceforge.net/doc/pgm.html)s and [BMP](https://en.wikipedia.org/wiki/BMP_file_format)s
+ generatePGM.sh -> A crude script to generate a single PGM from a single frame to allow playing with image sizes

## Demo

`mkdir -p images && gcc demo.c -I /usr/include/libusb-1.0/ -l usb-1.0 -o demo -Wall && ./demo`

If you have permission to access the device, this should generate 6 images from the scanner in the images directory
