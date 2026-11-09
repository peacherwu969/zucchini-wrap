// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/zucchini/image_index.h"

#include <algorithm>
#include <utility>

#include "components/zucchini/algorithm.h"
#include "components/zucchini/disassembler.h"

namespace zucchini {

ImageIndex::ImageIndex(ConstBufferView image)
    : image_(image),
      type_tags_(image.size(), kNoTypeTag),
      is_token_(image.size(), 1) {}

ImageIndex::ImageIndex(ImageIndex&&) = default;

ImageIndex::~ImageIndex() = default;

bool ImageIndex::Initialize(Disassembler* disasm) {
  std::vector<ReferenceGroup> ref_groups = disasm->MakeReferenceGroups();
  if (!ref_groups.empty()) {
    // Do not value-initialize: only bytes covered by references are ever read,
    // and InsertReferences() writes all of them before equivalence matching.
    reference_target_keys_.reset(new key_t[image_.size()]);
  }
  for (const auto& group : ref_groups) {
    // Build pool-to-type mapping.
    DCHECK_NE(kNoPoolTag, group.pool_tag());
    TargetPool& target_pool = target_pools_[group.pool_tag()];
    target_pool.AddType(group.type_tag());
    target_pool.InsertTargets(std::move(*group.GetReader(disasm)));
  }
  for (const auto& group : ref_groups) {
    // Find and store all references for each type, returns false on finding
    // any overlap, to signal error.
    if (!InsertReferences(group.traits(),
                          std::move(*group.GetReader(disasm)))) {
      return false;
    }
  }
  return true;
}

bool ImageIndex::InsertReferences(const ReferenceTypeTraits& traits,
                                  ReferenceReader&& ref_reader) {
  // Store ReferenceSet for current type (of |group|).
  DCHECK_NE(kNoTypeTag, traits.type_tag);
  auto result = reference_sets_.emplace(
      traits.type_tag, ReferenceSet(traits, pool(traits.pool_tag)));
  DCHECK(result.second);

  ReferenceSet& ref_set = result.first->second;
  ref_set.InitReferences(std::move(ref_reader));
  pool_tags_by_type_[traits.type_tag.value()] = traits.pool_tag;
  widths_by_type_[traits.type_tag.value()] = traits.width;

  const std::vector<Reference>& references = ref_set.references();
  const std::vector<key_t>& target_keys = ref_set.target_keys();
  DCHECK_EQ(references.size(), target_keys.size());
  for (size_t i = 0; i < references.size(); ++i) {
    const Reference& ref = references[i];
    DCHECK(RangeIsBounded(ref.location, traits.width, size()));
    auto cur_type_tag = type_tags_.begin() + ref.location;

    // Check for overlap with existing reference. If found, then invalidate.
    if (std::any_of(cur_type_tag, cur_type_tag + traits.width,
                    [](TypeTag type) { return type != kNoTypeTag; })) {
      return false;
    }
    std::fill(cur_type_tag, cur_type_tag + traits.width, traits.type_tag);
    std::fill(reference_target_keys_.get() + ref.location,
              reference_target_keys_.get() + ref.location + traits.width,
              target_keys[i]);

    // Raw bytes and the first byte of every reference are tokens. Only the
    // trailing bytes of a multi-byte reference need to be marked non-token.
    if (traits.width > 1) {
      auto token_padding = is_token_.begin() + ref.location + 1;
      std::fill(token_padding, token_padding + traits.width - 1, uint8_t{0});
    }
  }
  return true;
}

}  // namespace zucchini
