#pragma once

#include <algorithm>  // for upper_bound()
#include <array>
#include <cassert>
#include <vector>

#include "gauss-legendre.hpp"

namespace asdf {

using std::size_t;

template<typename S, typename V>
class PiecewiseCubicCurve
{
public:
  // NB: Base class ctor has to correctly populate _segments and _grid

  V evaluate(S t) const
  {
    auto [t0, t1, a] = _get_segment_and_trim(t);
    t = (t - t0) / (t1 - t0);
    return ((a[3] * t + a[2]) * t + a[1]) * t + a[0];
  }

  V evaluate_velocity(S t) const
  {
    auto [t0, t1, coeffs] = _get_segment_and_trim(t);
    return _segment_velocity(t0, t1, coeffs, t);
  }

  /// Read-only access
  auto& grid() const { return _grid; }

  S segment_length(size_t index) const
  {
    S t0 = _grid.at(index);
    S t1 = _grid.at(index + 1);
    return this->segment_length(index, t0, t1);
  }

  S segment_length(size_t index, S a, S b) const
  {
    const auto& coeffs = _segments.at(index);
    assert(a <= b);
    S t0 = _grid.at(index);
    S t1 = _grid.at(index + 1);
    assert(t0 <= a);
    assert(b <= t1);

    auto speed = [&](S t) {
      return length(_segment_velocity(t0, t1, coeffs, t));
    };

    static_assert(std::is_same_v<S, float>
        , "For now, this only works with float");
    return gauss_legendre13(speed, a, b);
  }

protected:
  std::vector<std::array<V, 4>> _segments;
  std::vector<S> _grid;

private:
  // If t is out of bounds, it is trimmed to the smallest/largest possible value
  auto _get_segment_and_trim(S& t) const
  {
    assert(_grid.size() >= 2);
    size_t idx;
    if (t < _grid.front())
    {
      t = _grid.front();
      idx = 0;
    }
    else if (t < _grid.back())
    {
      idx = std::upper_bound(_grid.begin(), _grid.end(), t) - _grid.begin() - 1;
    }
    else if (t == _grid.back())
    {
      idx = _grid.end() - _grid.begin() - 2;
    }
    else
    {
      t = _grid.back();
      assert(_segments.size() >= 1);
      idx = _segments.size() - 1;
    }
    assert(idx < _segments.size());
    return std::tuple{_grid[idx], _grid[idx + 1], _segments[idx]};
  }

  static V _segment_velocity(S t0, S t1, const std::array<V, 4>& a, S t)
  {
    t = (t - t0) / (t1 - t0);
    return ((S(3) * a[3] * t + S(2) * a[2]) * t + a[1]) / (t1 - t0);
  }
};

}  // namespace asdf
