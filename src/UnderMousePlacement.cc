// UnderMousePlacement.cc for Shynebox Window Manager

#include "UnderMousePlacement.hh"

#include "tk/App.hh"
#include "tk/KeyUtil.hh"

#include "Screen.hh"
#include "Window.hh"

void UnderMousePlacement::placeWindow(const ShyneboxWindow &win, int head,
                                      int &place_x, int &place_y) {
  int root_x, root_y;

  tk::KeyUtil::get_pointer_coords(
    tk::App::instance()->display(),
    win.screen().rootWindow().window(), root_x, root_y);

  // not using offset ones because we won't let tabs influence the "centre"
  int win_w = win.width() + win.sbWindow().borderWidth()*2,
      win_h = win.height() + win.sbWindow().borderWidth()*2;

  int test_x = root_x - (win_w / 2);
  int test_y = root_y - (win_h / 2);

  win.screen().fitToHead(head, test_x, test_y, win_w, win_h, 0, true);

  place_x = test_x;
  place_y = test_y;
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
