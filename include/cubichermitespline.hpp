#pragma once

#include "piecewisecubiccurve.hpp"

namespace asdf {

template<typename S, typename V>
class CubicHermiteSpline : public PiecewiseCubicCurve<S, V>
{
public:
  template<typename C1, typename C2, typename C3>
  CubicHermiteSpline(const C1& vertices, const C2& tangents, const C3& grid)
  {
    if (vertices.size() < 2)
    {
      throw std::runtime_error("At least 2 vertices are needed");
    }
    auto segments_size = vertices.size() - 1;
    if (tangents.size() != 2 * segments_size)
    {
      throw std::runtime_error("Exactly 2 tangents per segment are needed");
    }
    if (vertices.size() != grid.size())
    {
      throw std::runtime_error("As many grid times as vertices are needed");
    }
    if (std::adjacent_find(std::begin(grid), std::end(grid)
          , std::greater_equal<S>()) != std::end(grid))
    {
      throw std::runtime_error("Grid values must be strictly ascending");
    }

    this->_segments.reserve(segments_size);
    for (size_t i = 0; i < segments_size; ++i)
    {
      auto x0 = vertices[i];
      auto x1 = vertices[i + 1];
      auto v0 = tangents[2 * i];
      auto v1 = tangents[2 * i + 1];
      auto t0 = grid[i];
      auto t1 = grid[i + 1];
      auto delta = t1 - t0;

      // [a0]   [ 1,  0,          0,      0] [x0]
      // [a1] = [ 0,  0,      delta,      0] [x1]
      // [a2]   [-3,  3, -2 * delta, -delta] [v0]
      // [a3]   [ 2, -2,      delta,  delta] [v1]

      this->_segments.push_back({
                x0                                             ,
                                        delta * v0             ,
        -S(3) * x0 + S(3) * x1 - S(2) * delta * v0 - delta * v1,
         S(2) * x0 - S(2) * x1 +        delta * v0 + delta * v1});
    }
    this->_grid.assign(std::begin(grid), std::end(grid));
  }
};

}  // namespace asdf
