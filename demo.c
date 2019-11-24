#include <stdio.h>
#include <stdlib.h>	
#include <string.h>	
#include <unistd.h>

#include <libusb.h>

#define DEV_VID 0x1c7a
#define DEV_PID 0x0570
#define DEV_CONF 0x1
#define DEV_INTF 0x0

#define DEV_EPOUT 0x04
#define DEV_EPIN 0x83

#define IMAGE_WIDTH 114
#define IMAGE_HEIGHT 57
#define OUTPUT_LENGTH 32512

static unsigned char egis[4] = { 0x45, 0x47, 0x49, 0x53 };

struct eh570_msg {
	unsigned char header[4];
	unsigned char command;
	unsigned char key;
	unsigned char vaue;
} __attribute__((packed));

static unsigned char init[][3] = 
{
	{ 0x01, 0x0d, 0x00 },
	{ 0x01, 0x0e, 0x00 },
	{ 0x01, 0x02, 0x1f },
	{ 0x01, 0x05, 0x08 },
	{ 0x01, 0x03, 0xff },

	{ 0x01, 0x10, 0x00 },
	{ 0x01, 0x11, 0x38 },
	{ 0x01, 0x12, 0x00 },
	{ 0x01, 0x13, 0x71 },

	{ 0x01, 0x20, 0x41 },
	{ 0x01, 0x58, 0x41 },
	{ 0x01, 0x21, 0x09 },
	{ 0x01, 0x57, 0x09 },
	{ 0x01, 0x22, 0x02 },
	{ 0x01, 0x56, 0x02 },
	{ 0x01, 0x23, 0x01 },
	{ 0x01, 0x55, 0x01 },
	{ 0x01, 0x24, 0x00 },
	{ 0x01, 0x54, 0x00 },
	{ 0x01, 0x25, 0x00 },
	{ 0x01, 0x53, 0x00 },
	
	{ 0x01, 0x15, 0x00 },
	{ 0x01, 0x16, 0x41 },
	{ 0x01, 0x09, 0x0a },
	{ 0x01, 0x14, 0x00 },
	{ 0x01, 0x02, 0x0f },
	{ 0x01, 0x03, 0x80 },

	{ 0x00, 0x02, 0x80 },

	{ 0x01, 0x02, 0x2f },
	{ 0x06, 0x00, 0xfe }
};

static unsigned char repeat[][3] =
{
	{ 0x01, 0x02, 0x0f },
	{ 0x00, 0x02, 0x0f },
	{ 0x01, 0x02, 0x2f },
	{ 0x06, 0x00, 0xfe }
};


void perror_exit(const char * msg)
{
	perror(msg);
	exit(0);
}

void writeImages(int index, unsigned char * data, int width, int height)
{
	const char format_mark[] = "P2";
	printf("Writing images: %d (%dx%d)\n", index, width, height);

	for (int i=0; i<5; i++) {
		char filename[15];
		sprintf(filename, "images/%d-%d.pgm", index, i);
		FILE * fp = fopen(filename, "w");
		if (fp == NULL) {
			perror("Error opening file");
			return;
		}

		fprintf(fp, "%s\n%d %d\n%d\n", format_mark, width, height, 255);

		int j = 0;
		for (int x = 0; x < height; ++x) {
			for (int y = 0; y < width; ++y, ++j) {
				fprintf(fp, "%03d ", data[j]);
			}
			fputc('\n', fp);
		}

		fclose(fp);
	}
}

void send_requests(libusb_device_handle *handle, struct eh570_msg *msg, unsigned char requests[][3], int count, unsigned char *data, int transferred) {
	printf("Sending %d requests, buffer size = %d\n", count, OUTPUT_LENGTH);
	unsigned char * buffer = (unsigned char *)msg;
	for (int i = 0; i < count; ++i) {
		memcpy(buffer + sizeof(egis), requests[i], sizeof(requests[i]));
		if (libusb_bulk_transfer(handle, DEV_EPOUT, (unsigned char *)msg, 7, &transferred, 0) != 0) {
			perror_exit("'Out' transfer error for request");
		}
		// printf("Write %d: %d\n", i, transferred);

		if (libusb_bulk_transfer(handle, DEV_EPIN, data, OUTPUT_LENGTH, &transferred, 0) != 0) {
			perror_exit("'In' transfer error");
		}
		// printf("Read %d: %d\n", i, transferred);
	}
}

int main(int argc, char * argv[])
{
	printf("Here goes...\n");

	libusb_context * cntx = NULL;
	if (libusb_init(&cntx) != 0)
		perror_exit("libusb_init failed");

	//libusb_set_debug(cntx, LIBUSB_LOG_LEVEL_DEBUG);

	libusb_device_handle * handle = libusb_open_device_with_vid_pid(cntx, DEV_VID, DEV_PID);
	if (handle == NULL)
		perror_exit("Open device failed");

	if (libusb_kernel_driver_active(handle, DEV_INTF) == 1)
		libusb_detach_kernel_driver(handle, DEV_INTF);

	if (libusb_set_configuration(handle, DEV_CONF) != 0)
		perror_exit("Set configuration failed");

	if (libusb_claim_interface(handle, DEV_INTF) != 0)
		perror_exit("Claim interface failed");

	libusb_reset_device(handle);

	printf("Starting capture...\n");
	
	unsigned char data[OUTPUT_LENGTH];
	int transferred = 0;
	
	int request_size = sizeof(struct eh570_msg);
	struct eh570_msg * msg;
	msg = malloc(request_size);
	memcpy(msg, egis, sizeof(egis));

	int count = sizeof(init)/sizeof(init[0]);
	send_requests(handle, msg, init, count, data, transferred);

	writeImages(0, data, 114, 57);

	count = sizeof(repeat)/sizeof(repeat[0]);
	for (int i = 1; i <= 5; i++) {
		send_requests(handle, msg, repeat, count, data, transferred);
		writeImages(i, data, 114, 57);
		sleep(0.2);
	}

	free(msg);

	libusb_release_interface(handle, DEV_INTF);
	libusb_attach_kernel_driver(handle, DEV_INTF);
	libusb_close(handle);
	libusb_exit(cntx);
	return 0;
}
