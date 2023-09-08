# winshadows

### Window Shadows Plugin for Wayfire

This is a plugin for wayfire that adds window shadows. The code was initially a
fork of <https://gitlab.com/wayfireplugins/windecor> but only a small part of
that remains in the code.

## Screenshots

By default, the plugin will add fast and nice shadows around windows that use server side decorations. Additionally there is an option to make the focused window glow.

![image](https://github.com/timgott/wayfire-shadows/assets/18331942/94d27159-573c-4613-8bf6-4527502539f5)

<details>
<summary>Config used in screenshot</summary>

```ini
[decoration]
active_color = \#7A98BA76
border_size = 4
inactive_color = \#20201838
title_height = 0

[winshadows]
glow_enabled = true
shadow_radius = 50
vertical_offset = 10
horizontal_offset = 5
# remaining values are default

```
</details>

## Install

1. Get the sources
   ```bash
   git clone https://github.com/timgott/wayfire-shadows.git
   cd wayfire-shadows
   ```
2. ⚠️ Switch to the backport0.7 branch if you use wayfire version 0.7 (last stable release)
   ```bash
   git checkout backport0.7 # (if necessary)
   ```
3. Configure with meson and build & install with ninja.
   ```bash
   meson build --buildtype=release
   cd build
   # meson configure --prefix=... # if wayfire is not installed in /usr/local
   ninja
   sudo ninja install
   ```

## Install with [wfplug](https://github.com/timgott/wfplug)

As above, you have to change the branch on wayfire 0.7.

```bash
# enable wfplug
source ~/wfplug/activate  # edit path if necessary

# get plugin
wfplug-goto-plugins
git clone https://github.com/timgott/wayfire-shadows.git winshadows

# build and install to wfplug
wfplug-build winshadows
```

Try a testconfig with

```bash
wfplug-test winshadows bluelight
```

