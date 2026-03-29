#!/bin/bash

ffmpeg -i /Users/ahmed/bad_apple/vid.mp4  -vf "scale=128:64,format=gray" -r 10 frames/frame_%04d.png
