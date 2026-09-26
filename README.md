# Build instructions

## Native build
### Windows (w64devkit)
Note: w64devkit has been used to develop it, but any mingw setup should work.

Run `build_vendor.bat`

If you wish for a debug build run `build_debug.bat` or if you simply wish to run the game run `run.bat`

## Web build 

This will output a working web page in `build/bin/web` and you need to start a web server from that folder, for example by navigating to it and running `python -m http.server`. The latest web build is also built and published to github pages whenever a commit is pushed to main.

### Windows
To make a web build requires emscripten to be installed. The script will attempt to run the required setup script by assuming `emsdk` is located at `C:\emsdk`, if that is not the case you need to ensure that you run the script from a shell with those variables set already.

Run `build_vendor_emscripten.bat`

Run `build_web.bat`

### linux
This script makes the assumption that you have sourced the environment variables already.

Run `build_vendor_emscripten.sh`

Run `build_web.sh`