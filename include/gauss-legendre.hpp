#pragma once

namespace asdf {

/// Gauss-Legendre quadrature of order 13.
///
/// https://en.wikipedia.org/wiki/Gaussian_quadrature
///
/// Arrays were generated with scipy.special.roots_legendre(13).
/// 13th order typically leads to results within single-precision
/// accuracy [citation needed].
///
/// See also https://pomax.github.io/bezierinfo/legendre-gauss.html
template<typename F>
float gauss_legendre13(F f, float a, float b)
{
  std::array<float, 13> times = {
       -0.9841830547185881f, -0.9175983992229779f , -0.8015780907333099f ,
       -0.6423493394403403f, -0.44849275103644687f, -0.23045831595513483f,
        0.f                ,  0.23045831595513483f,  0.44849275103644687f,
        0.6423493394403403f,  0.8015780907333099f ,  0.9175983992229779f ,
        0.9841830547185881f};
  std::array<float, 13> weights = {
        0.04048400476531615f, 0.0921214998377276f ,  0.1388735102197876f ,
        0.17814598076194554f, 0.20781604753688862f,  0.2262831802628975f ,
        0.23255155323087406f, 0.2262831802628975f ,  0.20781604753688862f,
        0.17814598076194554f, 0.1388735102197876f ,  0.0921214998377276f ,
        0.04048400476531615f};

  float result = 0;
  for (size_t i = 0; i < times.size(); ++i)
  {
    result += weights[i] * f((b - a) * times[i] / 2 + (a + b) / 2);
  }
  return (b - a) * result / 2;
}

}  // namespace asdf
