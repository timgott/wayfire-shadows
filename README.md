# Window Shadows Plugin for Wayfire

This is a plugin for wayfire that adds window shadows. The code was initially a
fork of <https://gitlab.com/wayfireplugins/windecor> but only a small part of
that remains in the code.

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

## Screenshots

By default, the plugin will add fast and nice shadows around windows that use server side decorations.
![image](https://raw.github.com/timgott/wayfire-shadows/screenshots/screenshots/screenshot_stripes.png)

Bonus: If you really want to, you can also use the plugin to make windows glow.
![image](https://raw.github.com/timgott/wayfire-shadows/screenshots/screenshots/screenshot_sunset.png)
