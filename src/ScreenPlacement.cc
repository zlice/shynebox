// ScreenPlacement.cc for Shynebox Window Manager

#include "ScreenPlacement.hh"

#include "FocusSmartPlacement.hh"
#include "MinOverlapPlacement.hh"
#include "UnderMousePlacement.hh"
#include "CenterPlacement.hh"
#include "CascadePlacement.hh"

#include "Screen.hh"
#include "Window.hh"

#include "tk/Menu.hh"
#include "tk/Config.hh"

#include <exception>
#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

#define SPP tk::ScreenPlacementPolicy_e
#define ROWDIR tk::RowDirection_e
#define COLDIR tk::ColDirection_e

ScreenPlacement::ScreenPlacement(BScreen &screen):
    m_row_direction((ROWDIR&)((int&)(*screen.m_cfgmap["rowPlacementDirection"]) ) ),
    m_col_direction((COLDIR&)((int&)(*screen.m_cfgmap["colPlacementDirection"]) ) ),
    m_placement_policy((SPP&)((int&)(*screen.m_cfgmap["windowPlacement"]) ) ),
    m_old_policy(SPP::ROWSMARTPLACEMENT),
    m_screen(screen)
{ } // ScreenPlacement class init

void ScreenPlacement::placeWindow(const ShyneboxWindow &win, int head,
                                  int &place_x, int &place_y) {
  // check the resource placement and see if has changed
  // and if so update the strategy
  if (m_old_policy != m_placement_policy || !m_strategy) {
    m_old_policy = (SPP)m_placement_policy;
    if (m_strategy)
      delete m_strategy;
    switch ((SPP)m_placement_policy) {
    case SPP::ROWSMARTPLACEMENT:
    case SPP::COLSMARTPLACEMENT:
        m_strategy = new FocusSmartPlacement();
        break;
    case SPP::ROWMINOVERLAPPLACEMENT:
    case SPP::COLMINOVERLAPPLACEMENT:
        m_strategy = new MinOverlapPlacement();
        break;
    case SPP::UNDERMOUSEPLACEMENT:
        m_strategy = new UnderMousePlacement();
        break;
    case SPP::CENTERPLACEMENT:
        m_strategy = new CenterPlacement();
        break;
    case SPP::AUTOTABPLACEMENT:
        m_strategy = 0;
        break;
    case SPP::CASCADEPLACEMENT:
    default:
        m_strategy = new CascadePlacement(win.screen() );
        break;
    }
  } // if old policy == new policy

  int head_left  = (signed) win.screen().maxLeft(head),
      head_right = (signed) win.screen().maxRight(head),
      head_top   = (signed) win.screen().maxTop(head),
      head_bot   = (signed) win.screen().maxBottom(head);

  // start placement, top left corner
  place_x = head_left;
  place_y = head_top;

  m_strategy->placeWindow(win, head, place_x, place_y);

  int win_w = win.normalWidth() + win.sbWindow().borderWidth()*2 + win.widthOffset(),
      win_h = win.normalHeight() + win.sbWindow().borderWidth()*2 + win.heightOffset();

  // make sure the window is inside our screen(head) area
  if (place_x + win_w - win.xOffset() > head_right)
    place_x = head_left + (head_right - head_left - win_w) / 2 + win.xOffset();
  if (place_y + win_h - win.yOffset() > head_bot)
    place_y = head_top + (head_bot - head_top - win_h) / 2 + win.yOffset();
} // placeWindow

void ScreenPlacement::placeAndShowMenu(tk::Menu& menu,
                      int x, int y) {
  int head = m_screen.getHead(x, y);
  menu.setScreen(m_screen.getHeadX(head),
                 m_screen.getHeadY(head),
                 m_screen.getHeadWidth(head),
                 m_screen.getHeadHeight(head) );

  menu.updateMenu(); // recalculate the size

  x = x - (menu.width() / 2);
  if (menu.isTitleVisible() )
    y = y - (menu.titleWindow().height() / 2);

  int bw = 2 * menu.sbwindow().borderWidth();
  m_screen.fitToHead(head, x, y, menu.width(),
                       menu.height(), bw, true);

  menu.move(x, y);
  menu.show();
  menu.grabInputFocus();
} // placeAndShowMenu

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
