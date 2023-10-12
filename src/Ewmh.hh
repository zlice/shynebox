// Ewmh.hh for Shynebox Window Manager

/*
  Extended Window Manager Hints ( http://www.freedesktop.org/Standards/wm-spec )
  Calls back to window manager for sizes, placement, icons ...
*/

#ifndef EWMH_HH
#define EWMH_HH

#include <X11/Xlib.h>
#include <string>

class BScreen;
class ShyneboxWindow;
class WinClient;

namespace tk {
class SbWindow;
typedef std::string SbString;
}

class Ewmh {
public:
  Ewmh();
  ~Ewmh();

  void initForScreen(BScreen &screen);
  void setupFrame(ShyneboxWindow &win);
  void setupClient(WinClient &winclient);

  void updateFocusedWindow(BScreen &screen, Window win);
  void updateClientList(BScreen &screen);
  void updateWorkspaceNames(BScreen &screen);
  void updateCurrentWorkspace(BScreen &screen);
  void updateWorkspaceCount(BScreen &screen);
  void updateViewPort(BScreen &screen);
  void updateGeometry(BScreen &screen);
  void updateWorkarea(BScreen &screen);
  void updateState(ShyneboxWindow &win);
  void updateWorkspace(ShyneboxWindow &win);

  bool checkClientMessage(const XClientMessageEvent &ce,
                          BScreen * screen, WinClient * const winclient);

  bool propertyNotify(WinClient &winclient, Atom the_property);
  void updateClientClose(WinClient &winclient);
  void updateFrameExtents(ShyneboxWindow &win);

private:
  enum { STATE_REMOVE = 0, STATE_ADD = 1, STATE_TOGGLE = 2};

  void setState(ShyneboxWindow &win, Atom state, bool value);
  void setState(ShyneboxWindow &win, Atom state, bool value,
                WinClient &client);
  void toggleState(ShyneboxWindow &win, Atom state);
  void toggleState(ShyneboxWindow &win, Atom state, WinClient &client);
  void updateStrut(WinClient &winclient);
  void updateActions(ShyneboxWindow &win);

  void setupState(ShyneboxWindow &win);

  tk::SbWindow *m_dummy_window = 0;
  tk::SbString getUTF8Property(Atom property);

  class EwmhAtoms;
  EwmhAtoms* m_net;
};
#endif // EWMH_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2002 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
