// WinButtonTheme.hh for Shynebox Window Manager

/*
  Theme of Caption buttons. (Namely pixmaps)
*/

#ifndef WINBUTTONTHEME_HH
#define WINBUTTONTHEME_HH

#include "tk/Theme.hh" // string
#include "tk/PixmapWithMask.hh"

class SbWinFrameTheme;

class WinButtonTheme: public tk::Theme,
                      public tk::ThemeProxy<WinButtonTheme> {
public:
  WinButtonTheme(int screen_num,
                 const std::string &extra,
                 tk::ThemeProxy<SbWinFrameTheme> &frame_theme);
  ~WinButtonTheme();

  void reconfigTheme();

  const tk::PixmapWithMask &closePixmap() const { return *m_close_pm; }
  tk::PixmapWithMask &closePixmap() { return *m_close_pm; }

  const tk::PixmapWithMask &maximizePixmap() const { return *m_maximize_pm; }
  tk::PixmapWithMask &maximizePixmap() { return *m_maximize_pm; }

  const tk::PixmapWithMask &iconifyPixmap() const { return *m_iconify_pm; }
  tk::PixmapWithMask &iconifyPixmap() { return *m_iconify_pm; }

  const tk::PixmapWithMask &stickPixmap() const { return *m_stick_pm; }
  tk::PixmapWithMask &stickPixmap() { return *m_stick_pm; }

  const tk::PixmapWithMask &stuckPixmap() const { return *m_stuck_pm; }
  tk::PixmapWithMask &stuckPixmap() { return *m_stuck_pm; }

  const tk::PixmapWithMask &shadePixmap() const { return *m_shade_pm; }
  tk::PixmapWithMask &shadePixmap() { return *m_shade_pm; }

  const tk::PixmapWithMask &unshadePixmap() const { return *m_unshade_pm; }
  tk::PixmapWithMask &unshadePixmap() { return *m_unshade_pm; }

  const tk::PixmapWithMask &menuiconPixmap() const { return *m_menuicon_pm; }
  tk::PixmapWithMask &menuiconPixmap() { return *m_menuicon_pm; }

  tk::PixmapWithMask &titlePixmap() { return *m_title_pm; }
  const tk::PixmapWithMask &titlePixmap() const { return *m_title_pm; }

  tk::PixmapWithMask &leftHalfPixmap() { return *m_lefthalf_pm; }
  const tk::PixmapWithMask &leftHalfPixmap() const { return *m_lefthalf_pm; }

  tk::PixmapWithMask &rightHalfPixmap() { return *m_righthalf_pm; }
  const tk::PixmapWithMask &rightHalfPixmap() const { return *m_righthalf_pm; }

  virtual WinButtonTheme &operator *() { return *this; }
  virtual const WinButtonTheme &operator *() const { return *this; }

private:
  tk::ThemeItem<tk::PixmapWithMask>  m_close_pm,    m_maximize_pm,
                      m_iconify_pm,  m_shade_pm,    m_unshade_pm,
                      m_menuicon_pm, m_title_pm,    m_stick_pm,
                      m_stuck_pm,    m_lefthalf_pm, m_righthalf_pm;

  tk::ThemeProxy<SbWinFrameTheme> &m_frame_theme;
};

#endif // WINBUTTONTHEME_HH

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
