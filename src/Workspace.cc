// Workspace.cc for Shynebox Window Manager

#include "Workspace.hh"
#ifndef USE_TOOLBAR
#include "Window.hh"
#endif

#include "Screen.hh"
#include "FocusControl.hh"

#include "Debug.hh"

#ifdef HAVE_CSTDIO
  #include <cstdio>
#else
  #include <stdio.h>
#endif
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::string;

Workspace::Workspace(BScreen &scrn, const string &name, unsigned int id):
     m_screen(scrn),
     m_clientmenu(scrn, m_windowlist),
     m_name(name) {
  m_clientmenu.setInternalMenu();
  setName(name, static_cast<int>(id) );
} // Workspace class init

Workspace::~Workspace() { } // Workspace class destroy

void Workspace::addWindow(ShyneboxWindow &w) {
  // we don't need to add a window that already exist in our list
  for (auto it : m_windowlist)
    if (it == &w)
      return;

  m_windowlist.push_back(&w);
}

// still_alive is true if the window will continue to exist after
// this event. Particularly, this isn't the removeWindow for
// the destruction of the window. Because if so, the focus revert
// is done in another place
int Workspace::removeWindow(ShyneboxWindow *w, bool still_alive) {
  if (w == 0)
    return -1;

  // if w is focused and alive, remove the focus ... except if it
  // is a transient window. removing the focus from such a window
  // leads in a wild race between BScreen::reassociateWindow(),
  // BScreen::changeWorkspaceID(), ShyneboxWindow::focus() etc. which
  // finally leads to crash.
  if (w->isFocused() && !w->isTransient() && still_alive)
    FocusControl::unfocusWindow(w->winClient(), true, true);

  m_windowlist.remove(w);

  return m_windowlist.size();
}

void Workspace::showAll() {
  for (auto it : m_windowlist)
    it->show();
}

void Workspace::hideAll(bool interrupt_moving) {
  for (auto it : m_windowlist)
    if (! it->isStuck() )
      it->hide(interrupt_moving);
}


void Workspace::removeAll(unsigned int dest) {
  Windows tmp_list(m_windowlist);
  for (auto it : tmp_list)
    m_screen.sendToWorkspace(dest, it, false);
}

void Workspace::reconfigure() {
  m_clientmenu.reconfigure();
}

// id set on init() when called from screen, default arg is -1
// prevents crash while being created
void Workspace::setName(const string &name, int id) {
  if (id < 0)
    id = screen().getWorkspaceID(*this);

  if (!name.empty() && name != "") {
    if (name == m_name)
      return;
    m_name = name;
  } else { // set default name to just the id (starting from 1)
    char tname[12];
    snprintf(tname, sizeof(tname), "%d", id + 1);
    m_name = tname;
  }

  m_clientmenu.setLabel(tk::BiDiString(m_name) );
  m_clientmenu.updateMenu();
}

/**
 Calls restore on all windows
 on the workspace and then
 clears the m_windowlist
*/
void Workspace::shutdown() {
  // note: when the window dies it'll remove it self from the list
  while (!m_windowlist.empty() )
    delete m_windowlist.back();
    // delete window (the window removes it self from m_windowlist)
}

void Workspace::updateClientmenu() {
  m_clientmenu.updateMenu();
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2001 - 2008 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Workspace.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes at tcac.net)
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
