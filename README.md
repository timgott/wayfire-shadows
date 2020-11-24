# WinDecor Plugin for Wayfire

This is the alternate window decorator plugin to draw titlebar.

## Compile and install
You should have first compiled and installed wlroots, wf-config and wayfire.

- Get the sources
  - `git clone https://gitlab.com/marcusbritanicus/windecor`
- Enter the `windecor`
  - `cd windecor`
- Configure the project - we use meson for project management
  - `meson build --prefix=/usr --buildtype=release`
- Compile and install - we use ninja
  - `ninja -C build -k 0 -j $(nproc) && sudo ninja -C build install`

## Usage
To use windecor plugin to render the decorations, disable all other decoration plugins. You may use wcm to configure your wayfire config file.
If you make changes to the configurations, you will have to close and restart wayfire for the changes to take effect.
