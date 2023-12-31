// RadioMenuItem.hh for Shynebox Window Manager

/*
  MenuItem with multiple selections but only 1 selectable at a time.
*/

#ifndef TK_RADIOMENUITEM_HH
#define TK_RADIOMENUITEM_HH

#include "MenuItem.hh"

namespace tk {

class RadioMenuItem: public MenuItem {
public:
  RadioMenuItem(): MenuItem() { setToggleItem(true); }

  explicit RadioMenuItem(const SbString &label):
        MenuItem(label) {
    setToggleItem(true);
  }

  RadioMenuItem(const SbString &label, Menu &host_menu):
        MenuItem(label, host_menu) {
    setToggleItem(true);
  }

  // create a menu item with a specific command to be executed on click
  RadioMenuItem(const SbString &label, Command<void> &cmd,
                Menu *menu = 0):
        MenuItem(label, cmd, menu) {
    setToggleItem(true);
  }

  RadioMenuItem(const SbString &label, Menu *submenu, Menu *host_menu = 0):
        MenuItem(label, submenu, host_menu) {
    setToggleItem(true);
  }

  virtual ~RadioMenuItem() { }

  virtual bool isSelected() const = 0;
  bool isEnabled() const { return !isSelected(); } // MenuItem's isSelected()
};

} // end namespace tk

#endif // TK_RADIOMENUITEM_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2008 Fluxbox Team (fluxgen at fluxbox dot org)
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
