#include "bucket_list.h"

BucketList::BucketList(std::vector<int> values, const size_t capacity)
    : capacity_(capacity), buckets_(capacity), max_key_(0) {
  for (const int value : values) {
    buckets_.front().emplace_front(value);
    val_to_keys_.insert({value, 0});
    val_to_its_.insert({value, std::begin(buckets_.front())});
  }
}

void BucketList::increment(const int value) {
  const auto old_key = val_to_keys_.at(value);
  const auto new_key = ++val_to_keys_.at(value);
  assert(new_key < capacity_);
  buckets_.at(old_key).erase(val_to_its_.at(value));
  if (new_key > max_key_) {
    max_key_ = new_key;
  }
  buckets_.at(new_key).emplace_front(value);
  val_to_its_.at(value) = std::begin(buckets_.at(new_key));
}

int BucketList::pop() {
  while (buckets_.at(max_key_).empty()) {
    --max_key_;
  }
  const int value = buckets_.at(max_key_).front();
  buckets_.at(max_key_).pop_front();
  val_to_keys_.erase(value);
  val_to_its_.erase(value);
  return value;
}
