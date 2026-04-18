# vkdecor


A decorator plugin for wayfire, vkdecor is forked from Scott Moreau (soreau) pixdecor plugin and is keeping only very limited part of the initial functionality. 
The purpose is to migrate the decorator to vulkan API from GLES, so dropping majority of fancy GLES effects to simplify the transition.

I've unforked this development because it is never going to be backmerged to pixdecor, and sometimes the full masterpiece will come to vulkan world. 

But for now trying to make minimal functionality of server side deocorations to work 

The plan is to keep only  antialiased rounded corners with shadows effects droping the most advanced eye-candy.

Currently only basic decoration works/

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
