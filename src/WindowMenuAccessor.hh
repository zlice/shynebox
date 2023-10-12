// WindowMenuAccessor.hh for Shynebox Window Manager

/*
  Allows read/write access to bools 'shaded', 'iconic', and 'sticky'
  for current ShyneboxWindow through SbMenu::window() by using
  getters and setters.
  Used in MenuCreator and ConfigMenu.

  (basically BoolMenuItem, but the bool& ref is a specific window item)

  NOTE: Maximize is not compatible with this, as ShyneboxWindow
        has conflicting types and arguments.
*/

#ifndef WINDOW_MENU_ACCESSOR_HH
#define WINDOW_MENU_ACCESSOR_HH

#include "SbMenu.hh"

class WindowMenuAccessor: public tk::MenuItem {
public:
  typedef bool (ShyneboxWindow:: *Getter)() const;
  typedef void (ShyneboxWindow:: *Setter)(bool);

  WindowMenuAccessor(const tk::SbString &label, Getter g, Setter s):
        tk::MenuItem(label), m_getter(g), m_setter(s) {
    tk::MenuItem::setSelected(false);
    setToggleItem(true);
    setCloseOnClick(false);
  }

  bool isSelected() const {
    ShyneboxWindow *sbwin = SbMenu::window();
    if (sbwin)
      return (sbwin->*m_getter)();
    return tk::MenuItem::isSelected();
  }

  void click(int button, int time, unsigned int mods) {
    ShyneboxWindow *sbwin = SbMenu::window();
    if (sbwin)
      setSelected( !(sbwin->*m_getter)() );
    tk::MenuItem::click(button, time, mods);
  }

  void setSelected(bool value) {
    ShyneboxWindow *sbwin = SbMenu::window();
    if (sbwin) {
      (sbwin->*m_setter)(value); // see click()
      tk::MenuItem::setSelected( (sbwin->*m_getter)() );
    } // else, do nothing
  }

private:
  Getter m_getter;
  Setter m_setter;
};

#endif // WINDOW_MENU_ACCESSOR_HH

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
