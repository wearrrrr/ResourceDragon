# Building ResourceDragon

ResourceDragon is made to be really simple to build, it officially supports, Ubuntu, Arch, and Alpine currently.

Ubuntu:
```
git clone https://github.com/wearrrrr/ResourceDragon.git

sudo apt-get update
sudo apt-get install libgl-dev libxkbcommon-x11-dev libx11-xcb-dev libzstd-dev libxcb1 libxkbcommon0 libxkbcommon-x11-0 libx11-xcb1 libwayland-client0 libwayland-cursor0 libwayland-egl1 libwayland-server0 cmake ninja-build pkgconf libtool libfuse2 appstream libasound2 libasound2-dev libpipewire-0.3-0 libpipewire-0.3-dev libaudio2 libaudio-dev -y

./build.sh
```

Alpine:
```
git clone https://github.com/wearrrrr/ResourceDragon.git

sudo apk add clang clang-extra-tools llvm lld musl-dev build-base libunwind cmake pkgconf git ninja-build ninja-is-really-ninja zlib-dev sdl3-dev pipewire-dev pipewire-pulse pipewire-jack pipewire-alsa

./build.sh -alpine
```
