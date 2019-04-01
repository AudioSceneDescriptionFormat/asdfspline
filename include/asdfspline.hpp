#pragma once

#include <variant>
#include <memory>  // for unique_ptr

#include "bisect.hpp"
#include "centripetalkochanekbartelsspline.hpp"
#include "monotonecubicspline.hpp"

/// Main ASDF namespace
namespace asdf {

/// Dummy type to mark the last vertex in a closed curve
struct CLOSED {};

template<typename S, typename V>
class AsdfSpline
{
public:
  struct AsdfVertex
  {
    // Only last vertex can be "closed"
    std::variant<V, CLOSED> position;
    std::optional<S> time;
    std::optional<S> speed;
    std::array<S, 3> tcb{};
  };

  /// Container of AsdfVertex elements
  template<typename C>
  AsdfSpline(const C& data)
  : _init(new Initializer(data))
  , _path(_init->vertices, _init->tcb, _init->closed)
  , _t2s(std::make_from_tuple<MonotoneCubicSpline<S>>(
        _init->get_t2s_arguments(_path)))
  , _grid(_init->get_grid(_t2s))
  {
    _init.reset();  // Initializer is not needed anymore
    assert(_path.grid().size() == _grid.size());
    std::transform(_grid.begin(), _grid.end(), std::back_inserter(_s_grid)
        , [this](S t){ return _t2s.evaluate(t); });
  }

  V evaluate(S t) const
  {
    return _path.evaluate(_s2u(_t2s.evaluate(t)));
  }

  V evaluate_velocity(S t) const
  {
    S speed = _t2s.evaluate_velocity(t);
    V tangent = _path.evaluate_velocity(_s2u(_t2s.evaluate(t)));
    if (S tangent_length = length(tangent))
    {
      tangent /= tangent_length;
    }
    return speed * tangent;
  }

  auto& grid() const { return _grid; }

private:
  struct Initializer;

  /// If s is outside, return clipped u.
  S _s2u(S s) const
  {
    static_assert(std::is_same_v<S, float>
        , "For now, this only works with float");

    // TODO: proper accuracy (a bit less than single-precision?)
    auto accuracy = S(0.0001);

    size_t index;
    if (s <= _s_grid.front())
    {
      return _path.grid().front();
    }
    else if (s < _s_grid.back())
    {
      index = std::upper_bound(_s_grid.begin(), _s_grid.end(), s)
        - _s_grid.begin() - 1;
    }
    else
    {
      return _path.grid().back();
    }
    s -= _s_grid[index];
    S u0 = _path.grid()[index];
    S u1 = _path.grid()[index + 1];
    auto func = [&](S u){
      return _path.segment_length(index, u0, u) - s;
    };
    return bisect(func, u0, u1, accuracy, 50);
  }

  std::unique_ptr<Initializer> _init;
  CentripetalKochanekBartelsSpline<S, V> _path;
  MonotoneCubicSpline<S> _t2s;
  std::vector<S> _grid;
  std::vector<S> _s_grid;
};


template<typename S, typename V>
struct AsdfSpline<S, V>::Initializer
{
  template<typename C>
  explicit Initializer(const C& data)
  {
    if (data.size() < 2)
    {
      throw std::runtime_error("At least two vertices are required");
    }

    this->closed = std::holds_alternative<CLOSED>(data.back().position);

    for (size_t i = 0; i < data.size(); ++i)
    {
      const auto& current = data[i];
      if (std::holds_alternative<V>(current.position))
      {
        this->vertices.push_back(std::get<V>(current.position));
      }
      else if (i != data.size() - 1)
      {
        throw std::runtime_error("CLOSED is only allowed on last vertex");
      }

      if (current.time)
      {
        this->times.push_back(*current.time);
        this->speeds.push_back(current.speed);
      }
      else if (i == 0)
      {
        this->times.push_back(0);
        this->speeds.push_back(current.speed);
      }
      else if (i == data.size() - 1)
      {
        throw std::runtime_error("Time of last vertex must be specified");
      }
      else
      {
        this->missing_times.push_back(i);
        if (current.speed)
        {
          throw std::runtime_error("Speed is only allowed if time is given");
        }
      }
      if ((this->closed || 0 < i) && i < data.size() - 1)
      {
        this->tcb.push_back(current.tcb);
      }
      else if (current.tcb != decltype(current.tcb){})
      {
        throw std::runtime_error("TCB is not allowed for the first (except "
                                 "closed curves) and last vertex");
      }
    }
  }

  auto get_t2s_arguments(const CentripetalKochanekBartelsSpline<S, V>& path)
  {
    std::vector<S> lengths;
    lengths.push_back(0);

    for (size_t i = 0; i < path.grid().size() - 1; ++i)
    {
      S length = path.segment_length(i);
      if (std::find(this->missing_times.begin()
                  , this->missing_times.end(), i) != this->missing_times.end())
      {
        this->lengths_at_missing_times.push_back(lengths.back());
        lengths.back() += length;
      }
      else
      {
        lengths.push_back(lengths.back() + length);
      }
    }
    return std::make_tuple(lengths, this->speeds, this->times);
  }

  auto get_grid(const MonotoneCubicSpline<S>& t2s)
  {
    assert(this->missing_times.size() == this->lengths_at_missing_times.size());
    for (size_t i = 0; i < this->missing_times.size(); ++i)
    {
      if (auto time = t2s.get_time(this->lengths_at_missing_times[i]))
      {
        this->times.insert(times.begin() + this->missing_times[i], *time);
      }
      else
      {
        throw std::runtime_error("duplicate vertex without time");
      }
    }
    return this->times;
  }

  bool closed;
  std::vector<V> vertices;
  std::vector<S> times;
  std::vector<size_t> missing_times;
  std::vector<std::optional<S>> speeds;
  std::vector<std::array<S, 3>> tcb;
  std::vector<S> lengths_at_missing_times;
};

}  // namespace asdf
