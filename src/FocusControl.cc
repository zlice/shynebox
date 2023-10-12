// FocusControl.cc for Shynebox Window Manager

#include "FocusControl.hh"

#include "ClientPattern.hh"
#include "Screen.hh"
#include "Window.hh"
#include "WinClient.hh"
#include "Workspace.hh"
#include "shynebox.hh"
#include "SbWinFrameTheme.hh"
#include "Debug.hh"

#include "tk/EventManager.hh"
#include "tk/KeyUtil.hh"

#include <unistd.h>

#define MainFocEnum tk::MainFocusModel_e
#define TabFocEnum tk::TabFocusModel_e

WinClient *FocusControl::s_focused_window = 0;
ShyneboxWindow *FocusControl::s_focused_sbwindow = 0;
WinClient *FocusControl::s_expecting_focus = 0;
bool FocusControl::s_reverting = false;

namespace {

bool doSkipWindow(const Focusable &win, const ClientPattern *pat) {
  const ShyneboxWindow *sbwin = win.sbwindow();
  if (!sbwin || sbwin->isFocusHidden() || sbwin->isModal() )
    return true; // skip if no sbwindow or if focushidden
  if (pat && !pat->match(win) )
    return true; // skip if it doesn't match the pattern

  return false; // else don't skip
}

} // end anonymous namespace

FocusControl::FocusControl(BScreen &screen):
    m_screen(screen),
    m_focus_model((MainFocEnum&)(int&)(*screen.m_cfgmap["focusModel"]) ),
    m_tab_focus_model((TabFocEnum&)(int&)(*screen.m_cfgmap["tabs.focusModel"]) ),
    m_focus_new(*screen.m_cfgmap["focusNewWindows"]),
    m_focus_same_head(*screen.m_cfgmap["focusSameHead"]),
    m_focused_list(screen),     m_creation_order_list(screen),
    m_focused_win_list(screen), m_creation_order_win_list(screen),
    m_cycling_list(0),
    m_was_iconic(0),
    m_cycling_last(0),
    m_cycling_next(0),
    m_ignore_mouse_x(-1), m_ignore_mouse_y(-1) {
  m_cycling_window = m_focused_list.clientList().end();
} // FocusControl class init

void FocusControl::cycleFocus(const FocusableList &window_list,
                              const ClientPattern *pat, bool cycle_reverse) {
    if (!m_cycling_list) {
      if (m_screen.isCycling() )
        m_cycling_list = &window_list;
        // only set this when we're waiting for modifiers
      m_was_iconic = 0;
      m_cycling_last = 0;
      m_cycling_next = 0;
    } else if (m_cycling_list != &window_list)
      m_cycling_list = &window_list;

    Focusables::const_iterator it_begin = window_list.clientList().begin();
    Focusables::const_iterator it_end = window_list.clientList().end();

    // too many things can go wrong with remembering this
    m_cycling_window = it_end;
    if (m_cycling_next)
      m_cycling_window = find(it_begin, it_end, m_cycling_next);
    if (m_cycling_window == it_end)
      m_cycling_window = find(it_begin, it_end, s_focused_window);
    if (m_cycling_window == it_end)
      m_cycling_window = find(it_begin, it_end, s_focused_sbwindow);

    Focusables::const_iterator it = m_cycling_window;
    ShyneboxWindow *sbwin = 0;
    WinClient *last_client = 0;
    WinClient *was_iconic = 0;

    // find the next window in the list that works
    while (true) {
      if (cycle_reverse && it == it_begin)
        it = it_end;
      else if (!cycle_reverse && it == it_end)
        it = it_begin;
      else
        cycle_reverse ? --it : ++it;
      // give up [do nothing] if we reach the current focused again
      if (it == m_cycling_window)
        return;
      if (it == it_end)
        continue;

      sbwin = (*it)->sbwindow();
      if (!sbwin)
        continue;

      // keep track of the originally selected window in a group
      last_client = &sbwin->winClient();
      was_iconic = (sbwin->isIconic() ? last_client : 0);

      // now we actually try to focus the window
      if (!doSkipWindow(**it, pat) && (m_cycling_next = *it) && (*it)->focus() )
        break;
      m_cycling_next = 0;
    } // while true
    m_cycling_window = it;

    // if we're still in the same sbwin, there's nothing else to do
    if (m_cycling_last && m_cycling_last->sbwindow() == sbwin)
      return;

    // if we were already cycling, then restore the old state
    if (m_cycling_last) {
      // set back to originally selected window in that group
      m_cycling_last->sbwindow()->setCurrentClient(*m_cycling_last, false);

      if (m_was_iconic == m_cycling_last) {
        s_reverting = true; // little hack
        m_cycling_last->sbwindow()->iconify();
        s_reverting = false;
      }
    }

    if (!isCycling() )
      sbwin->raise();

    m_cycling_last = last_client;
    m_was_iconic = was_iconic;
} // cycleFocus

void FocusControl::goToWindowNumber(const FocusableList &winlist, int num,
                                    const ClientPattern *pat) {
  Focusables list = winlist.clientList();
  if (num < 0) {
    list.reverse();
    num = -num;
  }
  Focusable *win = 0;
  Focusables::const_iterator it = list.begin(), it_end = list.end();
  for (; num && it != it_end; ++it) {
    if (!doSkipWindow(**it, pat) && (*it)->acceptsFocus() ) {
      --num;
      win = *it;
    }
  }
  if (win) {
    win->focus();
    if (win->sbwindow() )
      win->sbwindow()->raise();
  }
} // goToWindowNumber

void FocusControl::addFocusBack(WinClient &client) {
  m_focused_list.pushBack(client);
  m_creation_order_list.pushBack(client);
}

void FocusControl::addFocusFront(WinClient &client) {
  m_focused_list.pushFront(client);
  m_creation_order_list.pushBack(client);
}

void FocusControl::addFocusWinBack(Focusable &win) {
  m_focused_win_list.pushBack(win);
  m_creation_order_win_list.pushBack(win);
}

void FocusControl::addFocusWinFront(Focusable &win) {
  m_focused_win_list.pushFront(win);
  m_creation_order_win_list.pushBack(win);
}

// move all clients in given window to back of focused list
// NOTE: ShyneboxWindow loops calls for its transients/clients
void FocusControl::setFocusBack(ShyneboxWindow &sbwin) {
  // do nothing if there are no windows open
  // don't change focus order while cycling
  if (m_focused_list.empty() || s_reverting)
    return;

  m_focused_win_list.moveToBack(sbwin);
  m_focused_list.moveToBack(sbwin);
}

void FocusControl::stopCyclingFocus() {
  // nothing to do
  if (m_cycling_list == 0)
    return;

  m_cycling_last = 0;
  m_cycling_next = 0;
  m_cycling_list = 0;

  // put currently focused window to top
  if (s_focused_window) {
    setScreenFocusedWindow(*s_focused_window);
    if (s_focused_sbwindow)
      s_focused_sbwindow->raise();
  } else
    revertFocus(m_screen);
}

/**
 * Used to find out which window was last focused on the given workspace
 * If workspace is outside the ID range, then the absolute last focused window
 * is given.
 */
Focusable *FocusControl::lastFocusedWindow(int workspace) {
  if (m_screen.isShuttingdown() ) return 0;

  if (workspace < 0 || workspace >= (int) m_screen.numberOfWorkspaces() )
    return m_focused_list.clientList().front();

  const int cur_head =
      m_screen.doObeyHeads() && m_focus_same_head
      && s_focused_sbwindow && !s_focused_sbwindow->isMoving() ?
    m_screen.getCurHead()
  :
    -1;

  // return first client that
  //   accepts focus && is valid client
  //   && on same head
  //   && isn't icon/minimized && on ws (or sticky)
  for (auto it : m_focused_list.clientList() ) {
    if (it->sbwindow() && it->acceptsFocus()
        && it->sbwindow()->winClient().validateClient()
        && ((cur_head == -1) || (it->sbwindow()->getOnHead() == cur_head) )
        && (!it->sbwindow()->isIconic() )
            && (((int)it->sbwindow()->workspaceNumber() ) == workspace
                || it->sbwindow()->isStuck() ) )
      return it;
  }
  return 0;
}

/**
 * Used to find out which window was last active in the given group
 * If ignore_client is given, it excludes that client.
 * Stuck, iconic etc don't matter within a group
 */
WinClient *FocusControl::lastFocusedWindow(ShyneboxWindow &group, WinClient *ignore_client) {
  if (m_focused_list.empty() || m_screen.isShuttingdown() )
    return 0;

  for (auto it : m_focused_list.clientList() )
    if ((it->sbwindow() == &group) && it != ignore_client)
      return dynamic_cast<WinClient *>(it);

  return 0;
}

// raise newly focused window to the top of the focused list
// don't change the order if we're cycling or shutting down
void FocusControl::setScreenFocusedWindow(WinClient &win_client) {
  if (!isCycling() && !m_screen.isShuttingdown() && !s_reverting) {
    m_focused_list.moveToFront(win_client);
    if (win_client.sbwindow() )
      m_focused_win_list.moveToFront(*win_client.sbwindow() );
  }
}

void FocusControl::setFocusModel(MainFocEnum model) {
  m_focus_model = model;
}

void FocusControl::setTabFocusModel(TabFocEnum model) {
  m_tab_focus_model = model;
}

void FocusControl::ignoreAtPointer(bool force) {
  int ignore_x, ignore_y;

  tk::KeyUtil::get_pointer_coords(
    m_screen.rootWindow().display(),
    m_screen.rootWindow().window(), ignore_x, ignore_y);

  this->ignoreAt(ignore_x, ignore_y, force);
}

void FocusControl::ignoreAt(int x, int y, bool force) {
	if (force || m_focus_model == MainFocEnum::MOUSEFOCUS) {
		m_ignore_mouse_x = x;
    m_ignore_mouse_y = y;
  }
}

void FocusControl::ignoreCancel() {
	m_ignore_mouse_x = m_ignore_mouse_y = -1;
}

bool FocusControl::isIgnored(int x, int y) {
  return x == m_ignore_mouse_x && y == m_ignore_mouse_y;
}

void FocusControl::removeClient(WinClient &client) {
  if (client.screen().isShuttingdown() )
    return;

  if (isCycling()
      && m_cycling_window != m_cycling_list->clientList().end()
      && *m_cycling_window == &client) {
    m_cycling_window = m_cycling_list->clientList().end();
    stopCyclingFocus();
  } else if (m_cycling_last == &client)
    m_cycling_last = 0;
  else if (m_cycling_next == &client)
    m_cycling_next = 0;

  m_focused_list.remove(client);
  m_creation_order_list.remove(client);
  Shynebox::instance()->clientListChanged(client.screen() );
}

void FocusControl::removeWindow(Focusable &win) {
  if (win.screen().isShuttingdown() )
    return;

  if (isCycling()
      && m_cycling_window != m_cycling_list->clientList().end()
      && *m_cycling_window == &win) {
    m_cycling_window = m_cycling_list->clientList().end();
    stopCyclingFocus();
  }

  m_focused_win_list.remove(win);
  m_creation_order_win_list.remove(win);
  Shynebox::instance()->clientListChanged(win.screen() );
}

void FocusControl::shutdown() {
  // restore windows backwards so they get put back correctly on restart
  Focusables::reverse_iterator it = m_focused_list.clientList().rbegin();
  for (; it != m_focused_list.clientList().rend(); ++it) {
    WinClient *client = dynamic_cast<WinClient *>(*it);
    if (client && client->sbwindow() )
      client->sbwindow()->restore(client, true);
  }
}

/**
 * This function is called whenever we aren't quite sure what
 * focus is meant to be, it'll make things right ;-)
 */
// STATIC
void FocusControl::revertFocus(BScreen &screen) {
  if (s_reverting || screen.isShuttingdown() )
    return;

  Focusable *next_focus =
      screen.focusControl().lastFocusedWindow(screen.currentWorkspaceID() );

  if (next_focus && next_focus->sbwindow()
      && next_focus->sbwindow()->isStuck() )
    FocusControl::s_reverting = true;

  // if setting focus fails, or isn't possible, fallback correctly
  if (!(next_focus && next_focus->focus() ) ) {
    setFocusedWindow(0); // so we don't get dangling m_focused_window pointer
    // if there's a menu open, focus it
    if (tk::Menu::shownMenu() )
      tk::Menu::shownMenu()->grabInputFocus();
    else {
      switch (screen.focusControl().focusModel() ) { // static
      case MainFocEnum::MOUSEFOCUS:
      case MainFocEnum::STRICTMOUSEFOCUS:
        XSetInputFocus(screen.rootWindow().display(),
                       PointerRoot, None, CurrentTime);
        break;
      case MainFocEnum::CLICKFOCUS:
        screen.rootWindow().setInputFocus(RevertToPointerRoot,
                                          CurrentTime);
        break;
      }
    }
  } // if ! next_focus
  FocusControl::s_reverting = false;
} // revertFocus

/*
 * Like revertFocus, but specifically related to this window (transients etc)
 * if full_revert, we fallback to a full revertFocus if we can't find anything
 * local to the client.
 * If unfocus_frame is true, we won't focus anything in the same frame
 * as the client.
 *
 * So, we first prefer to choose the last client in this window, and if no luck
 * (or unfocus_frame), then we just use the normal revertFocus on the screen.
 *
 * assumption: client has focus
 */
void FocusControl::unfocusWindow(WinClient &client,
                                 bool full_revert,
                                 bool unfocus_frame) {
  // go up the transient tree looking for a focusable window
  ShyneboxWindow *sbwin = client.sbwindow();
  if (sbwin == 0)
    return; // nothing more we can do

  BScreen &screen = sbwin->screen();

  if (client.isTransient() && client.transientFor()->focus() )
    return;

  if (!unfocus_frame) {
    WinClient *last_focus = screen.focusControl().lastFocusedWindow(*sbwin, &client);
    if (last_focus && last_focus->focus() )
      return;
  }

  if (full_revert && s_focused_window == &client)
    revertFocus(screen);
} // unfocusWindow

void FocusControl::setFocusedWindow(WinClient *client) {
  if (client == s_focused_window
      && (!client || client->sbwindow() == s_focused_sbwindow) )
    return;

  BScreen *screen = client ? &client->screen() : 0;
  if (client && screen && screen->focusControl().isCycling() ) {
    Focusable *next = screen->focusControl().m_cycling_next;
    WinClient *nextClient = dynamic_cast<WinClient*>(next);
    ShyneboxWindow *nextWindow = nextClient ? 0 : dynamic_cast<ShyneboxWindow*>(next);
    // if we're currently cycling and the client tries to juggle around focus
    // on FocusIn events to provide client-side modality - don't let him
    if (next && nextClient != client
        && nextWindow != client->sbwindow()
        && screen->focusControl().m_cycling_list->contains(*next) ) {
      next->focus();
      if (nextClient)
        setFocusedWindow(nextClient); // doesn't happen automatically while cycling, 1148
      return;
    }
  } // if cycling

  // if cycling and focusProtection is lock/deny
  if (client && client != expectingFocus() && s_focused_window
      && (!(screen && screen->focusControl().isCycling() ) )
      && ((s_focused_sbwindow->focusProtection() & Focus::Lock)
        || (client->sbwindow()
          && (client->sbwindow()->focusProtection() & Focus::Deny) ) ) ) {
    s_focused_window->focus();
    return;
  }

  BScreen *old_screen =
      FocusControl::focusedWindow() ?
      &FocusControl::focusedWindow()->screen() : 0;

  sbdbg<<"------------------\n";
  sbdbg<<"Setting Focused window = "<<client<<"\n";
  if (client != 0)
    sbdbg<<"title: "<<client->title().logical()<<"\n";
  sbdbg<<"Current Focused window = "<<s_focused_window<<"\n";
  if (s_focused_sbwindow)
    sbdbg<<"         x window id " << s_focused_sbwindow->clientWindow() << "\n";
  sbdbg<<"------------------\n";

  // Update the old focused client to non focus
  if (s_focused_sbwindow
      && (!client || client->sbwindow() != s_focused_sbwindow) )
    s_focused_sbwindow->setFocusFlag(false);

  // Set new focused client/window
  if (client && client->sbwindow() && !client->sbwindow()->isIconic() ) {
    s_focused_sbwindow = client->sbwindow();
    s_focused_window = client;     // update focused window
    s_expecting_focus = 0;
    s_focused_sbwindow->setCurrentClient(*client,
                          false); // don't set inputfocus
    s_focused_sbwindow->setFocusFlag(true); // set focus flag
  } else { // bg/root, menu clicks or oopsie
    s_focused_window = 0;
    s_focused_sbwindow = 0;
    s_expecting_focus = 0; // HACK: (donno the root cause)
    // ^^^ when dragging opaque FOCUSED windows to another ws
    // then clicking menu on/off
    // 'expecting' is still the old client. this breaks Shynebox::revertFocus
    // and never ends up calling FocusController::revertFocus
  }

  if (screen)
    Shynebox::instance()->focusedWindowChanged(*screen, s_focused_window);
  if (old_screen && screen != old_screen)
    Shynebox::instance()->focusedWindowChanged(*old_screen, s_focused_window);
} // setFocusedWindow

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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
