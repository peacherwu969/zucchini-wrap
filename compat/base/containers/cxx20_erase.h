#ifndef ZUCCHINI_STANDALONE_BASE_CONTAINERS_CXX20_ERASE_H_
#define ZUCCHINI_STANDALONE_BASE_CONTAINERS_CXX20_ERASE_H_

#include <algorithm>
#include <cstddef>

namespace base {

template <typename Container, typename Value>
size_t Erase(Container& container, const Value& value) {
  const size_t old_size = container.size();
  container.erase(std::remove(container.begin(), container.end(), value),
                  container.end());
  return old_size - container.size();
}

template <typename Container, typename Predicate>
size_t EraseIf(Container& container, Predicate predicate) {
  const size_t old_size = container.size();
  container.erase(
      std::remove_if(container.begin(), container.end(), predicate),
      container.end());
  return old_size - container.size();
}

}  // namespace base

#endif
