optusb: optusb.c usb.h libusb.a
	gcc -I `brew --prefix libusb-compat`/include/usb.h -L `brew --prefix libusb-compat`/lib -lusb -Wall -o optusb optusb.c

clean:
	rm -rf optusb *.o