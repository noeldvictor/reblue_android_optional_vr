// Address/SDK-independent post-root order and ping-pong lifetime checks.
#include "gpu/post_sequence.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <array>
#include <cassert>
#include <limits>
using namespace bd::gpu;

int main() {
  for (uint32_t count = 0; count <= PostSequence::kCapacity; ++count) {
    const auto sequence = MakePostSequence(count);
    assert(sequence && sequence->count == count);
    assert(sequence->target_count == (count < 2 ? count : 2));
    // Distinct eyes and a non-commutative stage operation detect stale sources,
    // reordering and unwritten scratch reads, across repeated target reuse.
    std::array<std::array<uint64_t, 2>, 2> targets{};
    std::array<uint64_t, 2> reference{7, 13}, input = reference;
    uint32_t previous_output = 2;
    for (uint32_t stage = 0; stage < count; ++stage) {
      const auto output = sequence->Output(stage);
      assert(output < sequence->target_count && output != previous_output);
      if (stage) input = targets[previous_output];
      for (uint32_t eye = 0; eye < 2; ++eye) {
        reference[eye] = reference[eye] * 17 + stage;
        targets[output][eye] = input[eye] * 17 + stage;
      }
      assert(targets[output] == reference);
      previous_output = output;
    }
  }
  assert(!MakePostSequence(PostSequence::kCapacity + 1));
  assert(!MakePostSequence(std::numeric_limits<uint32_t>::max()));
  // Exposure belongs to the incoming scene only, even when two targets are
  // reused repeatedly. Already-materialized and aliased HDR inputs must agree.
  for (uint32_t count = 1; count <= PostSequence::kCapacity; ++count) {
    const auto sequence = MakePostSequence(count);
    std::array<std::array<double, 2>, 2> targets{};
    std::array<double, 2> input{8, 12}, reference{2, 3};
    for (uint32_t stage = 0; stage < count; ++stage) {
      const auto output = sequence->Output(stage);
      if (stage) input = targets[sequence->Output(stage - 1)];
      const auto exposure = sequence->Exposure(stage, .25f);
      assert(exposure == (stage == 0 ? .25f : 1.0f));
      assert(sequence->Exposure(stage, 1.0f) == 1.0f);
      for (uint32_t eye = 0; eye < 2; ++eye) {
        targets[output][eye] = input[eye] * exposure * .5 + double(stage);
        reference[eye] = reference[eye] * .5 + double(stage);
      }
      assert(targets[output] == reference);
    }
  }
}
