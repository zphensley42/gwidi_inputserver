#!/bin/bash

echo "bad_header_msg" > /dev/udp/127.0.0.1/5577
echo "msg_hello" > /dev/udp/127.0.0.1/5577
# dd if=binary.dat bs=160 count=1 > /dev/udp/127.0.0.1/5577
