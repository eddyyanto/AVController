## AV & Dataton WatchOut Controller

A custom control system built for 2 dome projection system showflats. This repository contains the code built on top of Arduino Ethernet to automate the sytem power up and down for the following AV devices:
- An amplifier, originally used infrared which at later stage updated to use TCP socket for relability
- WatchOut cluster display computers (using relay)
- Projectors (using TCP sockets)

### Board IO Assignment

- Port 1 (Previously Unused)	: Pin 4, Ophir Duo
- Port 2						: Pin 6, Off
- Port 3						: Pin 8, Marina One
- Port 4						: Pin A2, On

### AV Devices Assignment

- Projectors		: 192.168.1-8:4532
- Amplifier			: 192.168.1.10:23
- WatchOut Cluster	: 192.168.1.51:3039
- Control board 	: 192.168.1.48:3038

### Dependency

[IRremote Arduino Library](https://github.com/z3t0/Arduino-IRremote)