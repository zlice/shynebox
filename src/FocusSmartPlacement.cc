// FocusSmartPlacement.cc for Shynebox Window Manager

#include "FocusSmartPlacement.hh"

#include "FocusControl.hh"
#include "Window.hh"
#include "Screen.hh"
#include "ScreenPlacement.hh"

using std::swap;

// sorry for the messy hard to read logic
// below is the original column code with proper names

// everything defaults to Row logic first
// x is x, w is w ... checks left/right then top/bot
// xchg values for Col placement to swap logic
// this means Col uses more CPU to have a single class

void FocusSmartPlacement::placeWindow(const ShyneboxWindow &win, int head,
                                    int &place_x, int &place_y) {
  ShyneboxWindow *foc_win = win.screen().focusControl().focusedSbWindow();

  const ScreenPlacement &sps = win.screen().placementStrategy();
  const bool is_col = sps.placementPolicy() == tk::ScreenPlacementPolicy_e::COLSMARTPLACEMENT;

  bool dir_one = sps.rowDirection() == tk::RowDirection_e::LEFTRIGHT,
       dir_two = sps.colDirection() == tk::ColDirection_e::TOPBOTTOM;

  int win_tre = win.width()
              + win.sbWindow().borderWidth()*2 + win.widthOffset(),
    win_for = win.height()
              + win.sbWindow().borderWidth()*2 + win.heightOffset(),
    x_off = win.xOffset(),
    y_off = win.yOffset(),
    head_one_start = (signed) win.screen().maxLeft(head),  // obey head covered
    head_one_end   = (signed) win.screen().maxRight(head), // inside screen
    head_two_start = (signed) win.screen().maxTop(head),
    head_two_end   = (signed) win.screen().maxBottom(head),
    dflt_one = head_one_start,
    dflt_two = head_two_start,
    try_one = dflt_one, try_two = dflt_two, // <--- placement locations
    foc_one = -1, foc_two = -1, foc_tre = -1, foc_for = -1;

  if (foc_win) { // if this is the first window saftey
    foc_one = foc_win->x() - foc_win->xOffset(); // offset means external tabs
    foc_two = foc_win->y() - foc_win->yOffset();
    foc_tre = foc_win->width() + foc_win->sbWindow().borderWidth()*2
            + foc_win->widthOffset();
    foc_for = foc_win->height() + foc_win->sbWindow().borderWidth()*2
            + foc_win->heightOffset();
  }

  if (is_col) {
    swap(dir_one, dir_two); // left/right vs top/bot checks
    swap(win_tre, win_for); // w, h
    swap(head_one_start, head_two_start);
    swap(head_one_end, head_two_end);
    swap(dflt_one, dflt_two); // x, y
    swap(try_one, try_two); // x, y
    swap(foc_one, foc_two); // x, y
    swap(foc_tre, foc_for); // w, h
  }

  // check first direction
  if (dir_one)
    try_one = foc_one + foc_tre;
  else {
    dflt_one = head_one_end - win_tre;
    try_one = foc_one - win_tre;
  }

  if (head_one_start <= try_one && head_one_end >= try_one + win_tre) {
    try_two = foc_two;
  } else { // goes outside screen
    try_one = dflt_one;
    if (dir_two)
      try_two = foc_two + foc_for;
    else {
      dflt_two = head_two_end - win_for;
      try_two = foc_two - win_for;
    }
    if (head_two_start > try_two || head_two_end < try_two + win_for)
      try_two = dflt_two;
  } // if inside y bounds

  if (is_col) // now that the logic is done, swap these back
    swap(try_one, try_two);

  place_x = try_one + x_off;
  place_y = try_two + y_off;
} // placeWindow

/*
  ShyneboxWindow *foc_win = win.screen().focusControl().focusedSbWindow();

  const ScreenPlacement &sps = win.screen().placementStrategy();

  const int win_w = win.width()
                  + win.sbWindow().borderWidth()*2 + win.widthOffset(),
    win_h = win.height()
                  + win.sbWindow().borderWidth()*2 + win.heightOffset(),
    x_off = win.xOffset(),
    y_off = win.yOffset(),
    head_left  = (signed) win.screen().maxLeft(head),  // obey head covered
    head_right = (signed) win.screen().maxRight(head), // inside screen
    head_top   = (signed) win.screen().maxTop(head),
    head_bot   = (signed) win.screen().maxBottom(head),
    change_y = sps.colDirection() == tk::ColDirection_e::BOTTOMTOP
             ? -1 : 1;
  int dflt_x = head_left,
      dflt_y = head_top,
      try_x = dflt_x, try_y = dflt_y,
      foc_x = -1, foc_y = -1, foc_w = -1, foc_h = -1;

  if (foc_win) { // first window saftey
    foc_x = foc_win->x() - foc_win->xOffset(); // offset means external tabs
    foc_y = foc_win->y() - foc_win->yOffset();
    foc_w = foc_win->width() + foc_win->sbWindow().borderWidth()*2
            + foc_win->widthOffset();
    foc_h = foc_win->height() + foc_win->sbWindow().borderWidth()*2
            + foc_win->heightOffset();
  }

  const bool top_bot    = sps.colDirection() == tk::ColDirection_e::TOPBOTTOM,
             left_right = sps.rowDirection() == tk::RowDirection_e::LEFTRIGHT;

  // check col first
  if (top_bot)
    try_y = foc_y + foc_h;
  else {
    dflt_y = head_bot - win_h;
    try_y = foc_y - win_h;
  }

  if (head_top <= try_y && head_bot >= try_y + win_h) {
    try_x = foc_x;
  } else { // goes outside screen
    try_y = dflt_y;
    if (left_right)
      try_x = foc_x + foc_w;
    else {
      dflt_x = head_right - win_w;
      try_x = foc_x - win_w;
    }
    if (head_left > try_x || head_right < try_x + win_w)
      try_x = dflt_x;
  } // if inside y bounds

  place_x = try_x + xoff;
  place_y = try_y + yoff;
} // placeWindow
*/

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
