// RectangleUtil.hh for Shynebox Window Manager
/*
  Utility for rectangle math.
*/

#ifndef RECTANGLEUTIL_HH
#define RECTANGLEUTIL_HH

namespace RectangleUtil {

// https://stackoverflow.com/questions/17095324/fastest-way-to-determine-if-an-integer-is-between-two-integers-inclusive-with/17095534#17095534
/*
 * width/height should always be positive, thus underflow will be great-than.
 * if test-x/y was somehow over int max value this may need reworked
 * but you'd have different issues then
 * and should never have values that high with today's monitors (2023)
*/
inline bool insideRectangle(int rx, int ry, int rw, int rh, int tx, int ty) {
  return ((unsigned)(tx - rx) <= rw)
      && ((unsigned)(ty - ry) <= rh);
}

/*
 * Determines if a point is inside a rectangle-like objects border.
 * RectangleLike is an object that has accessors for x, y, width, and height.
*/

template <typename RectangleLike>
bool insideBorder(const RectangleLike& rect,
                  int x, int y, int border) {
  const int w = static_cast<int>(rect.width() ) - border;
  const int h = static_cast<int>(rect.height() ) - border;
  return insideRectangle(rect.x() + border, rect.y() + border, w, h, x, y);
}

// Left in for reference

/*
 * Determines if rectangle 'a' overlaps rectangle 'b'
 * @returns true if 'a' overlaps 'b'
 *
 *    outside              overlap situations
 *
 *  a----a            a----a         a--------a  b--------b
 *  |    | b----b     |  b-+--b      | b----b |  | a----a |
 *  a----a |    |     a--+-a  |      | |    | |  | |    | |
 *         b----b        b----b      | b----b |  | a----a |
 *                                   a--------a  b--------b
 *
 */

/*
inline bool overlapRectangles(
        int ax, int ay, int awidth, int aheight,
        int bx, int by, int bwidth, int bheight) {
  bool do_not_overlap =
       ax > (bx + bwidth)
    || bx > (ax + awidth)
    || ay > (by + bheight)
    || by > (ay + aheight);

  return !do_not_overlap;
}


 // Determines if rectangle 'a' overlaps rectangle 'b'
template <typename RectangleLikeA, typename RectangleLikeB>
bool overlapRectangles(const RectangleLikeA& a, const RectangleLikeB& b) {
  return overlapRectangles(
          a.x(), a.y(), a.width(), a.height(),
          b.x(), b.y(), b.width(), b.height() );
}
*/

} // namespace RectangleUtil

#endif // RECTANGLEUTIL_HH

// Copyright (c) 2023 Shynebox - zlice
//
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
