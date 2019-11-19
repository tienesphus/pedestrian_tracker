#!/usr/bin/env bash
# makes a directory in ram for quick writing of image files between processes
# TODO run this file from the CMAKE script
mount -t tmpfs -o size=50m tmpfs ram_disk
