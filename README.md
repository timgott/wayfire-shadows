# winshadows

### Window Shadows Plugin for Wayfire

This is a plugin for wayfire that adds window shadows. The code was initially a
fork of <https://gitlab.com/wayfireplugins/windecor> but only a small part of
that remains in the code.

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
source ~/wfplug/activate  # edit path if necessary
wfplug-goto-plugins
git clone https://github.com/timgott/wayfire-shadows.git winshadows
wfplug-build winshadows
```

Try a testconfig with

```bash
wfplug-test winshadows bluelight
```

## Screenshots

By default, the plugin will add fast and nice shadows around windows that use server side decorations.
![image](https://raw.github.com/timgott/wayfire-shadows/screenshots/screenshots/screenshot_stripes.png)

Bonus: The plugin can additionally make the focused window glow.
![image](https://raw.github.com/timgott/wayfire-shadows/screenshots/screenshots/screenshot_glass_glow.png)

<details>
<summary>Config used in last screenshot</summary>

```
[decoration]
active_color = \#A8A0C9A4
border_size = 4
inactive_color = \#20252338
title_height = 0

[winshadows]
glow_color = \#97AFCD26
glow_radius = 40
shadow_color = \#00000033
shadow_radius = 20
```
</details>
