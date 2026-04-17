# vkdecor
![vkdecor](https://github.com/soreau/vkdecor/assets/1450125/af891554-8eeb-4769-b571-fa587afd8350)

A highly configurable decorator plugin for wayfire, vkdecor features antialiased rounded corners with shadows and optional animated effects.

## Installing

Set `--prefix` to the same as the wayfire installation.

```
$ meson setup build --prefix=/usr
$ ninja -C build
# ninja -C build install
```

Restart wayfire.

## Running

Disable other decorator plugins and enable vkdecor plugin in core section of `wayfire.ini`.
