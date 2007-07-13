#ifndef AUTO_VALUE_HEADER
#define AUTO_VALUE_HEADER

template< class T, T i = () >
class auto_value
{
  T t_;
public:
  typedef T data_t;
  typedef auto_value& self_t;

  // конструктор по умолчанию - главное достоинство этой тулзы
  inline auto_value() : t_(i) {}

  // конструктор с 1 параметром (в том числе - конструктор копирования)
  template< class V >
  inline auto_value(V v) : t_(v) {}

  // доступ к данному
  inline const T& data() const { return t_; }
  inline T& data() { return t_; }

  // считается, что исходный тип - простой
  inline operator T  () const { return t_; }
  inline operator T& ()       { return t_; }

  // операторы присваивания
  template< class V > inline self_t operator =   (V v) { t_ =   v; return *this; }
  template< class V > inline self_t operator +=  (V v) { t_ +=  v; return *this; }
  template< class V > inline self_t operator -=  (V v) { t_ -=  v; return *this; }
  template< class V > inline self_t operator *=  (V v) { t_ *=  v; return *this; }
  template< class V > inline self_t operator /=  (V v) { t_ /=  v; return *this; }
  template< class V > inline self_t operator %=  (V v) { t_ %=  v; return *this; }
  template< class V > inline self_t operator &=  (V v) { t_ &=  v; return *this; }
  template< class V > inline self_t operator |=  (V v) { t_ |=  v; return *this; }
  template< class V > inline self_t operator ^=  (V v) { t_ ^=  v; return *this; }
  template< class V > inline self_t operator <<= (V v) { t_ <<= v; return *this; }
  template< class V > inline self_t operator >>= (V v) { t_ >>= v; return *this; }
  inline self_t operator ++ ()    { ++t_; return *this; }
  inline data_t operator ++ (int) { return t_++; }
  inline self_t operator -- ()    { --t_; return *this; }
  inline data_t operator -- (int) { return t_--; }
};

#endif