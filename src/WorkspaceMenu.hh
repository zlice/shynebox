// WorkspaceMenu.hh for Shynebox Window Manager

/*
  Menu for Workspaces and Icons (minimized windows).
  Can expand and shrink current Workspaces per configed
  name list, as well as edit a Workspace name.

  Uses ClientMenu from Workspace directly.
  See Workspace comment, basically do NOT try to delete it here.

  It does create a ClientMenu from Screen's IconList(),
  however this is deleted upon menu destruction.
*/

#ifndef WORKSPACEMENU_HH
#define WORKSPACEMENU_HH

#include "SbMenu.hh"
#include "ClientMenu.hh"

class BScreen;

class WorkspaceMenu: public SbMenu {
public:
  explicit WorkspaceMenu(BScreen &screen);

  void workspaceInfoChanged(BScreen& screen);
  void workspaceChanged(BScreen& screen);
  void updateIconMenu() { icon_menu->updateMenu(); }

private:
  ClientMenu *icon_menu = 0; // gets deleted by menu destroys
};

#endif //  WORKSPACEMENU_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2004 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
