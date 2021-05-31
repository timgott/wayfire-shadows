# WinShadows Plugin for Wayfire

This is a plugin for wayfire that adds window shadows. The code was initially a
fork of <https://gitlab.com/wayfireplugins/windecor> but only a small part of
that remains in the code.

![image](screenshot.webp)

## Compile and install
You should have first compiled and installed wlroots, wf-config and wayfire.

- Get the sources
  - `git clone https://github.com/timgott/wayfire-shadows.git`
- Enter the `wayfire-shadows`
  - `cd wayfire-shadows`
- Configure the project - we use meson for project management
  - `meson build --prefix=/usr --buildtype=release`
- Compile and install - we use ninja
  - `ninja -C build -k 0 -j $(nproc) && sudo ninja -C build install`
