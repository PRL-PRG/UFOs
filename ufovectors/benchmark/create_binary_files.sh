#!/bin/bash

head -c 400000 /dev/urandom > 100Kints.bin
head -c 2000000 /dev/urandom > 500Kints.bin

head -c 4000000 /dev/urandom > 1Mints.bin
head -c 64000000 /dev/urandom > 16Mints.bin
head -c 128000000 /dev/urandom > 32Mints.bin
head -c 256000000 /dev/urandom > 64Mints.bin
head -c 512000000 /dev/urandom > 128Mints.bin
#head -c 1024000000 /dev/urandom > 256Mints.bin
#head -c 2048000000 /dev/urandom > 512Mints.bin
head -c 4096000000 /dev/urandom > 1024Mints.bin