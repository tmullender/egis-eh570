#! /usr/bin/env python

# This will parse a CSV generated from a USB pcap file
# The CSV can be generated with the following command
# tshark -r <pcap> -T fields -e _ws.col.No. -e _ws.col.Info -e _ws.col.Length -eusb.capdata -Eseparator=,

import os
import sys

EGIS_CONTROL = [69, 71, 73, 83, 1]
SIGE = [83, 73, 71, 69]
IMAGE_WIDTH = 114
IMAGE_HEIGHT = 57
IMAGE_COUNT = 5
IMAGE_SIZE = IMAGE_HEIGHT * IMAGE_WIDTH


class Message:

    def __init__(self, inum, isize, binbound, adata):
        self.num = inum
        self.size = isize
        self.inbound = binbound
        self.command_string = adata.strip()[-8:]
        self.data = list(map(lambda x: int(x, 16), adata.split(":")))

    def is_control(self):
        return self.data[0:5] == EGIS_CONTROL or (self.data[0:4] == SIGE and self.data[6] == 1)

    def get_command(self):
        return self.data[-3:]

    def is_image(self):
        return self.inbound and self.size == 32539

    def create_images(self, path):
        image_dir = os.path.join(path, "%d" % self.num)
        if not os.path.exists(image_dir):
            os.mkdir(image_dir)
        for i in range(IMAGE_COUNT):
            filename = os.path.join(image_dir, "%d.pgm" % i)
            with open(filename, 'w') as writer:
                writer.write("P2\n%d %d\n255\n" % (IMAGE_WIDTH, IMAGE_HEIGHT))
                image_data = list(map(lambda x: "% 3d" % x, self.data[i*IMAGE_SIZE:(i+1)*IMAGE_SIZE]))
                rows = (image_data[i:i + IMAGE_WIDTH] for i in range(0, IMAGE_SIZE, IMAGE_WIDTH))
                writer.writelines(map(lambda y: "".join(y), rows))


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Tell me where the file is")
        exit(1)

    messages = {}
    commands = {}
    input_file = sys.argv[1]
    output_dir = os.path.join(os.path.dirname(input_file), 'images')
    if not os.path.exists(output_dir):
        os.mkdir(output_dir)
    with open(input_file) as reader:
        for line in reader.readlines():
            parts = line.split(",")
            num = int(parts[0])
            inbound = "in" in parts[1]
            size = int(parts[2])
            data = parts[3]
            message = Message(num, size, inbound, data)
            messages[message.num] = message
            if message.is_image():
                message.create_images(output_dir)
            if not message.inbound:
                command = message.data[4]
                register = message.data[5]
                value = message.data[6]
                if command not in commands:
                    commands[command] = {}
                if register not in commands[command]:
                    commands[command][register] = {}
                if value not in commands[command][register]:
                    commands[command][register][value] = []
                commands[command][register][value].append(message.num)

    print(len(messages))
    for k in sorted(commands.keys()):
        for r in sorted(commands[k].keys()):
            for v in sorted(commands[k][r].keys()):
                print(k, r, v, len(commands[k][r][v]), commands[k][r][v])
