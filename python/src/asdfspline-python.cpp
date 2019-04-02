#include <pybind11/pybind11.h>
//#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "asdfspline.hpp"
#include "vec3.hpp"

namespace py = pybind11;
using namespace pybind11::literals;

template<typename T>
class AsdfSpline : public asdf::AsdfSpline<T, Vec3<T>>
{
public:
  using V = Vec3<T>;

  explicit AsdfSpline(py::iterable data)
  : asdf::AsdfSpline<T, V>(_init(data))
  {}

  auto grid_as_array() const
  {
    auto& grid = this->grid();
    // NB: "none" is used to get non-owning reference to original memory:
    py::array_t<T> array(grid.size(), grid.data(), py::none());
    // Make read-only:
    py::detail::array_proxy(array.ptr())->flags
      &= ~py::detail::npy_api::NPY_ARRAY_WRITEABLE_;
    return array;
  }

private:
  auto _init(py::iterable data)
  {
    std::vector<typename asdf::AsdfSpline<T, V>::AsdfVertex> vertices;
    for (const auto& item: data)
    {
      if (!py::isinstance(item
            , py::module::import("collections.abc").attr("Mapping")))
      {
        throw py::type_error("Expected an iterable of dictionaries");
      }
      std::variant<V, asdf::CLOSED> position;
      try
      {
        auto name = item["position"].cast<std::string>();
        if (name == "closed")
        {
          position = asdf::CLOSED();
        }
        else
        {
          throw py::cast_error("Invalid string for position");
        }
      }
      catch (py::cast_error&)
      {
        position = item["position"].cast<V>();
      }

      std::optional<T> time;
      std::optional<T> speed;
      T tension = 0;
      T continuity = 0;
      T bias = 0;
      if (item.contains("time"))
      {
        time = item["time"].cast<T>();
      }
      if (item.contains("speed"))
      {
        speed = item["speed"].cast<T>();
      }
      if (item.contains("tension"))
      {
        tension = item["tension"].cast<T>();
      }
      if (item.contains("continuity"))
      {
        continuity = item["continuity"].cast<T>();
      }
      if (item.contains("bias"))
      {
        bias = item["bias"].cast<T>();
      }
      vertices.push_back({position, time, speed, {tension, continuity, bias}});
    }
    return vertices;
  }
};


namespace pybind11 { namespace detail {
  template <typename T> struct type_caster<Vec3<T>>
  {
  private:
    using _vec = Vec3<T>;

  public:
    PYBIND11_TYPE_CASTER(_vec, _("Vec3<T>"));

    // Python -> C++
    bool load(py::handle src, bool convert)
    {
      if (!convert && !py::array_t<T>::check_(src)) { return false; }
      auto buf = py::array_t<T>::ensure(src);
      if (!buf || buf.ndim() != 1 || buf.size() != 3) { return false; }
      value.x = buf.at(0);
      value.y = buf.at(1);
      value.z = buf.at(2);
      return true;
    }

    // C++ -> Python
    static py::handle cast(const _vec& src,
        py::return_value_policy policy, py::handle parent)
    {
      (void)policy;
      (void)parent;

      py::array_t<T> a({3});
      a.mutable_at(0) = src.x;
      a.mutable_at(1) = src.y;
      a.mutable_at(2) = src.z;
      return a.release();
    }
  };
}}


PYBIND11_MODULE(asdfspline, m) {
  m.doc() = R"raw(ASDF splines.)raw";

  py::class_<AsdfSpline<float>>(m, "AsdfSpline",
R"raw(ASDF spline.)raw")
    .def(py::init<py::iterable>(), "data"_a,
R"raw(Construct a spline from an iterable of dicts.)raw")
    .def("evaluate", &AsdfSpline<float>::evaluate, "t"_a,
R"raw(Evaluate position at *t*.)raw")
    .def("evaluate_velocity", &AsdfSpline<float>::evaluate_velocity, "t"_a,
R"raw( Evaluate velocity at *t*.)raw")
    .def_property_readonly("grid", &AsdfSpline<float>::grid_as_array)
    ;

#ifdef VERSION_INFO
  m.attr("__version__") = VERSION_INFO;
#else
  m.attr("__version__") = "unknown";
#endif
}
