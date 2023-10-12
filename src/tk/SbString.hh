// SbString.hh for Shynebox Window Manager

/*
  Base string class for bi-directional strings.
  Use this type for things converted to our internal encoding (UTF-8)
  (or just plain whatever for now if no utf-8 available)
*/

#ifndef TK_SBSTRING_HH
#define TK_SBSTRING_HH

#include <string>

#ifdef HAVE_ICONV
#include <iconv.h>
#endif // HAVE_ICONV

#include "NotCopyable.hh"

namespace tk {

typedef std::string SbString;

class BiDiString {
public:
  BiDiString(const SbString& logical = SbString() );

  const SbString& logical() const { return m_logical; }
  const SbString& visual() const;

  const SbString& setLogical(const SbString& logical);

private:
  SbString m_logical;
#ifdef HAVE_FRIBIDI
  mutable SbString m_visual;
  mutable bool m_visual_dirty;
#endif
};

namespace SbStringUtil {
  void init();
  void shutdown();
  // Stuff to handle strings in different encodings
  // Rule: Only hardcode-initialise SbStrings as ascii (7bit) characters

  // NOTE: X "STRING" types are defined (ICCCM) as ISO Latin-1 encoding
  SbString XStrToSb(const std::string &src);
  std::string SbStrToX(const SbString &src);

  // Handle thislocale string encodings (strings coming from userspace)
  SbString LocaleStrToSb(const std::string &src);
  std::string SbStrToLocale(const SbString &src);

  bool haveUTF8();

} // namespace SbStringUtil

class StringConvertor: private NotCopyable {
public:
  enum EncodingTarget { ToSbString, ToLocaleStr };

  StringConvertor(EncodingTarget target);
  ~StringConvertor();

  bool setSource(const std::string &encoding);
  void reset();

  SbString recode(const SbString &src);

private:
#ifdef HAVE_ICONV
  iconv_t m_iconv;
#endif
  std::string m_destencoding;
};

} // namespace tk

#endif // TK_SBSTRING_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
// Copyright (c) 2006 Simon Bowden    (rathnor at fluxbox dot org)
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
