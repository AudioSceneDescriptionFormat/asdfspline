#pragma once

#include <optional>
#include <tuple>

#include "cubichermitespline.hpp"

namespace asdf {

template<typename S>
class ShapePreservingCubicSpline : public CubicHermiteSpline<S, S>
{
private:
  using _base = CubicHermiteSpline<S, S>;

public:
  template<typename C1, typename C2>
  ShapePreservingCubicSpline(const C1& values, const C2& grid, bool closed)
  : _base(std::make_from_tuple<_base>(_init(values, grid, closed)))
  {}

  template<typename C1, typename C2, typename C3>
  ShapePreservingCubicSpline(const C1& values, const C2& slopes, const C3& grid
      , bool closed)
  : _base(std::make_from_tuple<_base>(_init(values, slopes, grid, closed)))
  {}

private:
  /// Add undefined slopes and call the other _init() overload.
  template<typename C1, typename C2>
  static auto _init(const C1& values, const C2& grid, bool closed)
  {
    return _init(
        values, std::vector<std::optional<S>>(values.size()), grid, closed);
  }

  template<typename C1, typename C2, typename C3>
  static auto _init(
      const C1& values_in, const C2& slopes_in, const C3& grid_in, bool closed)
  {
    if (values_in.size() < 2)
    {
      throw std::runtime_error("At least two values are required");
    }
    if (values_in.size() != grid_in.size() + closed)
    {
      throw std::runtime_error("Number of grid values must be same as values "
                               "(one more for closed curves)");
    }
    if (values_in.size() != slopes_in.size())
    {
      throw std::runtime_error("Number of slopes must be same as values");
    }

    std::tuple<std::vector<S>, std::vector<S>, std::vector<S>> result;
    auto& [values, slopes, grid] = result;

    // TODO: reserve space for values?
    // TODO: reserve space for grid?

    values.assign(std::begin(values_in), std::end(values_in));
    grid.assign(std::begin(grid_in), std::end(grid_in));

    // TODO: check if grid values are increasing?

    if (closed)
    {
      values.push_back(values_in[0]);
      values.push_back(values_in[1]);
      grid.push_back(grid.back() + grid[1] - grid[0]);
    }

    slopes.emplace_back();  // Will be overwritten later

    for (size_t i = 0; i < values.size() - 2; ++i)
    {
      S x_1 = values[i];
      S x0 = values[i + 1];
      S x1 = values[i + 2];
      S t_1 = grid[i];
      S t0 = grid[i + 1];
      S t1 = grid[i + 2];
      S left = (x0 - x_1) / (t0 - t_1);
      S right = (x1 - x0) / (t1 - t0);
      auto maybe_slope = slopes_in[(i + 1) % slopes_in.size()];
      S slope;
      if (maybe_slope)
      {
        slope = *maybe_slope;
        if (slope != _fix_slope(slope, left, right))
        {
          throw std::runtime_error("Slope too steep or wrong sign");
        }
      }
      else
      {
        slope = _calculate_slope(x_1, x0, x1, t_1, t0, t1);
        slope = _fix_slope(slope, left, right);
      }
      slopes.push_back(slope);  // incoming
      slopes.push_back(slope);  // outgoing
    }

    if (closed)
    {
      // Move last (outgoing) slope to the beginning:
      slopes[0] = slopes.back();
      slopes.pop_back();
    }
    else if (slopes_in.size() == 2)
    {
      slopes.clear();  // Remove temporary element
      S chord = (values_in[1] - values_in[0]) / (grid_in[1] - grid_in[0]);
      auto one = slopes_in[0];
      auto two = slopes_in[1];

      auto check_slope = [chord](S slope) {
        if (slope != _fix_slope(slope, chord, chord))
        {
          throw std::runtime_error("Slope too steep or wrong sign");
        }
      };

      if (one)
      {
        check_slope(*one);
        slopes.push_back(*one);
        if (two)
        {
          check_slope(*two);
          slopes.push_back(*two);
        }
        else
        {
          slopes.push_back(_end_slope(*one, chord));
        }
      }
      else
      {
        if (two)
        {
          check_slope(*two);
          slopes.push_back(_end_slope(*two, chord));
          slopes.push_back(*two);
        }
        else
        {
          slopes.push_back(chord);
          slopes.push_back(chord);
        }
      }
    }
    else
    {
      auto end_slope = [](auto outer, S inner, S chord) {
        if (outer)
        {
          if (*outer != _fix_slope(*outer, chord, chord))
          {
            throw std::runtime_error("Slope too steep or wrong sign");
          }
          return *outer;
        }
        else
        {
          return _end_slope(inner, chord);
        }
      };

      slopes[0] = end_slope(slopes_in.front(), slopes[1]
          , (values[1] - values[0]) / (grid[1] - grid[0]));
      slopes.push_back(end_slope(slopes_in.back(), slopes.back()
          , (values[values.size() - 1] - values[values.size() - 2])
          / (grid[grid.size() - 1] - grid[grid.size() - 2])));
    }
    return result;
  }

  static S _calculate_slope(S x_1, S x0, S x1, S t_1, S t0, S t1)
  {
    return ((x0 - x_1) / (t0 - t_1) + (x1 - x0) / (t1 - t0)) / 2;
  }

  /// Manipulate the slope to preserve shape.
  /// See Dougherty et al. (1989), eq. (4.2).
  static S _fix_slope(S slope, S left, S right)
  {
    using std::min, std::max, std::abs;
    S zero = 0;
    if (left * right <= zero)
    {
      return zero;
    }
    else if (right > zero)
    {
      return min(max(zero, slope), 3 * min(abs(left), abs(right)));
    }
    else
    {
      return max(min(zero, slope), -3 * min(abs(left), abs(right)));
    }
  }

  static S _end_slope(S inner_slope, S chord_slope)
  {
    // NB: This is a very ad-hoc algorithm meant to minimize the change in slope
    // within the first/last curve segment.  Especially, this should avoid a
    // change from negative to positive acceleration (and vice versa).
    // There might be a better method available!?!
    if (chord_slope < 0)
    {
      return -_end_slope(-inner_slope, - chord_slope);
    }
    assert(0 <= inner_slope);
    assert(inner_slope <= 3 * chord_slope);
    if (inner_slope <= chord_slope)
    {
      return 3 * chord_slope - 2 * inner_slope;
    }
    else
    {
      return (3 * chord_slope - inner_slope) / 2;
    }
  }
};

}  // namespace asdf
