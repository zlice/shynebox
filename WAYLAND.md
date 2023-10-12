# Wayland

There's a lot missing or that I need, not counting reconfiguring.
Wayland is only drawbacks for me.

### rewrite libinput

libinput still seems to be the only way to to control input devices in Wayland.
To me the mouse feels odd with libinput on X or Wayland, most notably with synaptic
laptop devices.

X's evdev and synaptics are my preferred input controls.
I do not know if they are both able to be integrated separately into Wayland at the
same time or what the needs are.

### rewrite ROX Filer

ROX has long since been dead, but there is no replacement (even in the X world) that
I feel is suitable.

### wlroots still has forced v-sync

This was one of the initial issues I had with Wayland. Seems like it was out of
laziness or not understanding the audience. Some Wayland desktops have this feature
finally, ~~however wlroots still has open issues for v-sync disable~~.

Just got merged while wrapping this up, last week.

https://gitlab.freedesktop.org/wlroots/wlroots/-/issues/3249

### client request positioning

The compositors job to implement for now.

https://gitlab.freedesktop.org/wayland/wayland-protocols/-/issues/72

### single window sharing

Either has to be implemented in the WM itself or may not work altogether
from what I hear other people talking about with other Wayland/wlroots WMs.
Just a headache overall for something that is a privacy/security concern.

### login and lock screens

Last I tried, none of these were low resource or fast or stable or worked as I expected.

### Discord

Don't see them doing it any time soon, if at all. Between screen sharing, drag'n'drop
and XWalyand woes, it won't be the same.

### Zoom

More or less the same.

### OBS

Better I assume, but again, same.

### Lazy

And of course, if I implement any of the above or a wlroots WM...

I have to maintain them (:
