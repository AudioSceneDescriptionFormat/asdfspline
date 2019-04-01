#pragma once

#include <cassert>

namespace asdf {

/// https://en.wikipedia.org/wiki/Bisection_method
///
/// Root must be within [xmin, xmax], otherwise one of those is returned
/// (whichever has a function value closer to zero).
template<typename T, typename F>
T bisect(F f, T xmin, T xmax, T xtol, size_t max_calls)
{
  assert(xmin <= xmax);
  size_t calls = 0;
  T fmin = f(xmin);
  ++calls;
  if (fmin == 0)
  {
    return xmin;
  }
  T fmax = f(xmax);
  ++calls;
  if (fmax == 0)
  {
    return xmax;
  }

  if (fmin * fmax < 0)
  {
    while ((max_calls - calls) > 0 && (xmax - xmin) > xtol)
    {
      T xmid = (xmin + xmax) / 2;
      if (xmid == xmin || xmid == xmax)
      {
        return xmid;
      }
      T fmid = f(xmid);
      ++calls;
      if (fmid == 0)
      {
        return xmid;
      }
      if (fmin * fmid < 0)
      {
        xmax = xmid;
        fmax = fmid;
      }
      else
      {
        xmin = xmid;
        fmin = fmid;
      }
    }
  }
  using std::abs;
  return (abs(fmin) < abs(fmax)) ? xmin : xmax;
  // TODO: return number of calls?
  // TODO: return function value that's supposedly zero?
}

}  // namespace asdf
