// IntMenuItem.hh for Shynebox Window Manager

/*
  A int value menu item. Has a min and max value.
  Used for configuring through the menu.
*/

#ifndef TK_INTMENUITEM_HH
#define TK_INTMENUITEM_HH

#include "MenuItem.hh"
#include "StringUtil.hh"

namespace tk {

class IntMenuItem: public MenuItem {
public:
  IntMenuItem(const SbString &label, int &res,
              int min_val, int max_val, Menu &host_menu) :
    MenuItem(label, host_menu), m_org_label(MenuItem::label() ),
    m_max(max_val), m_min(min_val), m_res(res) {
    updateLabel();
    setCloseOnClick(false);
  }

  void click(int button, int time, unsigned int mods) {
    static int last_time = -201;
    int inc_val = 1;
    // check double click - hardcoded time
    if (time - last_time <= 200)
      inc_val = 5;

    last_time = time;

    // make sure values stay within bounds _before_ we try to set m_res
    // otherwise, this may cause bugs (say, with casting to unsigned char)
    if ((button == 4 || button == 3) && m_res < m_max) { // up
      if (m_res + inc_val < m_max)
        m_res = m_res + inc_val;
      else
        m_res = m_max;
    } else if ((button == 5 || button == 1) && m_res > m_min) { // down
      if (m_res - inc_val >= m_min)
        m_res = m_res - inc_val;
      else
        m_res = m_min;
    }

    // update label
    updateLabel();
    // call other commands
    MenuItem::click(button, time, mods);

    // show new value, which for us means forcing a full menu update
    // since the text is drawn onto the background!
    if (menu() ) {
      menu()->frameWindow().updateBackground();
      menu()->clearWindow();
    }
  }

  void updateLabel() {
    setLabel(m_org_label.logical() + ":  " + StringUtil::number2String(m_res) );
  }

private:
  BiDiString m_org_label; // original label
  const int m_max; // maximum value the integer can have
  const int m_min; // minimum value the integer can have
  int &m_res;      // resource item to be changed
};

} // end namespace tk

#endif // TK_INTMENUITEM_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003-2007 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
