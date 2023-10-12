// WinButtonTheme.cc for Shynebox Window Manager

#include "WinButtonTheme.hh"

#include "tk/Image.hh"
#include "tk/PixmapWithMask.hh"

#include "SbWinFrameTheme.hh"

WinButtonTheme::WinButtonTheme(int screen_num, const std::string &extra,
                        tk::ThemeProxy<SbWinFrameTheme> &frame_theme):
      tk::Theme(screen_num),
      m_close_pm(*this, "window.close" + extra + ".pixmap"),
      m_maximize_pm(*this, "window.maximize" + extra + ".pixmap"),
      m_iconify_pm(*this, "window.iconify" + extra + ".pixmap"),
      m_shade_pm(*this, "window.shade" + extra + ".pixmap"),
      m_unshade_pm(*this, "window.unshade" + extra + ".pixmap"),
      m_menuicon_pm(*this, "window.menuicon" + extra + ".pixmap"),
      m_title_pm(*this, "window.title" + (extra.empty() ? std::string(".focus")
                                                        : extra) + ".pixmap"),
      m_stick_pm(*this, "window.stick" + extra + ".pixmap"),
      m_stuck_pm(*this, "window.stuck" + extra + ".pixmap"),
      m_lefthalf_pm(*this, "window.lhalf" + extra + ".pixmap"),
      m_righthalf_pm(*this, "window.rhalf" + extra + ".pixmap"),
      m_frame_theme(frame_theme) {
  tk::ThemeManager::instance().loadTheme(*this);
} // WinButtonTheme class init

WinButtonTheme::~WinButtonTheme() { } // WinButtonTheme class destroy

void WinButtonTheme::reconfigTheme() {
  // rescale the pixmaps to match frame theme height
  unsigned int size = m_frame_theme->titleHeight()
                      - 2 * m_frame_theme->bevelWidth();
  if (m_frame_theme->titleHeight() == 0)
    size = m_frame_theme->font().height() + 2;
    // calculate height from font and border width to scale pixmaps
  // else  use specified height to scale pixmaps

  // scale all pixmaps
  m_close_pm->scale(size, size);
  m_maximize_pm->scale(size, size);
  m_menuicon_pm->scale(size, size);
  m_iconify_pm->scale(size, size);
  m_shade_pm->scale(size, size);
  m_unshade_pm->scale(size, size);
  m_title_pm->scale(size, size);
  m_stick_pm->scale(size, size);
  m_stuck_pm->scale(size, size);
  m_lefthalf_pm->scale(size, size);
  m_righthalf_pm->scale(size, size);
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
