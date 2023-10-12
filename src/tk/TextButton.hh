// TextButton.hh for Shynebox Window Manager

/*
  A Button with a text label.
*/

#ifndef TK_TEXTBUTTON_HH
#define TK_TEXTBUTTON_HH

#include "Button.hh"
#include "SbString.hh"

namespace tk {

class Font;

// Displays a text on a button
class TextButton: public tk::Button, tk::SbWindowRenderer {
public:
  TextButton(const tk::SbWindow &parent,
             tk::Font &font, const tk::BiDiString &text);

  void setJustify(tk::Justify just);
  void setOrientation(tk::Orientation orient);
  void setText(const tk::BiDiString &text);
  void setFont(tk::Font &font);
  void setTextPadding(unsigned int padding);
  void setTextPaddingLeft(unsigned int leftpadding);
  void setTextPaddingRight(unsigned int rightpadding);

  // clears window and redraw text
  void clear();
  // clears area and redraws text
  void clearArea(int x, int y,
                 unsigned int width, unsigned int height,
                 bool exposure = false);

  void exposeEvent(XExposeEvent &event);

  void renderForeground(SbWindow &win, SbDrawable &drawable);

  tk::Justify justify() const { return m_justify; }
  const BiDiString &text() const { return m_text; }
  tk::Font &font() const { return *m_font; }
  tk::Orientation orientation() const { return m_orientation; }
  unsigned int textWidth() const;
  int bevel() const { return m_bevel; }

  virtual unsigned int preferredWidth() const;

protected:
  virtual void drawText(int x_offset, int y_offset, SbDrawable *drawable_override);
  // return true if the text will be truncated
  bool textExceeds(int x_offset);

private:
  tk::Font *m_font;
  BiDiString m_text;
  tk::Justify m_justify;
  tk::Orientation m_orientation;

  int m_bevel;
  unsigned int m_left_padding; // space between buttonborder and text
  unsigned int m_right_padding;
};

} // end namespace tk

#endif // TK_TEXTBUTTON_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 Henrik Kinnunen (fluxgen[at]fluxbox.org)
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
