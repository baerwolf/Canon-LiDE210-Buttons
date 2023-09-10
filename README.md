This simple demo shows polling of Canon's Lide210 scanner buttons

The protocol is very simple:
The scanner implements a seperate input endpoint (something the host receives).
From its usb descriptor it is obvious interrupt EP3 - one byte in length.
The buttons are simply mapped to individual bits in this byte.
A pressed button corresponds to its bit set. If multiple buttons are pressed during
one report interval (8ms)  - multiple bits are set.
Until the button is released, it is only reported once...

please compile with: gcc -o /tmp/lide210 lide210.c -lusb -DMYDEBUG

by S. Bärwolf, Rudolstadt 2023
