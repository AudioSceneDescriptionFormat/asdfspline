#pragma once

#include <algorithm>

#include "shapepreservingcubicspline.hpp"
#include "bisect.hpp"

namespace asdf {

/// NB: Input values must be increasing.
template<typename S>
class MonotoneCubicSpline : public ShapePreservingCubicSpline<S>
{
public:
  template<typename C, typename... Args>
  MonotoneCubicSpline(C&& values, Args&&... args)
  : ShapePreservingCubicSpline<S>(values, std::forward<Args>(args)..., false)
  , _values(std::forward<C>(values))
  {
    if (!std::is_sorted(std::begin(_values), std::end(_values)))
    {
      throw std::invalid_argument("Values must be increasing");
    }
  }

  /// Get the time instance for the given value.
  /// If the solution is not unique, std::nullopt is returned.
  /// If "value" is outside of the range, the first/last time is returned.
  // TODO: rename to something with "solve"?
  std::optional<S> get_time(S value) const
  {
    // NB: If initially given values are monotone (which we checked above!),
    // repetitions (i.e. a plateau) can only occur at those exact values.

    auto [beginmatch, endmatch] =
      std::equal_range(_values.begin(), _values.end(), value);

    if (endmatch == _values.begin())
    {
      // Value too small
      return this->_grid.front();
    }
    else if (beginmatch == _values.end())
    {
      // Value too large
      return this->_grid.back();
    }
    else if (endmatch - beginmatch == 1)
    {
      // Exactly one match
      return this->_grid[beginmatch - _values.begin()];
    }
    else if (endmatch - beginmatch > 1)
    {
      // Multiple matches
      return std::nullopt;
    }

    auto idx = endmatch - _values.begin() - 1;
    auto a = this->_segments[idx];
    a[0] -= value;
    auto func = [&a](S t) {
      return ((a[3] * t + a[2]) * t + a[1]) * t + a[0];
    };
    // TODO: proper tolerance value
    S time = bisect(func, S(0), S(1), S(0.0001), 500);
    assert(0 <= time && time <= 1);
    S t0 = this->_grid[idx];
    S t1 = this->_grid[idx + 1];
    return time * (t1 - t0) + t0;
  }

private:
  std::vector<S> _values;
};

}  // namespace asdf
