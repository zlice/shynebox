// TooltipWindow.hh for Shynebox Window Manager

/*
 A pop-up window that holds information about the IconButton related to it.
*/

#ifndef TOOLTIPWINDOW_HH
#define TOOLTIPWINDOW_HH

#include "OSDWindow.hh"
#include "tk/Command.hh"
#include "tk/Timer.hh"
#include "tk/SimpleCommand.hh"
#include "tk/SbString.hh"

class TooltipWindow : public OSDWindow  {
public:
  TooltipWindow(const tk::SbWindow &parent, BScreen &screen,
                tk::ThemeProxy<SbWinFrameTheme> &theme);
  /**
   * Sets the text in the window and starts the display timer.
   * @param text the text to show in the window.
   */
  void showText(const tk::BiDiString& text);
  // updates the text directly without any delay
  void updateText(const tk::BiDiString& text);

  // Sets the delay before the window pops up
  void setDelay(int delay) {
    m_delay = delay;
    m_timer.setTimeout(delay * tk::SbTime::IN_MILLISECONDS);
  }

  void hide();

private:
  void raiseTooltip();
  void show();
  int m_delay; // delay time for the timer
  tk::BiDiString m_lastText; // last text to be displayed
  tk::Timer m_timer; // delay timer before the tooltip will show
};

#endif // TOOLTIPWINDOW_HH_

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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
