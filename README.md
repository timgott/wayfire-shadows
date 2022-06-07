# Window Shadows Plugin for Wayfire

This is a plugin for wayfire that adds window shadows. The code was initially a
fork of <https://gitlab.com/wayfireplugins/windecor> but only a small part of
that remains in the code.

## Compile and install

This branch targets wayfire master version. If you want to build for wayfire 0.7.x, switch to the branch `backport0.7`.

You should have first compiled and installed wlroots, wf-config and wayfire.

- Get the sources
  - `git clone https://github.com/timgott/wayfire-shadows.git`
- Enter the cloned folder
  - `cd wayfire-shadows`
- Configure the project with meson (point to your Wayfire installation prefix with `--prefix=...` if necessary)
  - `meson build --buildtype=release`
- Compile and install using ninja
  - `ninja -C build && sudo ninja -C build install`

## Screenshots

By default, the plugin will add fast and nice shadows around windows that use server side decorations.
![image](https://raw.github.com/timgott/wayfire-shadows/screenshots/screenshots/screenshot_stripes.png)

Bonus: If you really want to, you can also use the plugin to make windows glow.
![image](https://raw.github.com/timgott/wayfire-shadows/screenshots/screenshots/screenshot_sunset.png)
