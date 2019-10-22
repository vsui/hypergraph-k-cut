#pragma once

#include <list>
#include <unordered_map>
#include <vector>

/* A collection of distinct values, ordered by their keys. The key for every
 * value is initially zero.
 *
 * Supports incrementing the key of a value in constant time as well as removing
 * one of the values with maximum key in time linear to the number of buckets.
 *
 * Useful for computing vertex orderings for unweighted hypergraphs. See [M'05]
 * for more details.
 */
class BucketList {
public:
  /* Create a bucket list with the given values and capacity (number of
   * buckets). The values should be unique.
   *
   * Time complexity: O(n), where n in the number of values.
   */
  BucketList(std::vector<int> values, const int capacity);

  /* Increment the key of the value.
   *
   * Time complexity: O(1).
   */
  void increment(const int value);

  /* Pop an arbitrary value with a maximum key.
   *
   * Time complexity: Expected is O(1), worst-case is O(b), where b is the number of buckets.
   */
  int pop();

private:
  const int capacity_;

  // buckets_[i] is a collection of all values with key = i
  std::vector<std::list<int>> buckets_;

  // A mapping of values to their keys
  std::unordered_map<int, int> val_to_keys_;

  // A mapping of values to their iterators (for fast deletion)
  std::unordered_map<int, std::list<int>::iterator> val_to_its_;

  // The current maximum key
  int max_key_;
};
