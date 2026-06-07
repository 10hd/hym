# hym

A very simple CLI mp3 player since my laptop is butt.

## About

The reason I made this is because my laptop struggles to run Spotify and I thought it would be a fun project even if it's not very good. It does what I intended it to do.

## Note

I was going to try to implement threading so you can skip songs but I decided not to because I dont know how lol.

## Installation

Make sure `hym.c` and `dr_mp3.h` are in the same folder, then run

```bash
gcc hym.c -o hym
sudo mv hym /usr/local/bin/
```

## Instructions

To play a single mp3 file:

```bash
hym songName.mp3
```

To play multiple mp3 files:

```bash
hym songName1.mp3 songName2.mp3 songName3.mp3
```
or
```bash
hym path/to/songs/*.mp3
```