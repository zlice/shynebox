// Focusable.hh for Shynebox Window Manager

/*
  Abstract base class for focusable windows.
  Used for 'Window', 'WinClient' and 'Menu's
*/

#ifndef FOCUSABLE_HH
#define FOCUSABLE_HH

#include "tk/PixmapWithMask.hh"
#include "tk/ITypeAheadable.hh"
#include "tk/SbString.hh"

class BScreen;
class ShyneboxWindow;

class Focusable: public tk::ITypeAheadable {
public:
  Focusable(BScreen &scr, ShyneboxWindow *sbwin = 0):
      m_screen(scr), m_sbwin(sbwin),
      m_instance_name("shynebox"), m_class_name("shynebox"),
      m_focused(false) { }
  virtual ~Focusable() { }

  // true if ~this~ took focus
  virtual bool focus() { return false; }

  // true if ~this~ has focus
  virtual bool isFocused() const { return m_focused; }
  // true if ~this~ can be focused
  virtual bool acceptsFocus() const { return true; }
  // true if ~this~ temporarily prevented from being focused
  virtual bool isModal() const { return false; }

  BScreen &screen() { return m_screen; }
  const BScreen &screen() const { return m_screen; }

  ShyneboxWindow *sbwindow() { return m_sbwin; }
  const ShyneboxWindow *sbwindow() const { return m_sbwin; }

  // ShyneboxWindow or WinClient should override for pattern matching
  // WM_CLASS class
  virtual const tk::SbString &getWMClassClass() const { return m_class_name; }
  // WM_CLASS name
  virtual const tk::SbString &getWMClassName() const { return m_instance_name; }
  // wm role
  virtual std::string getWMRole() const { return "Focusable"; }
  virtual tk::SbString getTextProperty(Atom prop ,bool *exists=NULL) const {
    (void)prop; (void)exists;
    return "";
  }
  virtual long getCardinalProperty(Atom prop, bool *exists=NULL) const {
    (void)prop; (void)exists;
    return 0;
  }
  // whether this window is a transient (for pattern matching)
  virtual bool isTransient() const { return false; }

  // so we can make nice buttons, menu entries, etc.
  // icon pixmap of the focusable
  virtual const tk::PixmapWithMask &icon() const { return m_icon; }
  virtual const tk::BiDiString &title() const { return m_title; }
  // type ahead string for menu searching
  const std::string &iTypeString() const { return title().logical(); }

  virtual void setInView(bool in_v) { m_in_view = in_v; }
  virtual bool isInView() const { return m_in_view; }

protected:
  BScreen &m_screen; // the screen in which it works
  ShyneboxWindow *m_sbwin = 0; // the working shynebox window

  tk::BiDiString m_title;
  tk::SbString m_instance_name,
                 m_class_name;

  bool m_focused = false; // if it has focus
  bool m_in_view = false; // if it's viewable on screen
  tk::PixmapWithMask m_icon; // icon pixmap with mask
};

#endif // FOCUSABLE_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2007 Fluxbox Team (fluxgen at fluxbox dot org)
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
