#include "gpu/scene/native_material_data.h"
#include "gpu/scene/native_model_materials.h"
#include "gpu/scene/native_primitive_policy_source.h"
#include "gpu/scene/native_material_alpha_source.h"
#include <iostream>
#include <stdexcept>
#include <unordered_map>
using namespace bd::gpu::scene;
namespace {
void Require(bool ok, const char *message) { if (!ok) throw std::runtime_error(message); }
}
void TestNativePrimitivePolicies() {
  std::vector<uint16_t> words{0x2000, 1, 0, 0x0901, 0x1000, 1, 3,
      0x0911, 0x3000, 1, 6, 0x0900, 0x2000, 1, 9, 0xff};
  std::vector<NativeMaterialRange> ranges;
  std::vector<PrimitivePolicyStep> steps;
  Require(DecodeMeshMaterials(words, ranges, nullptr, &steps) && steps.size() == 3,
          "load-owned alpha/winding program");
  words.clear(); words.shrink_to_fit();
  PrimitivePolicyInputs inputs;
  inputs.pass_cull = PrimitiveCull::Front;
  std::vector<NativePrimitivePolicy> values;
  auto ordinary = [](auto) { return PrimitiveTextureClass::Ordinary; };
  auto compose = [&] { return ComposePrimitivePolicies(std::span<const PrimitivePolicyStep>(steps),
      std::span<const NativeMaterialRange>(ranges), inputs, ordinary, values); };
  Require(compose() && values[0].direct && !values[0].deferred && !values[0].alpha_test &&
      !values[1].direct && values[1].deferred && values[1].alpha_test &&
      values[2].direct && !values[2].deferred && values[2].alpha_test && values[3].direct,
      "opaque, sorted, alpha-tested and reset routing survive source destruction");
  Require(values[0].cull == PrimitiveCull::Front && values[1].cull == PrimitiveCull::Back &&
      values[2].cull == PrimitiveCull::None, "all three primitive winding recipes");
  const auto plan = SummarizePrimitivePlan(values);
  Require(plan.known && plan.direct == 3 && plan.deferred == 1 && !plan.suppressed,
          "a mixed node retains both direct and deferred participants");
  Require(PrimitivePlanMatches(plan.stamp, plan) && !PrimitivePlanMatches({}, plan), "unwarmed plan cannot replay");
  inputs.pass_mode = 1;
  Require(compose() && values[1].direct && !values[1].deferred, "pass one forces direct participation");
  Require(!PrimitivePlanMatches(plan.stamp, SummarizePrimitivePlan(values)), "changed compound plan expires both halves");
  inputs.pass_mode = 2;
  Require(compose() && SummarizePrimitivePlan(values).deferred == 4, "pass two forces alpha/sorted participation");
  inputs.pass_mode = 3;
  Require(compose() && SummarizePrimitivePlan(values).direct == 2 &&
      SummarizePrimitivePlan(values).suppressed == 2 && values[1].sorted && !values[2].sorted,
      "pass three preserves sorted policy while suppressing both alpha participants");
  inputs.pass_mode = 0; inputs.phase = 1; inputs.technique = 3; inputs.wind_rejects_shadow = true;
  Require(compose() && values[0].direct && !values[1].shadow_allowed && !values[3].shadow_allowed &&
      !values[3].direct && !values[3].deferred, "zero alpha does not erase earlier shadow rejection");
  inputs.phase = 2;
  Require(compose() && SummarizePrimitivePlan(values).direct == 4, "phase two suppresses alpha commands");
  inputs.phase = 0; inputs.technique = 8;
  Require(compose() && values[1].direct && !values[1].deferred, "technique eight unsorted override");
  inputs.technique = 9;
  Require(compose() && values[2].deferred && !values[2].direct, "technique nine sorted override");
  inputs.technique = 14;
  Require(compose() && values[0].alpha_test && values[3].alpha_test && values[3].direct,
          "technique fourteen initial and reset rules");
  inputs = {}; inputs.phase = 1; inputs.shadow_modulates_colour = true;
  Require(compose() && values[2].deferred, "shadow modulation forces sorted alpha");
  inputs = {}; inputs.special_shadow_block = true;
  Require(compose() && !values[1].shadow_allowed && values[1].deferred, "special shadow veto leaves scene routing intact");
  inputs = {}; inputs.technique = 3; inputs.wind_forces_sorted = true;
  Require(compose() && values[2].deferred, "wind forces sorted alpha");
  inputs = {}; inputs.pass_cull = PrimitiveCull::Back;
  Require(compose() && values[0].cull == PrimitiveCull::Back && values[1].cull == PrimitiveCull::Front,
          "fresh pass winding reverses the appropriate primitive");
  inputs.pass_cull = PrimitiveCull::None;
  Require(compose() && values[0].cull == PrimitiveCull::None && values[1].cull == PrimitiveCull::None,
          "disabled pass culling remains disabled");

  const uint16_t texture_words[]{0x6001, 0x2000, 1, 0, 0x6002, 0x2000, 1, 3, 0xff};
  Require(DecodeMeshMaterials(texture_words, ranges, nullptr, &steps), "texture participation steps ordered with alpha");
  inputs.texture_effects = true;
  auto volume_then_ordinary = [](auto step) { return step.value == 1 ? PrimitiveTextureClass::Volume : PrimitiveTextureClass::Ordinary; };
  Require(ComposePrimitivePolicies(std::span<const PrimitivePolicyStep>(steps),
      std::span<const NativeMaterialRange>(ranges), inputs, volume_then_ordinary, values) &&
      !values[0].routing_known && values[1].routing_known && values[1].direct,
      "volume effect is unknown until an ordinary image resets it");
  auto unknown = SummarizePrimitivePlan(values);
  Require(!unknown.known && !PrimitivePlanMatches(plan.stamp, unknown) && PrimitivePlanMatches({}, unknown),
          "unconverted family cannot reuse a formerly known compound plan");
  auto retain_volume = [](auto step) { return step.value == 1 ? PrimitiveTextureClass::Volume : PrimitiveTextureClass::Unchanged; };
  Require(ComposePrimitivePolicies(std::span<const PrimitivePolicyStep>(steps),
      std::span<const NativeMaterialRange>(ranges), inputs, retain_volume, values) && !values[1].routing_known,
      "early image override preserves prior volume routing");
  inputs.texture_effects = false;
  Require(compose() && values[0].routing_known && values[0].direct, "disabled volume analysis never invents deferred work");
  const auto before = values;
  ranges[1].policy_step_end = 0;
  Require(!compose() && values == before, "backward frontier fails transactionally");
  ranges[1].policy_step_end = 999;
  Require(!compose() && values == before, "missing policy steps fail transactionally");
  ranges[1].policy_step_end = 2;
  Require(!ComposePrimitivePolicies(std::span<const PrimitivePolicyStep>(steps),
      std::span<const NativeMaterialRange>(ranges), inputs, ordinary, values, 1), "primitive output bound");
  inputs.phase = 3;
  Require(!compose(), "unknown phase is not a guessed scene route");

  constexpr uint32_t context = 10000, visual = 20000, wind = 30000;
  constexpr uint32_t cull = (uint32_t(-32036) << 16) - 5536;
  constexpr uint32_t scene = (uint32_t(-32036) << 16) - 7768;
  constexpr uint32_t effects = (uint32_t(-32101) << 16) + 22688;
  std::unordered_map<uint64_t, uint32_t> memory{{context + 16, 1}, {context + 32, 2},
      {visual + 3000, 3}, {visual + 3120, 1}, {visual + 3068, 1}, {visual + 3532, wind},
      {wind, 1}, {cull, 0x01020304}, {scene + 212, 1}, {effects, 1}};
  auto read = [&](uint64_t address) -> std::optional<uint32_t> {
    const auto it = memory.find(address);
    return it == memory.end() ? std::nullopt : std::optional(it->second);
  };
  const auto imported = ReadPrimitivePolicyInputs(context, visual, read);
  Require(imported && imported->pass_cull == PrimitiveCull::Back && imported->pass_mode == 2 &&
      imported->wind_rejects_shadow && imported->special_shadow_block && imported->texture_effects,
      "producer imports named flags and unsigned winding byte");
  Require(!ReadPrimitivePolicyInputs(UINT32_MAX - 3, visual, read), "context overflow rejected");
  memory.erase(wind);
  Require(!ReadPrimitivePolicyInputs(context, visual, read), "missing wind source refused");
  memory.clear();
  Require(imported->shadow_modulates_colour && imported->wind_rejects_shadow,
          "owned pass publication survives source destruction");
  {
    // Reset-to-default is an action, not just a material value. A repeated E000
    // must reset a default latched by an earlier opaque/sorted/suppressed draw.
    const uint16_t commands[]{0x0901,0x2000,1,0, 0x0911,0x2000,1,3,
        0xe000,0x2000,1,6, 0xe001,0x2000,1,9, 0xe000,0x2000,1,12,
        0x0901,0xe000,0x2000,1,15, 0x0911,0x2000,1,18,0xff};
    Require(DecodeMeshMaterials(commands,ranges,nullptr,&steps,[](uint16_t index) {
      return std::optional(NativeMaterialControl{true,false,false,false,index ? 192u : 0u});
    }), "cutoff reset recipe decode");
    inputs = {}; inputs.pass_mode = 3;
    Require(compose() && values[0].sorted && !values[0].deferred && !values[0].direct,
        "suppressed sorted primitive still selects sorted default");
    NativeMaterialAlphaInputs alpha{64,128};
    std::vector<uint32_t> references;
    Require(ComposeMaterialAlphaReferences(ranges,values,alpha,references) &&
        references == std::vector<uint32_t>{128,128,64,192,64,128,128},
        "ordered default latching, explicit reference and repeated reset");
    alpha.object_reference = 0;
    Require(ComposeMaterialAlphaReferences(ranges,values,alpha,references) &&
        references == std::vector<uint32_t>(7,0), "explicit zero object override is not pass default");
    inputs.pass_mode = 0;
    Require(compose() && values[0].deferred && values[5].deferred && values[1].direct,
        "ordinary pass emits sorted entries and direct siblings");
    Require(ComposeMaterialAlphaReferences(ranges,values,alpha,references) &&
        references == std::vector<uint32_t>{128,0,0,0,0,128,0},
        "deferred cutoff copy precedes direct-only object override");
    inputs.pass_mode = 3;
    Require(compose(), "restore suppressed cutoff fixture");
    alpha.object_reference.reset(); alpha.direct_reference = alpha.sorted_reference = 0;
    Require(ComposeMaterialAlphaReferences(ranges,values,alpha,references) &&
        references == std::vector<uint32_t>{0,0,0,192,0,0,0}, "zero pass defaults remain unresolved until next primitive");
    const auto before_references = references;
    ranges[3].alpha_reference.reset();
    Require(!ComposeMaterialAlphaReferences(ranges,values,alpha,references) && references == before_references,
        "unknown control fails transactionally, never a previous object's cutoff");
    Require(!ComposeMaterialAlphaReferences({},values,alpha,references) &&
        !ComposeMaterialAlphaReferences(ranges,std::span(values).first(1),alpha,references), "cutoff cardinality bound");
    const uint16_t null_control[]{0xe000,0x2000,1,0,0xff};
    Require(DecodeMeshMaterials(null_control,ranges,nullptr,nullptr,[](uint16_t) {
      return std::optional(NativeMaterialControl{});
    }) && !ranges[0].resets_alpha_reference, "null control table is no reset");
    Require(DecodeMeshMaterials(null_control,ranges) && ranges[0].resets_alpha_reference &&
        !ranges[0].alpha_reference, "unknown table produces unknown reset, not absent control");
  }
  {
    constexpr uint32_t defaults = (uint32_t(-32036) << 16) - 22280;
    constexpr uint32_t override_byte = (uint32_t(-32035) << 16) - 26711;
    memory = {{defaults+60,64},{defaults+64,128},{visual+3120,1},{visual+3124,17},
        {scene+212,0},{override_byte & ~3u,0xaa00bbcc}};
    const auto alpha = ReadMaterialAlphaInputs(visual,read);
    Require(alpha && alpha->direct_reference == 64 && alpha->sorted_reference == 128 &&
        alpha->object_reference == 17, "cutout object override precedence");
    memory[override_byte & ~3u] = 0xaa01bbcc;
    const auto shadow_alpha = ReadMaterialAlphaInputs(visual,read);
    Require(shadow_alpha && shadow_alpha->direct_reference == 64 && shadow_alpha->sorted_reference == 128 &&
        shadow_alpha->object_reference == 17,
        "light-space colour skip does not suppress shadow cutoff defaults or object override");
    memory[scene+212] = 1; memory.erase(visual+3124);
    Require(ReadMaterialAlphaInputs(visual,read) && !ReadMaterialAlphaInputs(visual,read)->object_reference,
        "blocked special override does not read unused object cutoff");
    memory.erase(override_byte & ~3u);
    Require(ReadMaterialAlphaInputs(visual,read).has_value(), "cutoff does not read the unrelated colour/light-space flag");
    memory.erase(defaults+60);
    Require(!ReadMaterialAlphaInputs(visual,read), "missing actual cutoff defaults remain refused");
    Require(!ReadMaterialAlphaInputs(visual+1,read) && !ReadMaterialAlphaInputs(UINT32_MAX-3,read),
        "cutout input alignment and complete word bounds");
    memory.clear();
    Require(alpha->object_reference == 17, "copied cutout inputs survive source destruction");
  }
  std::cout << "native primitive winding, alpha/pass participation, compound invalidation and bounds passed\n";
}
