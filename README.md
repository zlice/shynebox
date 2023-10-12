# What

**shynebox** is a (X11) window manager that provides highly configurable window
decorations, hotkeys and menus to configure itself and launch applications.

A (mostly compatible) fork of Fluxbox with a little Shyne :)

Contains a lot of small little improvements, cleanup of unused, undocumented,
dead code, or complete replacements, etc. XRandr is probably the biggest add.

# Why

There were a few issues I had that made me look into the code and I kept seeing
bits I wanted to change. Was able to use C++17 'for auto' loops to make a lot
of things more readable, and then I just kept going. I had other projects ideas
that I still want to do and figured this would help brush back up on some coding.
I don't know if I'll keep making changes to this, or entertain request, but we'll see.

Some of the main motivators were

- style menus always behaving weirdly and pretty much required a restart
- creating say, 100 windows, was noticeably slow
- Firefox broke clicks for a while which was annoying even though it was only for certain sites

Is there a huge improvement in memory use, stability or performance? Not particularly.

Will you notice a difference? Probably not.

Should you use it? Go for it?

> and because I'm sure someone will ask: [WAYLAND](./WAYLAND.md)

# Changes / Removals

List of major/obvious changes a user will notice.
See [CHANGELOG](./CHANGELOG) for more "under the hood" changes.

- Replaced `Xinerama` with `XRandr` for multi-monitor setup.
- Refactored `config` and `themes`. Some themes may only be 99% compatible (mainly wildcards I assume).
- Fixed `Style` and `Config` `menus`. Reconfigures/themes work like I'd expect.
- Removed `MoveN` key binds. Made window action logic more complex and caused overhead for every motion event. Also could conflict with MouseN and ClickN bindings.
- Removed `OnToolbar` key binds. There were comments mentioning on how it was unreliable. It interfered with other toolbar items and was easy to break using key binds, and for bindings to not work at all.
- Removed `workspace arrows`. Clicking on workspace name button works to change workspaces.
- Removed `alpha`. picom and hotkeys seems to work better since you need to run a compositor anyway.
- Removed `slit`. I quit using it after some changes made clicks raise, that wouldn't lower, and I realized it was easy to break much like toolbar binds.
- Removed `Deiconify` commands. Didn't work, and if they used to, comments said they did not always work. Same can be achieved with ForEach client patterns like 'ForEach {Activate} (Minimized=yes)'
- Removed `right/left maximize` caption (titlebar) buttons.
- Removed `fallback placement`. This was a crutch for Col/Row SmartPlacements and was confusing to change placement behavior after a few windows were placed. They now place and wrap according to the focused window, which seems more appropriate.
- Removed `variable/environment/resource` setting commands and dialogs.
- Removed 'on-the-fly' `BindKey` command.
- Removed 'on-the-fly' `ToolButton` commands (relabel/map).
- Removed `DirFocus` commands (FocusLeft). Tried several different things but there is no way to make this predictable. Sometimes things make sense but you often don't know where you'll go and will entirely skip windows.
- Added `CenterPlacement` placement strategy.
- Added center align for `Iconbar`.
- Added `ClientPattern` 'Viewable' for XVisibility events. Nice to do 'NextWindow (Viewable=no)'.
- Fixed `resize` logic that would account for off-screen negative direction but not positive.
- Fixed non-opaque/xor rectangle moves. Worst offender was `StartTabbing` which could leave weird draws all over the screen.
- Fixed `NextWindow` and `menus` moving iconified windows. Previously iconified windows'  on another workspace were **moved to your current workspace** and then focused. Now cycling and activating in menus **moves you** the window's workspace and focuses it there.
- Improved ordering on `systray.pinRight`. Swapped logic so items are "left to right" in text and in tray.
- Improved `systray.pinLeft/Right` so case doesn't matter and you can use partial words.
- Improved/Fixed `Set Title` window menu command. Title changes are now ignored until the user manually changes back to an empty string.
- Improved/Fixed `OnWindow MouseN` action tracking. `Move`, `Resize`, `StartTabbing` will only stop/cancel on the button that started that action. EWMH actions for move/resize are tracked specially so something like mpv, which grabs mouse1 for drag moves, won't conflict with 'OnWindow Mod4 Mouse1 :StartMoving'.
- Improved `Unclutter`. Uses your current placement strategy to re-organize windows.
- Improved some `window/titlebar` logic behavior. IIRC, mapping 'OnTitlebar StartMoving' and 'OnWindow StartMoving' conflicted, causing titlebar clicks to not move.
- Improved `Menu placement`. Smarter and will center on current monitor for keyboard actions or under mouse for mouse actions.
- Improved `Delay` command. Micro-seconds seemed absurd, defaults to milli-seconds now. Also will look for 's' or 'm' at end of time arg for seconds/minutes.
- Improved `keys`. You can use `Super` and `Win` for `Mod4`.

### Config changes

Some `init` items have changed, like multi-screen configuration being removed
with overhaul, new `obeyHead` for xrandr behavior, and variable naming.
If you want to try to convert your old `~/.fluxbox/init` this copy-paste script
should do the trick for most Linux systems.

`$XDG_CONFIG_HOME/shynebox` is used over `~/.shynebox` if it exist.

Most other files should be drop-in compatible. A few changes:

- Keys mentioned in **Changes / Removals** above.
- Menus have a few changes, 'extramenus' picks up 'remember'.

```
#### update old fluxbox init config to new shynebox naming

olddir=".fluxbox"
oldcfg="$HOME/.fluxbox/init"
newdir="shynebox"
newcfg="shynebox/init"
if [ ! -z "$XDG_CONFIG_HOME" ] ; then
  newdir=$(echo $XDG_CONFIG_HOME | sed -e "s|${HOME}/||")/$newdir
  newcfg="$XDG_CONFIG_HOME/$newcfg"
else
  newdir=".$newdir"
  newcfg="$HOME/.$newcfg"
fi

if [ ! -d $newcfg ] ; then
  mkdir -p $newdir
  cp -r $HOME/.fluxbox/* $HOME/$newdir/.
fi

#### prune obsolete items and
#### convert to new item names

sed -e 's/session.//g' -e 's/screen..//g' $oldcfg | grep -vE "alpha|Transparency|slit|ollowModel|allowRemote|configVersion|demansAttention|ignoreBorder|menuMode|menuDelayClose|rootCommand|tabsAttachArea|^overlay|arping:" | tr '\t' ' ' | sort -d | \
sed -E \
-e "s|${olddir}|${newdir}|g" \
-e 's/^autoRaise:/autoRaiseWindows:/i' \
-e 's/^clickRaises/clickRaisesWindows/i' \
-e 's/TopToBottom/TOPBOTTOM/i' \
-e 's/BottomToTop/BOTTOMTOP/i' \
-e 's/LeftToRight/LEFTRIGHT/i' \
-e 's/RightToLeft/RIGHTLEFT/i' \
-e 's/windowMenu:/windowMenuFile:/i' \
-e 's/tab.width/tabs.width/i' \
-e 's/tabFocusModel/tabs.focusModel/i' \
-e 's/ClickToTabFofucs/CLICKTABFOCUS/i' \
-e 's/SloppyTabFocus/MOUSETABFOCUS/i' \
-e 's/tab.inTitlebar/tabs.inTitlebar/i' \
-e 's/tab.placement/tabs.placement/i' \
-e 's/tabPadding/tabs.textPadding/i' \
-e 's/toolbar.max.*true/toolbar.claimSpace: false/i' \
-e 's/toolbar.max.*false/toolbar.claimSpace: true/i' > $newcfg

```

# KNOWN ISSUES

- CascadePlacement may lose its place if using multiple monitors and monitor positions swap around.
- Some old issues related to actively changing multiple running menus and configs at once may break or even crash the WM (but you kind of have to try).
- Some key binds, like `Mod4 Mouse3 :StartTabbing` without the proper prefix target (`OnWindow` for tabbing), will freeze the system and require a restart (Restart command or SIGUSR1).
- Using a weird multi-monitor setup like an L-shape may have undesired behavior. With specific setups and/or config settings, windows may be placed offscreen, or the Toolbar may go into nothingness. The full screen shape is a square which is not the actual case for a L-shaped setup.
- If you have an odd sized (3,5...15) sized toolbar/strut or available desktop area - rounding errors occur in some commands that do percentage calculations and will be a few pixel off.
- Restarting with tabs and having the last tab selected will not draw that tab's image-icon until an update hits (resize, select new tab).
- Most memory use seems to be font-cache which can, and will, creep up under the 'right' (don't ask me what) circumstances over time. I'd love to know how to "release" it. Leaving my machine on overnight while compiling something intense like Chromium will clear it and in the morning the window manager is only using half the RSS of what it supposedly does normally.
- Firefox (maybe other browsers or tools) use `DESKTOP_SESSION`, `XDG_CURRENT_DESKTOP` or `XDG_CURRENT_SESSION` to pick up `fluxbox` for some mouse hacks. The mouse issue this solved has been fixed. But there may be other similar hacks in Firefox or other programs that are not addressed.

# Building / Installing

### Requirements

`fribidi imlib2 libXext libXft libXpm libXrender libXrandr `


### Build (autotools)

```
./autogen.sh
./configure --prefix=/usr
make
```

### Install

```
sudo make install
```

# Shout Outs

Many thanks to Brad Hughes, Henrik Kinnunen, and the rest of the Blackbox/Fluxbox contributors.

See [AUTHORS](./AUTHORS)
