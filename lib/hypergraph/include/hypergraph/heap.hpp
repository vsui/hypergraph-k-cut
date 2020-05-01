#pragma once

#include <cassert>
#include <list>
#include <unordered_map>
#include <vector>

#include <boost/heap/fibonacci_heap.hpp>

namespace hypergraphlib {

/* A collection of distinct values, ordered by their keys. The key for every
 * value is initially zero.
 *
 * Supports incrementing the key of a value in constant time as well as removing
 * one of the values with maximum key in time linear to the number of buckets.
 *
 * Useful for computing vertex orderings for unweighted hypergraphs. See [M'05]
 * for more details.
 */
class BucketHeap {
public:
  /* Create a bucket list with the given values and capacity (number of
   * buckets). The values should be unique.
   *
   * Time complexity: O(n), where n in the number of values.
   */
  BucketHeap(std::vector<int> values, size_t capacity);

  /* Increment the key of the value.
   *
   * Time complexity: O(1).
   */
  void increment(int value);

  /* Pop an arbitrary value with a maximum key.
   *
   * Time complexity: Expected is O(1), worst-case is O(b), where b is the
   * number of buckets.
   */
  int pop();

  /* Pop a (key, value) pair with a maximum key.
   *
   * Time complexity: Expected is O(1), worst-case is O(b), where b is the
   * number of buckets.
   */
  std::pair<size_t, int> pop_key_val();

private:
  const size_t capacity_;

  // buckets_[i] is a collection of all values with key = i
  std::vector<std::list<int>> buckets_;

  // A mapping of values to their keys
  std::unordered_map<int, size_t> val_to_keys_;

  // A mapping of values to their iterators (for fast deletion)
  std::unordered_map<int, std::list<int>::iterator> val_to_its_;

  // The current maximum key
  size_t max_key_;
};

/* Wrapper of boost::fibonacci_heap that conforms to the BucketHeap interface
 */
template<typename EdgeWeightType>
class FibonacciHeap {
public:
  FibonacciHeap(const std::vector<int> &values, [[maybe_unused]] size_t capacity) {
    for (const int v : values) {
      auto handle = heap_.push({0, v});
      const auto[it, inserted] = handles_.insert({v, handle});
      assert(inserted);
    }
  }

  /* Increment the key of the value.
   *
   * Time complexity: constant
   */
  void increment(const int value, const EdgeWeightType amount) {
    auto handle = handles_.at(value);
    auto[key, val] = *handle;
    *handle = {key + amount, val};
    heap_.increase(handle);
  }

  /* Pop an arbitrary value with a maximum key.
   *
   * Time complexity: constant
   */
  int pop() {
    return pop_key_val().second;
  }

  std::pair<EdgeWeightType, int> pop_key_val() {
    const auto pair = heap_.top();
    heap_.pop();
    return pair;
  }

private:
  // Elements are ordered by their edge weight, and keep a reference to the vertex ID
  using element_t = std::pair<EdgeWeightType, int>;
  using handle_t = typename boost::heap::fibonacci_heap<element_t>::handle_type;

  std::unordered_map<int, handle_t> handles_;

  boost::heap::fibonacci_heap<element_t> heap_;
};

}
