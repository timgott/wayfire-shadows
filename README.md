# Window Shadows Plugin for Wayfire

Use this branch to build the plugin on 0.7.x. Use `master` for wayfire master version.

## Compile and install
If you have installed `wayfire` from your package manager you might have to install `wayfire-devel`.

- Get the sources
  - `git clone https://github.com/timgott/wayfire-shadows.git`
- Enter the cloned folder
  - `cd wayfire-shadows`
- Configure the project with meson
  - `meson build --buildtype=release`
- Compile and install using ninja
  - `ninja -C build && sudo ninja -C build install`
