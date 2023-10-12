// CascadePlacement.cc for Shynebox Window Manager

#include "CascadePlacement.hh"

#include "Window.hh"
#include "Screen.hh"
#include "Debug.hh"

CascadePlacement::CascadePlacement(const BScreen &screen) {
  m_cascade_x = new int[screen.numHeads() + 1];
  m_cascade_y = new int[screen.numHeads() + 1];
  for (int i=1; i < screen.numHeads() + 1; i++) {
    m_cascade_x[i] = -1;
    m_cascade_y[i] = -1;
  }
} // CascadePlacement class init

CascadePlacement::~CascadePlacement() {
  delete [] m_cascade_x;
  delete [] m_cascade_y;
} // CascadePlacement class destroy

// TODO : BAD LOGIC
//   multihead needs improvement here
//   switching monitor positions from right to bot fuks up entire positioning
void CascadePlacement::placeWindow(const ShyneboxWindow &win, int head,
                                   int &place_x, int &place_y) {
  int head_left  = (signed) win.screen().maxLeft(head),
      head_right = (signed) win.screen().maxRight(head),
      head_top   = (signed) win.screen().maxTop(head),
      head_bot   = (signed) win.screen().maxBottom(head);

  if (m_cascade_x[head] + (signed)win.width() > head_right
      || m_cascade_x[head] < head_left)
    m_cascade_x[head] = head_left;
  if (m_cascade_y[head] + (signed)win.height() > head_bot
      || m_cascade_y[head] < head_top)
    m_cascade_y[head] = head_top;

  place_x = m_cascade_x[head];
  place_y = m_cascade_y[head];

  // just one borderwidth, so they can share a borderwidth (looks better)
  int titlebar_height =
    win.titlebarHeight() + win.sbWindow().borderWidth();
  if (titlebar_height < 4) // make sure it is not insignificant
    titlebar_height = 32;

  m_cascade_x[head] += titlebar_height;
  m_cascade_y[head] += titlebar_height;
}

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
