# djifix

Mirror of http://live555.com/drones/DJI-Phantom-2-Vision+-video-fix/

Repair corrupt DJI video files.

## Install

```bash
make
make install
```

## Usage

```bash
djifix path/to/video/DJI_XYZW
ffmpeg -i DJI_XYZW-repaired.h264 -c copy DJI_XYZW-repaired.mp4
```
