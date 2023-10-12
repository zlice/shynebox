// Workspace.hh for Shynebox Window Manager

/*
  A list of ShyneboxWindows that represent a virtual desktop.

  Creates a ClientMenu for these windows and has a name.

  NOTE: WorkspaceMenu uses this ClientMenu but does not own it.
        So when handling creation/deletion the 'internalMenu'
        variable is set on Workspace creating the ClientMenu.
*/

#ifndef WORKSPACE_HH
#define WORKSPACE_HH

#include "ClientMenu.hh" // SbMenu > tk/Menu > SbString

#include "tk/NotCopyable.hh"

#include <list>

class BScreen;
class ShyneboxWindow;

class Workspace: private tk::NotCopyable {
public:
  typedef std::list<ShyneboxWindow *> Windows;

  Workspace(BScreen &screen, const std::string &name,
            unsigned int workspaceid = 0);
  ~Workspace();

  // Set workspace name
  void setName(const tk::SbString& name, int id = -1);
  // Deiconify all windows on this workspace
  void showAll();
  void hideAll(bool interrupt_moving);
  // Iconify all windows on this workspace
  void removeAll(unsigned int dest);
  void reconfigure();
  void shutdown();

  void addWindow(ShyneboxWindow &win);
  int removeWindow(ShyneboxWindow *win, bool still_alive);
  void updateClientmenu();

  BScreen &screen() { return m_screen; }
  const BScreen &screen() const { return m_screen; }

  tk::Menu &menu() { return m_clientmenu; }
  const tk::Menu &menu() const { return m_clientmenu; }

  const tk::SbString &name() const { return m_name; }

  const Windows &windowList() const { return m_windowlist; }
  Windows &windowList() { return m_windowlist; }

private:
  void placeWindow(ShyneboxWindow &win);

  BScreen &m_screen;

  Windows m_windowlist; // list of windows by order added to ws
  ClientMenu m_clientmenu;

  tk::SbString m_name; // name of this workspace
};

#endif // WORKSPACE_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2002 - 2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Workspace.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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
