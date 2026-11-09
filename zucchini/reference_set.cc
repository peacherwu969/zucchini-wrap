// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zucchini/reference_set.h"

#include <algorithm>
#include <iterator>

#include "base/check_op.h"
#include "components/zucchini/target_pool.h"

namespace zucchini {

namespace {

// Returns true if |refs| is sorted by location.
bool IsReferenceListSorted(const std::vector<Reference>& refs) {
  return std::is_sorted(refs.begin(), refs.end(),
                        [](const Reference& a, const Reference& b) {
                          return a.location < b.location;
                        });
}

}  // namespace

ReferenceSet::ReferenceSet(const ReferenceTypeTraits& traits,
                           const TargetPool& target_pool)
    : traits_(traits), target_pool_(target_pool) {}
ReferenceSet::ReferenceSet(ReferenceSet&&) = default;
ReferenceSet::~ReferenceSet() = default;

void ReferenceSet::InitReferences(ReferenceReader&& ref_reader) {
  DCHECK(references_.empty());
  for (auto ref = ref_reader.GetNext(); ref.has_value();
       ref = ref_reader.GetNext()) {
    references_.push_back(*ref);
  }
  DCHECK(IsReferenceListSorted(references_));
  target_keys_.reserve(references_.size());
  for (const Reference& ref : references_)
    target_keys_.push_back(target_pool_.KeyForOffset(ref.target));
}

void ReferenceSet::InitReferences(const std::vector<Reference>& refs) {
  DCHECK(references_.empty());
  DCHECK(IsReferenceListSorted(references_));
  references_.assign(refs.begin(), refs.end());
  target_keys_.reserve(references_.size());
  for (const Reference& ref : references_)
    target_keys_.push_back(target_pool_.KeyForOffset(ref.target));
}

}  // namespace zucchini
