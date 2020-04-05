//
// o p t u s b . c
//
// Multiboot program downloader for optimize's USB-bootcable
//
// Version 1.01
//
// Target OS: Linux-x86-pc
// Build: gcc -Wall -o optusb optusb.c libusb.a
//
// NOTICE: This program requires the "libusb" library
//         http://libusb.sourceforge.net/
//
// History:
//   June 16, 2003	1st revision
//   July 20, 2003	1.01 release
//			  Bug fixed (word size confusion in send_file)
//			  More verbose error messages are inserted.
//			  read_magic() is now independent of endians.
//			  50msec waits are inserted.
//
// June 20, 2003 by Wataru Nishida (http://www.wnishida.com)
//

#include <stdio.h>	// fprintf(), stderr
#include <unistd.h>	// open(), read(), close(), sleep(), usleep()
#include <fcntl.h>	// O_RDONLY
#include "usb.h"	// "libusb" header file

// Constants

#define VERSION		"1.01"
#define MAX_PROG_SIZE	(1024 * 256)	// 256KB
#define WAIT		(50 * 1000)	// 50 msec wait

// Device descriptor for USB-bootcable

#define OPTUSB_VEND	0x0BFE		// Vendor code
#define OPTUSB_PROD	0x3000		// Product code

// Endpoints in USB-bootcable

#define OPTUSB_CMD	0x01		// Command endpoint
#define OPTUSB_IN	0x82		// Input endpoint
#define OPTUSB_OUT	0x02		// Output endpoint

// Commands required for the program transfer

#define OPTUSB_WRITE	2		// Write data
#define OPTUSB_STATUS	3		// Read status

// Negotiation codes

#define OPTUSB_MAGIC	0xFEDCBA98	// Magic number for negotiation test
#define OPTUSB_ON	0x01		// Status code bit0: GBA power
#define OPTUSB_READY	0x02		// Status code bit1: ready to load

// Global variable

struct usb_dev_handle *optusb;		// Device handler for bootcable
const char* errmsg;			// Holds error message

// Read a magic number

int read_magic(int num) {
  char cmd[ 2 ];
  unsigned char res[ 4 ];
  int ret, val;

  cmd[ 0 ] = OPTUSB_STATUS;
  cmd[ 1 ] = num;
  ret = usb_bulk_write(optusb, OPTUSB_CMD, cmd, sizeof(cmd), 1000);
  if (ret != sizeof(cmd)) {
    errmsg = "read_magic: Command transmission error";
    return -1;
   }
  usleep(WAIT);
  ret = usb_bulk_read(optusb, OPTUSB_IN, (char*) res, sizeof(res), 1000);
  if (ret != sizeof(res)) {
    errmsg = "read_magic: Data read error";
    return -1;
   }
  val  = (int) res[ 0 ];
  val += res[ 1 ] << 8;
  val += res[ 2 ] << 16;
  val += res[ 3 ] << 24;
  return val;
 }

// Send a magic number

int send_magic(int num, int magic) {
  char cmd[ 6 ];
  int ret, val;

  cmd[ 0 ] = OPTUSB_STATUS;
  cmd[ 1 ] = 0x80 + num;
  cmd[ 2 ] = magic & 0xFF;
  cmd[ 3 ] = (magic >>  8) & 0xFF;
  cmd[ 4 ] = (magic >> 16) & 0xFF;
  cmd[ 5 ] = (magic >> 24) & 0xFF;

  ret = usb_bulk_write(optusb, OPTUSB_CMD, cmd, sizeof(cmd), 1000);
  if (ret != sizeof(cmd)) {
    errmsg = "send_magic: Command transmission error";
    return -1;
   }
  usleep(WAIT);
  ret = usb_bulk_read(optusb, OPTUSB_IN, (char*) &val, sizeof(val), 1000);
  if (ret != sizeof(val)) {
    errmsg = "send_magic: Data write error";
    return -1;
   }
  return val;
 }

// Send buffer data via USB

int send_file(char *buf, int len) {
  char cmd[ 4 ];
  int ret, sizew;

  // Send program length

  sizew = 1;				// One word for program size
  cmd[ 0 ] = OPTUSB_WRITE;
  cmd[ 1 ] = sizew & 0xFF;
  cmd[ 2 ] = (sizew >> 8) & 0xFF;
  ret = usb_bulk_write(optusb, OPTUSB_CMD, cmd, 3, 1000);
  if (ret != 3) {
    errmsg = "send_file: 1st command transmission error";
    return -1;
   }
  usleep(WAIT);

  cmd[ 0 ] = len & 0xFF;
  cmd[ 1 ] = (len >>  8) & 0xFF;
  cmd[ 2 ] = (len >> 16) & 0xFF;
  cmd[ 3 ] = (len >> 24) & 0xFF;
  ret = usb_bulk_write(optusb, OPTUSB_OUT, cmd, 4, 1000);
  if (ret != 4) {
    errmsg = "send_file: File size transmission error";
    return -1;
   }
  usleep(WAIT);

  // Send data

  sizew = (len + 3) / 4;		// Program SIZE IN WORDS
  cmd[ 0 ] = OPTUSB_WRITE;		//   -- BUG FIXED in July 19, 2003 --
  cmd[ 1 ] = sizew & 0xFF;
  cmd[ 2 ] = (sizew >> 8) & 0xFF;
  ret = usb_bulk_write(optusb, OPTUSB_CMD, cmd, 3, 1000);
  if (ret != 3) {
    errmsg = "send_file: 2nd command transmission error";
    return -1;
   }
  usleep(WAIT);

  ret = usb_bulk_write(optusb, OPTUSB_OUT, buf, sizew * 4, 1000);
  if (ret != (sizew * 4)) {
    errmsg = "send_file: File data transmission error";
    return -1;
   }

  return 0;
 }

int main(int argc, char** argv) {
  struct usb_bus *bus, *busses;
  struct usb_device *dev;
  struct usb_device_descriptor *dd;
  unsigned char *buf;
  int ret, fd, len;

  // Check command argument

  if (argc != 2) {
    fprintf(stderr, "Usage: optusb mb-file-name\n");
    return 1;
   }
  fprintf(stderr, "      === optusb v%s ===\n\n", VERSION);

  // Allocate 256KB buffer

  buf = malloc(MAX_PROG_SIZE);
  if (buf == NULL) {
    fprintf(stderr, "  Error: Insufficient memory!\n");
    return 1;
   }

  // Open a program file

  fd = open(argv[ 1 ], O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "  Error: file open failure!\n");
    return 1;
   }

  // Read into all data

  len = read(fd, buf, MAX_PROG_SIZE);
  if (len == 0) {
    fprintf(stderr, "  Error: file read error!\n");
    return 1;
   }

  fprintf(stdout, "   Source file = %s\n", argv[ 1 ]);
  fprintf(stdout, "  Program size = %d\n", len);

  // Initialize "libusb" and get device information

  usb_init();
  usb_find_busses();
  usb_find_devices();
  busses = usb_get_busses();

  // Search OPTIMIZE's USB-GBA boot cable

  for (bus = busses; bus; bus = bus->next) {
    for (dev = bus->devices; dev; dev = dev->next) {
      dd = &dev->descriptor;
      if ((dd->idVendor == OPTUSB_VEND) && (dd->idProduct == OPTUSB_PROD))
        goto found;
     }
   }
  fprintf(stderr, "  Error: OPTIMIZE GBA-USB-bootcable is not found!\n");
  return 1;

  // Open the device

 found:
  optusb = usb_open(dev);

  // Setup interface #0 (configuration is already done in libusb)

  ret = usb_claim_interface(optusb, 0);
  if (ret) {
    fprintf(stderr, "        Status = Interface activation error!\n");
    return 1;
   }

  // Negotiate with the cable

  ret = read_magic(1);
  if (ret == 0x12345678 || ret == -1) {
    fprintf(stderr, "        Status = Negotiation error!\n");
    fprintf(stderr, "        Detail = %s\n", errmsg);
    return 1;
   }

  send_magic(2, OPTUSB_MAGIC);
  ret = read_magic(2);
  if (ret != OPTUSB_MAGIC || ret == -1) {
    fprintf(stderr, "        Status = Negotiation error!\n");
    fprintf(stderr, "        Detail = %s\n", errmsg);
    return 1;
   }

  send_magic(2, 0);
  ret = read_magic(0);
  if ((ret & OPTUSB_READY) == 0 ||
      (ret & OPTUSB_ON) == 0 || ret == -1) {
    fprintf(stderr, "        Status = Negotiation error!\n");
    fprintf(stderr, "        Detail = %s\n", errmsg);
    return 1;
   }

  // Send the program file

  ret = read_magic(1);
  if (ret == 0x12345678 || ret == -1) {
    fprintf(stderr, "        Status = Negotiation error!\n");
    fprintf(stderr, "        Detail = %s\n", errmsg);
    return 1;
   }
  ret = send_file(buf, len);
  if (ret) {
    fprintf(stderr, "        Status = Transmission error!\n");
    fprintf(stderr, "        Detail = %s\n", errmsg);
    return 1;
   }

  fprintf(stdout, "        Status = Successfully transferred.\n");
  return 0;
 }
