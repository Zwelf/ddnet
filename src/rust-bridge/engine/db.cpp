#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

namespace rust {
inline namespace cxxbridge1 {
// #include "rust/cxx.h"

struct unsafe_bitcopy_t;

#ifndef CXXBRIDGE1_RUST_STRING
#define CXXBRIDGE1_RUST_STRING
class String final {
public:
  String() noexcept;
  String(const String &) noexcept;
  String(String &&) noexcept;
  ~String() noexcept;

  String(const std::string &);
  String(const char *);
  String(const char *, std::size_t);
  String(const char16_t *);
  String(const char16_t *, std::size_t);

  static String lossy(const std::string &) noexcept;
  static String lossy(const char *) noexcept;
  static String lossy(const char *, std::size_t) noexcept;
  static String lossy(const char16_t *) noexcept;
  static String lossy(const char16_t *, std::size_t) noexcept;

  String &operator=(const String &) &noexcept;
  String &operator=(String &&) &noexcept;

  explicit operator std::string() const;

  const char *data() const noexcept;
  std::size_t size() const noexcept;
  std::size_t length() const noexcept;
  bool empty() const noexcept;

  const char *c_str() noexcept;

  std::size_t capacity() const noexcept;
  void reserve(size_t new_cap) noexcept;

  using iterator = char *;
  iterator begin() noexcept;
  iterator end() noexcept;

  using const_iterator = const char *;
  const_iterator begin() const noexcept;
  const_iterator end() const noexcept;
  const_iterator cbegin() const noexcept;
  const_iterator cend() const noexcept;

  bool operator==(const String &) const noexcept;
  bool operator!=(const String &) const noexcept;
  bool operator<(const String &) const noexcept;
  bool operator<=(const String &) const noexcept;
  bool operator>(const String &) const noexcept;
  bool operator>=(const String &) const noexcept;

  void swap(String &) noexcept;

  String(unsafe_bitcopy_t, const String &) noexcept;

private:
  struct lossy_t;
  String(lossy_t, const char *, std::size_t) noexcept;
  String(lossy_t, const char16_t *, std::size_t) noexcept;
  friend void swap(String &lhs, String &rhs) noexcept { lhs.swap(rhs); }

  std::array<std::uintptr_t, 3> repr;
};
#endif // CXXBRIDGE1_RUST_STRING

#ifndef CXXBRIDGE1_RUST_BOX
#define CXXBRIDGE1_RUST_BOX
template <typename T>
class Box final {
public:
  using element_type = T;
  using const_pointer =
      typename std::add_pointer<typename std::add_const<T>::type>::type;
  using pointer = typename std::add_pointer<T>::type;

  Box() = delete;
  Box(Box &&) noexcept;
  ~Box() noexcept;

  explicit Box(const T &);
  explicit Box(T &&);

  Box &operator=(Box &&) &noexcept;

  const T *operator->() const noexcept;
  const T &operator*() const noexcept;
  T *operator->() noexcept;
  T &operator*() noexcept;

  template <typename... Fields>
  static Box in_place(Fields &&...);

  void swap(Box &) noexcept;

  static Box from_raw(T *) noexcept;

  T *into_raw() noexcept;

  /* Deprecated */ using value_type = element_type;

private:
  class uninit;
  class allocation;
  Box(uninit) noexcept;
  void drop() noexcept;

  friend void swap(Box &lhs, Box &rhs) noexcept { lhs.swap(rhs); }

  T *ptr;
};

template <typename T>
class Box<T>::uninit {};

template <typename T>
class Box<T>::allocation {
  static T *alloc() noexcept;
  static void dealloc(T *) noexcept;

public:
  allocation() noexcept : ptr(alloc()) {}
  ~allocation() noexcept {
    if (this->ptr) {
      dealloc(this->ptr);
    }
  }
  T *ptr;
};

template <typename T>
Box<T>::Box(Box &&other) noexcept : ptr(other.ptr) {
  other.ptr = nullptr;
}

template <typename T>
Box<T>::Box(const T &val) {
  allocation alloc;
  ::new (alloc.ptr) T(val);
  this->ptr = alloc.ptr;
  alloc.ptr = nullptr;
}

template <typename T>
Box<T>::Box(T &&val) {
  allocation alloc;
  ::new (alloc.ptr) T(std::move(val));
  this->ptr = alloc.ptr;
  alloc.ptr = nullptr;
}

template <typename T>
Box<T>::~Box() noexcept {
  if (this->ptr) {
    this->drop();
  }
}

template <typename T>
Box<T> &Box<T>::operator=(Box &&other) &noexcept {
  if (this->ptr) {
    this->drop();
  }
  this->ptr = other.ptr;
  other.ptr = nullptr;
  return *this;
}

template <typename T>
const T *Box<T>::operator->() const noexcept {
  return this->ptr;
}

template <typename T>
const T &Box<T>::operator*() const noexcept {
  return *this->ptr;
}

template <typename T>
T *Box<T>::operator->() noexcept {
  return this->ptr;
}

template <typename T>
T &Box<T>::operator*() noexcept {
  return *this->ptr;
}

template <typename T>
template <typename... Fields>
Box<T> Box<T>::in_place(Fields &&...fields) {
  allocation alloc;
  auto ptr = alloc.ptr;
  ::new (ptr) T{std::forward<Fields>(fields)...};
  alloc.ptr = nullptr;
  return from_raw(ptr);
}

template <typename T>
void Box<T>::swap(Box &rhs) noexcept {
  using std::swap;
  swap(this->ptr, rhs.ptr);
}

template <typename T>
Box<T> Box<T>::from_raw(T *raw) noexcept {
  Box box = uninit{};
  box.ptr = raw;
  return box;
}

template <typename T>
T *Box<T>::into_raw() noexcept {
  T *raw = this->ptr;
  this->ptr = nullptr;
  return raw;
}

template <typename T>
Box<T>::Box(uninit) noexcept {}
#endif // CXXBRIDGE1_RUST_BOX

#ifndef CXXBRIDGE1_RUST_OPAQUE
#define CXXBRIDGE1_RUST_OPAQUE
class Opaque {
public:
  Opaque() = delete;
  Opaque(const Opaque &) = delete;
  ~Opaque() = delete;
};
#endif // CXXBRIDGE1_RUST_OPAQUE

#ifndef CXXBRIDGE1_IS_COMPLETE
#define CXXBRIDGE1_IS_COMPLETE
namespace detail {
namespace {
template <typename T, typename = std::size_t>
struct is_complete : std::false_type {};
template <typename T>
struct is_complete<T, decltype(sizeof(T))> : std::true_type {};
} // namespace
} // namespace detail
#endif // CXXBRIDGE1_IS_COMPLETE

#ifndef CXXBRIDGE1_LAYOUT
#define CXXBRIDGE1_LAYOUT
class layout {
  template <typename T>
  friend std::size_t size_of();
  template <typename T>
  friend std::size_t align_of();
  template <typename T>
  static typename std::enable_if<std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_size_of() {
    return T::layout::size();
  }
  template <typename T>
  static typename std::enable_if<!std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_size_of() {
    return sizeof(T);
  }
  template <typename T>
  static
      typename std::enable_if<detail::is_complete<T>::value, std::size_t>::type
      size_of() {
    return do_size_of<T>();
  }
  template <typename T>
  static typename std::enable_if<std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_align_of() {
    return T::layout::align();
  }
  template <typename T>
  static typename std::enable_if<!std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_align_of() {
    return alignof(T);
  }
  template <typename T>
  static
      typename std::enable_if<detail::is_complete<T>::value, std::size_t>::type
      align_of() {
    return do_align_of<T>();
  }
};

template <typename T>
std::size_t size_of() {
  return layout::size_of<T>();
}

template <typename T>
std::size_t align_of() {
  return layout::align_of<T>();
}
#endif // CXXBRIDGE1_LAYOUT
} // namespace cxxbridge1
} // namespace rust

enum class ScoreResult : ::std::uint8_t;
struct ResultMap;
struct ResultDirectMessage;
struct ResultTeamMessage;
struct ResultAllMessage;
struct ResultBroacast;
struct ResultPlayerInfo;
struct DbPool;

#ifndef CXXBRIDGE1_ENUM_ScoreResult
#define CXXBRIDGE1_ENUM_ScoreResult
// Result from Database request. Not all request need to result in a response.
// Some results have more data that can get requested.
enum class ScoreResult : ::std::uint8_t {
  // No result currently Exist
  None = 0,
  ShutdownDone = 1,
  RandomMap = 2,
};
#endif // CXXBRIDGE1_ENUM_ScoreResult

#ifndef CXXBRIDGE1_STRUCT_ResultMap
#define CXXBRIDGE1_STRUCT_ResultMap
struct ResultMap final {
  ::std::uint64_t player_uid;
  ::std::array<::std::uint8_t, 64> map_name;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ResultMap

#ifndef CXXBRIDGE1_STRUCT_ResultDirectMessage
#define CXXBRIDGE1_STRUCT_ResultDirectMessage
struct ResultDirectMessage final {
  ::std::uint64_t player_uid;
  ::rust::String msg;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ResultDirectMessage

#ifndef CXXBRIDGE1_STRUCT_ResultTeamMessage
#define CXXBRIDGE1_STRUCT_ResultTeamMessage
struct ResultTeamMessage final {
  ::std::int32_t team;
  ::rust::String msg;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ResultTeamMessage

#ifndef CXXBRIDGE1_STRUCT_ResultAllMessage
#define CXXBRIDGE1_STRUCT_ResultAllMessage
struct ResultAllMessage final {
  // attach `player_uid` for rate limiting
  ::std::int32_t player_uid;
  ::rust::String msg;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ResultAllMessage

#ifndef CXXBRIDGE1_STRUCT_ResultBroacast
#define CXXBRIDGE1_STRUCT_ResultBroacast
struct ResultBroacast final {
  ::rust::String msg;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ResultBroacast

#ifndef CXXBRIDGE1_STRUCT_ResultPlayerInfo
#define CXXBRIDGE1_STRUCT_ResultPlayerInfo
struct ResultPlayerInfo final {
  ::std::int32_t player_uid;
  // score is attached to player name. Discard results if changed
  ::rust::String name;
  ::std::array<float, 25> time_cp;
  bool birthday;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_ResultPlayerInfo

#ifndef CXXBRIDGE1_STRUCT_DbPool
#define CXXBRIDGE1_STRUCT_DbPool
struct DbPool final : public ::rust::Opaque {
  void worker_shutdown() noexcept;
  void worker_set_sqlite(::rust::String path) noexcept;
  void query_random_map(::std::uint64_t player_uid, ::rust::String category, ::std::int32_t stars) noexcept;
  ::ScoreResult get_next_result() noexcept;
  ::ResultMap result_map() const noexcept;
  ~DbPool() = delete;

private:
  friend ::rust::layout;
  struct layout {
    static ::std::size_t size() noexcept;
    static ::std::size_t align() noexcept;
  };
};
#endif // CXXBRIDGE1_STRUCT_DbPool

extern "C" {
::std::size_t cxxbridge1$DbPool$operator$sizeof() noexcept;
::std::size_t cxxbridge1$DbPool$operator$alignof() noexcept;

::DbPool *cxxbridge1$db_pool() noexcept;

void cxxbridge1$DbPool$worker_shutdown(::DbPool &self) noexcept;

void cxxbridge1$DbPool$worker_set_sqlite(::DbPool &self, ::rust::String *path) noexcept;

void cxxbridge1$DbPool$query_random_map(::DbPool &self, ::std::uint64_t player_uid, ::rust::String *category, ::std::int32_t stars) noexcept;

::ScoreResult cxxbridge1$DbPool$get_next_result(::DbPool &self) noexcept;

::ResultMap cxxbridge1$DbPool$result_map(::DbPool const &self) noexcept;
} // extern "C"

::std::size_t DbPool::layout::size() noexcept {
  return cxxbridge1$DbPool$operator$sizeof();
}

::std::size_t DbPool::layout::align() noexcept {
  return cxxbridge1$DbPool$operator$alignof();
}

::rust::Box<::DbPool> db_pool() noexcept {
  return ::rust::Box<::DbPool>::from_raw(cxxbridge1$db_pool());
}

void DbPool::worker_shutdown() noexcept {
  cxxbridge1$DbPool$worker_shutdown(*this);
}

void DbPool::worker_set_sqlite(::rust::String path) noexcept {
  cxxbridge1$DbPool$worker_set_sqlite(*this, &path);
}

void DbPool::query_random_map(::std::uint64_t player_uid, ::rust::String category, ::std::int32_t stars) noexcept {
  cxxbridge1$DbPool$query_random_map(*this, player_uid, &category, stars);
}

::ScoreResult DbPool::get_next_result() noexcept {
  return cxxbridge1$DbPool$get_next_result(*this);
}

::ResultMap DbPool::result_map() const noexcept {
  return cxxbridge1$DbPool$result_map(*this);
}

extern "C" {
::DbPool *cxxbridge1$box$DbPool$alloc() noexcept;
void cxxbridge1$box$DbPool$dealloc(::DbPool *) noexcept;
void cxxbridge1$box$DbPool$drop(::rust::Box<::DbPool> *ptr) noexcept;
} // extern "C"

namespace rust {
inline namespace cxxbridge1 {
template <>
::DbPool *Box<::DbPool>::allocation::alloc() noexcept {
  return cxxbridge1$box$DbPool$alloc();
}
template <>
void Box<::DbPool>::allocation::dealloc(::DbPool *ptr) noexcept {
  cxxbridge1$box$DbPool$dealloc(ptr);
}
template <>
void Box<::DbPool>::drop() noexcept {
  cxxbridge1$box$DbPool$drop(this);
}
} // namespace cxxbridge1
} // namespace rust
