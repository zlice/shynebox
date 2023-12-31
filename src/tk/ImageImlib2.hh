// ImageImLib2.hh for Shynebox Window Manager

/*
  Imlib2 Image loader for common file types.
*/

#ifndef TK_IMAGEIMLIB2_HH
#define TK_IMAGEIMLIB2_HH

#include "Image.hh"
namespace tk {

class ImageImlib2: public ImageBase {
public:
  ImageImlib2();
  PixmapWithMask *load(const std::string &filename, int screen_num) const;
};

} // end namespace tk

#endif // TK_IMAGEIMLIB2_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2004 Mathias Gumz (akira at fluxbox dot org)
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
