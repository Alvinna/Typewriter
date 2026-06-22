#!/bin/sh

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <mountpoint src> <mountpoint dst>"
    exit 1
fi

mount --bind "$1" "$1"
mount --make-shared "$1"
mount --bind "$1" "$2"
