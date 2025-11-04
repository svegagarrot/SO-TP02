#!/bin/bash
#Flags para audio y la hora
if [[ "$1" = "gdb" ]]; then
    qemu-system-x86_64 -s -S -hda Image/x64BareBonesImage.qcow2 -m 512 -d int
else
    qemu-system-x86_64 -machine type=pc,pcspk-audiodev=speaker -audiodev driver=coreaudio,id=speaker -hda Image/x64BareBonesImage.qcow2 -m 512 -rtc base=localtime
fi