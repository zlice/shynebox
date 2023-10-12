// FocusControl.hh for Shynebox Window Manager

/*
  Handles window focus for a specific screen.
  Holds focused, last focused, reverts and ignores focus for special cases.
*/

#ifndef FOCUSCONTROL_HH
#define FOCUSCONTROL_HH

#include <list>

#include "tk/Config.hh"
#include "FocusableList.hh"

class ClientPattern;
class WinClient;
class ShyneboxWindow;
class Focusable;
class BScreen;

class FocusControl {
public:
  typedef std::list<Focusable *> Focusables;

  explicit FocusControl(BScreen &screen);
  void prevFocus() { cycleFocus(m_focused_list, 0, true); }
  void nextFocus() { cycleFocus(m_focused_list, 0, false); }
  // Cycle focus for a set of windows.
  void cycleFocus(const FocusableList &winlist, const ClientPattern *pat = 0,
                  bool reverse = false);

  void goToWindowNumber(const FocusableList &winlist, int num,
                        const ClientPattern *pat = 0);
  void setScreenFocusedWindow(WinClient &win_client);
  void setFocusModel(tk::MainFocusModel_e model);
  void setTabFocusModel(tk::TabFocusModel_e model);
  void stopCyclingFocus();

  // Set the "ignore" pointer location to the current pointer location
  void ignoreAtPointer(bool force = false);
  // Set the "ignore" pointer location to the given coordinates
  void ignoreAt(int x, int y, bool force = false);
  // unset the "ignore" pointer location
  void ignoreCancel();
  // true if events at the given X/Y coordinate should be ignored
  // (ie, they were previously cached via one of the ignoreAt calls)
  bool isIgnored(int x, int y);

  bool isCycling() const { return m_cycling_list != 0; }
  // Appends a client to the front of the focus list
  void addFocusBack(WinClient &client);
  // Appends a client to the front of the focus list
  void addFocusFront(WinClient &client);
  void addFocusWinBack(Focusable &win);
  void addFocusWinFront(Focusable &win);
  void setFocusBack(ShyneboxWindow &sbwin);

  // config items
  bool isMouseFocus() const { return focusModel() != tk::MainFocusModel_e::CLICKFOCUS; }
  bool isMouseTabFocus() const { return tabFocusModel() == tk::TabFocusModel_e::MOUSETABFOCUS; }
  tk::MainFocusModel_e focusModel() const { return m_focus_model; }
  tk::TabFocusModel_e tabFocusModel() const { return m_tab_focus_model; }
  bool focusNew() const { return m_focus_new; }
  bool focusSameHead() const { return m_focus_same_head; }

  // last focused client in a specific workspace, or NULL.
  Focusable *lastFocusedWindow(int workspace);

  WinClient *lastFocusedWindow(ShyneboxWindow &group, WinClient *ignore_client = 0);

  // focus list in creation order
  const FocusableList &creationOrderList() const { return m_creation_order_list; }
  // the focus list in focused order
  const FocusableList &focusedOrderList() const { return m_focused_list; }
  const FocusableList &creationOrderWinList() const { return m_creation_order_win_list; }
  const FocusableList &focusedOrderWinList() const { return m_focused_win_list; }

  // remove client from focus list
  void removeClient(WinClient &client);
  // remove window from focus list
  void removeWindow(Focusable &win);
  // starts terminating this control
  void shutdown();

  // do fallback focus for screen if normal focus control failed.
  static void revertFocus(BScreen &screen);
  // like revertFocus, but specifically related to this window (transients etc)
  static void unfocusWindow(WinClient &client, bool full_revert = true, bool unfocus_frame = false);
  static void setFocusedWindow(WinClient *focus_to);
  static void setFocusedSbWindow(ShyneboxWindow *focus_to) { s_focused_sbwindow = focus_to; }
  static void setExpectingFocus(WinClient *client) { s_expecting_focus = client; }
  static WinClient *focusedWindow() { return s_focused_window; }
  static ShyneboxWindow *focusedSbWindow() { return s_focused_sbwindow; }
  static WinClient *expectingFocus() { return s_expecting_focus; }

private:
  BScreen &m_screen;

  // config items
  tk::MainFocusModel_e &m_focus_model;
  tk::TabFocusModel_e  &m_tab_focus_model;
  bool &m_focus_new, &m_focus_same_head;

  // This list keeps the order of window focusing for this screen
  // Screen global so it works for sticky windows too.
  FocusableList m_focused_list,
                m_creation_order_list,
                m_focused_win_list,
                m_creation_order_win_list;

  Focusables::const_iterator m_cycling_window;
  const FocusableList *m_cycling_list;
  Focusable *m_was_iconic;
  WinClient *m_cycling_last;
  Focusable *m_cycling_next;
  int m_ignore_mouse_x, m_ignore_mouse_y;

  static WinClient *s_focused_window;
  static ShyneboxWindow *s_focused_sbwindow;
  static WinClient *s_expecting_focus;
  static bool s_reverting;
};

#endif // FOCUSCONTROL_HH

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
