// SbAtom.cc for Shynebox Window Manager

#include "SbAtoms.hh"
#include "tk/App.hh"

namespace {
SbAtoms* s_singleton = 0;
}

SbAtoms::SbAtoms() {
  Display* dpy = tk::App::instance()->display();

  xa_wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
  xa_wm_state = XInternAtom(dpy, "WM_STATE", False);
  xa_wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", False);
  xa_wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
  xa_wm_take_focus = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
  motif_wm_info = XInternAtom(dpy, "_MOTIF_WM_INFO", False);
  motif_wm_hints = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);

  s_singleton = this;
} // SBAtoms class init

SbAtoms::~SbAtoms() {
  s_singleton = 0;
} // SBAtoms class destroy

SbAtoms *SbAtoms::instance() {
  if (s_singleton == 0)
    s_singleton = new SbAtoms();
  return s_singleton;
}

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
