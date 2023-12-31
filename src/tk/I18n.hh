// I18n.hh for Shynebox Window Manager

/*
  Internationalization for multi-language strings.
  See comments in C file. Currently disabled :(
*/

#ifndef I18N_HH
#define I18N_HH

#include "../../nls/fluxbox-nls.hh"

#include "SbString.hh"

// Some defines to help out
#ifdef NLS
#define _SB_USES_NLS \
    tk::I18n &i18n = tk::I18n::instance()

// ignore the description, it's for helping translators

// Text for X
#define _SB_XTEXT(msgset, msgid, default_text, description) \
    i18n.getMessage(SBNLS::msgset ## Set, SBNLS::msgset ## msgid, default_text, true)

// Text for console
#define _SB_CONSOLETEXT(msgset, msgid, default_text, description) \
    i18n.getMessage(SBNLS::msgset ## Set, SBNLS::msgset ## msgid, default_text, false)

// This ensure that tk nls stuff is in a kind of namespace of its own
#define _TK_XTEXT( msgset, msgid, default_text, description) \
    i18n.getMessage(SBNLS::tk ## msgset ## Set, SBNLS::tk ## msgset ## msgid, default_text, true)

#define _TK_CONSOLETEXT( msgset, msgid, default_text, description) \
    i18n.getMessage(SBNLS::tk ## msgset ## Set, SBNLS::tk ## msgset ## msgid, default_text, false)

#else // no NLS

#define _SB_USES_NLS

#define _SB_XTEXT(msgset, msgid, default_text, description) \
    std::string(default_text)

#define _SB_CONSOLETEXT(msgset, msgid, default_text, description) \
    std::string(default_text)

#define _TK_XTEXT(msgset, msgid, default_text, description) \
    std::string(default_text)

#define _TK_CONSOLETEXT(msgset, msgid, default_text, description) \
    std::string(default_text)

#endif // defined NLS

namespace tk {

class I18n {
public:
  static void init(const char*);
  static I18n& instance();

  const char *getLocale() const { return m_locale.c_str(); }
  bool multibyte() const { return m_multibyte; }
  SbString getMessage(int set_number, int message_number,
                      const char *default_messsage = 0, bool translate_sb = false) const;

private:
  I18n();
  ~I18n();
  std::string m_locale;
  bool m_multibyte;
  bool m_utf8_translate;
};

} // end namespace tk

#endif // I18N_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2001 - 2002 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// i18n.hh for Blackbox - an X11 Window manager
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
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
