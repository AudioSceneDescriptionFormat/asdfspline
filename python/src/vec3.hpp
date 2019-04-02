/// Quick and dirty three-dimensional vector type

#include <cmath>  // for std::hypot()

template<typename T>
struct Vec3
{
  T x{}, y{}, z{};

  Vec3& operator+=(const Vec3<T>& rhs)
  {
    this->x += rhs.x;
    this->y += rhs.y;
    this->z += rhs.z;
    return *this;
  }

  Vec3& operator-=(const Vec3<T>& rhs)
  {
    this->x -= rhs.x;
    this->y -= rhs.y;
    this->z -= rhs.z;
    return *this;
  }

  Vec3& operator*=(T rhs)
  {
    this->x *= rhs;
    this->y *= rhs;
    this->z *= rhs;
    return *this;
  }

  Vec3& operator/=(T rhs)
  {
    this->x /= rhs;
    this->y /= rhs;
    this->z /= rhs;
    return *this;
  }
};

template<typename T>
Vec3<T> operator+(const Vec3<T>& lhs, const Vec3<T>& rhs)
{
   auto result = Vec3<T>{lhs};
   result += rhs;
   return result;
}

template<typename T>
Vec3<T> operator-(const Vec3<T>& lhs, const Vec3<T>& rhs)
{
   auto result = Vec3<T>{lhs};
   result -= rhs;
   return result;
}

template<typename T>
Vec3<T> operator*(const Vec3<T>& lhs, T rhs)
{
   auto result = Vec3<T>{lhs};
   result *= rhs;
   return result;
}

template<typename T>
Vec3<T> operator*(T lhs, const Vec3<T>& rhs)
{
  return rhs * lhs;
}

template<typename T>
Vec3<T> operator/(const Vec3<T>& lhs, T rhs)
{
   auto result = Vec3<T>{lhs};
   result /= rhs;
   return result;
}

template<typename T>
T length(Vec3<T> v)
{
  return std::hypot(v.x, v.y, v.z);
}
