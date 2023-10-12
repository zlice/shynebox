// ClientMenu.hh for Shynebox Window Manager

/*
  Menu with clients to select from.
  Used for workspaces, icons (minimized) and custom menus.
*/

#ifndef CLIENTMENU_HH
#define CLIENTMENU_HH

#include "SbMenu.hh"

class BScreen;
class ShyneboxWindow;
class Focusable;

/**
 * A menu holding a set of client menus.
 * @see WorkspaceMenu
 */
class ClientMenu: public SbMenu {
public:
  typedef std::list<ShyneboxWindow *> Focusables;

  ClientMenu(BScreen &screen, Focusables &clients);

  // refresh the entire menu
  void updateMenu();

private:
  Focusables &m_list; ///< clients in the menu
};

#endif // CLIENTMENU_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2007-2008 Fluxbox Team (fluxgen at fluxbox dot org)
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
