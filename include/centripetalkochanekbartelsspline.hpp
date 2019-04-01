#pragma once

#include <cmath>  // for sqrt(), pow()
#include "cubichermitespline.hpp"

namespace asdf {

template<typename S, typename V>
class CentripetalKochanekBartelsSpline : public CubicHermiteSpline<S, V>
{
private:
  using _base = CubicHermiteSpline<S, V>;

public:
  template<typename C1, typename C2>
  CentripetalKochanekBartelsSpline(const C1& vertices, const C2& tcb
      , bool closed)
  : _base(std::make_from_tuple<_base>(_init(vertices, tcb, closed)))
  {}

private:
  template<typename C1, typename C2>
  static auto _init(const C1& vertices_in, const C2& tcb, bool closed)
  {
    std::tuple<std::vector<V>, std::vector<V>, std::vector<S>> result;
    auto& [vertices, tangents, grid] = result;

    if (vertices_in.size() < 2)
    {
      throw std::runtime_error("At least two vertices are required");
    }

    // TODO: reserve space for vertices?

    vertices.assign(std::begin(vertices_in), std::end(vertices_in));
    if (closed)
    {
      vertices.push_back(vertices_in[0]);
      vertices.push_back(vertices_in[1]);
    }

    if (tcb.size() + 2 != vertices.size())
    {
      throw std::runtime_error("There must be two more vertices than TCB "
                               "values (except for closed curves)");
    }

    // Create grid with centripetal parametrization

    // TODO: reserve space for grid?

    grid.push_back(0);
    for (size_t i = 0; i < vertices.size() - 1; ++i)
    {
      V x0 = vertices[i];
      V x1 = vertices[i + 1];
      auto delta = std::sqrt(length(x1 - x0));
      if (delta == 0)
      {
        throw std::runtime_error("Repeated vertices are not possible");
      }
      grid.push_back(grid.back() + delta);
    }

    // TODO: reserve space for tangents?

    tangents.emplace_back();  // Will be overwritten later

    assert(vertices.size() == grid.size());
    assert(vertices.size() == tcb.size() + 2);
    for (size_t i = 0; i < vertices.size() - 2; ++i)
    {
      auto [incoming, outgoing] = _calculate_tangents(
          vertices[i], vertices[i + 1], vertices[i + 2],
          grid[i], grid[i + 1], grid[i + 2],
          tcb[(i + closed) % tcb.size()]);
      tangents.push_back(incoming);
      tangents.push_back(outgoing);
    }

    if (closed)
    {
      // Move last (outgoing) tangent to the beginning:
      tangents[0] = tangents.back();
      tangents.pop_back();

      // Remove temporary vertex and grid elements:
      vertices.pop_back();
      grid.pop_back();
    }
    else if (vertices.size() == 2)
    {
      // Straight line
      assert(grid.size() == 2);
      assert(tangents.size() == 1);
      V tangent = (vertices[1] - vertices[0]) / (grid[1] - grid[0]);
      tangents[0] = tangent;
      tangents.push_back(tangent);
    }
    else
    {
      // End conditions for non-closed curves
      assert(tangents.size() >= 2);
      tangents[0] = _end_tangent(vertices[0], vertices[1], grid[0], grid[1]
          , tangents[1]);
      auto last = vertices.size() - 1;
      tangents.push_back(_end_tangent(vertices[last - 1], vertices[last]
          , grid[last - 1], grid[last], tangents[tangents.size() - 1]));
    }
    return result;
  }

  template<typename X>
  static std::tuple<V, V>
  _calculate_tangents(V x_1, V x0, V x1, S t_1, S t0, S t1, X tcb)
  {
    auto [T, C, B] = tcb;
    auto a = (1 - T) * (1 + C) * (1 + B);
    auto b = (1 - T) * (1 - C) * (1 - B);
    auto c = (1 - T) * (1 - C) * (1 + B);
    auto d = (1 - T) * (1 + C) * (1 - B);
    using std::pow;
    auto incoming = (
      c * pow(t1 - t0, S(2)) * (x0 - x_1) + d * pow(t0 - t_1, S(2)) * (x1 - x0)
    ) / (
      (t1 - t0) * (t0 - t_1) * (t1 - t_1)
    );
    auto outgoing = (
      a * pow(t1 - t0, S(2)) * (x0 - x_1) + b * pow(t0 - t_1, S(2)) * (x1 - x0)
    ) / (
      (t1 - t0) * (t0 - t_1) * (t1 - t_1)
    );
    return {incoming, outgoing};
  }

  /// "natural" end conditions
  static V _end_tangent(V x0, V x1, S t0, S t1, V inner_tangent)
  {
    auto delta = t1 - t0;
    return (S(3) * x1 - S(3) * x0 - delta * inner_tangent) / (S(2) * delta);
  }
};

}  // namespace asdf
