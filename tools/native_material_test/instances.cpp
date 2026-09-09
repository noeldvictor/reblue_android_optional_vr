#include "gpu/scene/native_instance.h"
#include "gpu/scene/native_instance_source.h"
#include "gpu/scene/native_object_primitive.h"
#include "gpu/scene/native_skeleton_source.h"
#include "gpu/scene/native_material_texture_source.h"
#include <barrier>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <unordered_map>

using namespace bd::gpu::scene;
void TestAnimationClips();
namespace {
void Require(bool valid, const char *message) {
  if (!valid) throw std::runtime_error(message);
}
RenderMatrix World(float x) {
  RenderMatrix m{};
  m[0] = m[5] = m[10] = m[15] = 1; m[12] = x;
  return m;
}
void TestEyeMaterialOwnership() {
  NativeEyeControl control{{.5f,-.5f},{.4f,.6f},{.1f,.2f},{.9f,.8f}};
  const auto values=EvaluateNativeEyeUV(control);
  Require(values && std::abs((*values)[0][0]-.25f)<1e-7f &&
      std::abs((*values)[1][0]-.65f)<1e-7f && std::abs((*values)[0][1]-.4f)<1e-7f &&
      (*values)[0][1] == (*values)[1][1], "asymmetric authored limits and mirrored horizontal gaze");
  control.gaze={-2,2};
  const auto extrapolated=EvaluateNativeEyeUV(control);
  Require(extrapolated && (*extrapolated)[0][0]>control.maximum[0] &&
      (*extrapolated)[1][0]<control.minimum[0], "authored gaze is not clamped");
  control.gaze={-0.0f,0.0f};
  Require(EvaluateNativeEyeUV(control)->at(0) == control.origin, "signed zero uses origin");
  control.gaze[0]=std::numeric_limits<float>::infinity();
  Require(!EvaluateNativeEyeUV(control), "nonfinite gaze refuses complete publication");
  control.gaze={1,1}; control.origin[0]=-std::numeric_limits<float>::max();
  control.maximum[0]=std::numeric_limits<float>::max();
  Require(!EvaluateNativeEyeUV(control), "overflowing rounded extent is not published");
  control={}; control.gaze={1,0}; control.maximum[0]=std::numeric_limits<float>::denorm_min();
  const auto tiny=EvaluateNativeEyeUV(control);
  Require(tiny && std::bit_cast<uint32_t>((*tiny)[1][0]) == 1,
      "authored subnormal UV is retained in the original non-flushing mode");

  constexpr uint32_t visual=0x10000, table=0x20000, gaze=0x30000;
  std::unordered_map<uint64_t,uint32_t> words{
      {visual+3560,table},{visual+3564,3},{visual+3000,0},{visual+3128,0},
      {visual+3440,1},{visual+3572,0},{visual+3680,0},{visual+3712,~0u}};
  const auto number=[&](uint64_t address,float value) { words[address]=std::bit_cast<uint32_t>(value); };
  const auto read=[&](uint64_t address)->std::optional<uint32_t> {
    if ((address & 3) || address>UINT32_MAX-3) return {};
    const auto it=words.find(address); return it == words.end() ? std::nullopt : std::optional(it->second);
  };
  for (uint32_t axis=0; axis<2; ++axis) {
    number(gaze+axis*4,axis == 0 ? .5f : -.5f);
    number(table+60+axis*4,0); number(table+68+axis*4,1); number(table+76+axis*4,.5f);
  }
  for (uint32_t n=0; n<4; ++n) number(visual+3444+n*4,0);
  for (uint32_t i=0; i<3; ++i) {
    const uint64_t record=table+i*152;
    words[record+4]=i+1; words[record+8]=0; words[record+20]=i<2 ? 2 : 1;
    words[record+24]=0; number(record+28,9); number(record+32,8);
  }
  auto publication=material_uv_source::ReadEyeControl(visual,gaze,read);
  Require(publication && publication->material.count == 3 && publication->material.entries.size() == 2 && publication->binding.count == 3 &&
      publication->output[0] == std::array<float,2>{.25f,.25f} &&
      publication->output[1] == std::array<float,2>{.75f,.25f},
      "first record's limits drive both eyes; third record is not an eye output");
  NativeInstanceRegistry registry;
  instance_source::Binding binding{registry.Create(4),4,{}};
  const auto resident=registry.Stats().bytes;
  auto publish=[&] {
    binding.material_uv=publication->binding;
    return registry.PublishMaterialUVs(binding.instance,4,publication->material);
  };
  auto copy=[&] {
    for (uint32_t i=0; i<std::min(publication->material.count,2u); ++i)
      for (uint32_t axis=0; axis<2; ++axis) number(table+i*152+28+axis*4,publication->output[i][axis]);
  };
  Require(publish() && !instance_source::ReadMaterialUVs(registry,binding,visual,4,read),
      "producer cannot expose UVs before the actual caller copy");
  copy();
  Require(!instance_source::ReadMaterialUVs(registry,binding,visual,4,read), "invalidated output never resurrects from matching bytes");
  Require(publish(), "republish after incomplete caller");
  auto owned=instance_source::ReadMaterialUVs(registry,binding,visual,4,read);
  const auto material_bytes=registry.Stats().bytes-resident;
  Require(owned && material_bytes > sizeof(NativeMaterialUVs), "shared material lease charges actual retained storage");
  Require(publish() && registry.ReadMaterialUVs(binding.instance,4) == owned &&
      registry.Stats().bytes == resident+material_bytes, "identical publication reuses the same immutable lease");
  auto input_read=[&](uint64_t address) {
    for (uint32_t i=0; i<2; ++i)
      Require(address != table+i*152+28 && address != table+i*152+32, "native consumer must not reimport exported eye UV");
    return read(address);
  };
  auto capture=[](uint32_t image) { return MaterialImageSelection<uint32_t>{MaterialImageAction::Bind,image}; };
  const auto inputs=ReadMaterialTextureInputs<uint32_t>(visual,input_read,capture,&*owned);
  Require(inputs && inputs->overrides.size() == 3 && inputs->overrides[0].native_eye &&
      inputs->overrides[1].native_eye && !inputs->overrides[2].native_eye, "only producer-owned slots bypass UV imports");
  std::array<MaterialImageAssignment,4> assignments{{{MaterialImageSource::Table,0,1},
      {MaterialImageSource::Table,1,2},{MaterialImageSource::Table,0,3},{MaterialImageSource::Table,1,4}}};
  std::array<NativeMaterialRange,4> ranges{};
  for (size_t i=0; i<ranges.size(); ++i) ranges[i].texture_assignment_end=i+1;
  std::vector<MaterialTextureValues<uint32_t>> composed;
  Require(ComposeMaterialTextures(std::span<const MaterialImageAssignment>(assignments),
      std::span<const NativeMaterialRange>(ranges),*inputs,capture,composed) &&
      composed[0].uv[0] == .25f && composed[0].native_eye_uv_mask == 1 &&
      composed[1].uv == std::array<float,4>{.25f,.25f,.75f,.25f} && composed[1].native_eye_uv_mask == 3 &&
      composed[2].uv[0] == 9 && composed[2].native_eye_uv_mask == 2 && composed[3].native_eye_uv_mask == 0,
      "actual material ordering selects, preserves, replaces and resets owned eye provenance by channel");
  Require(composed[0].native_animated_uv_mask == 1 && composed[1].native_animated_uv_mask == 3 &&
      composed[2].native_animated_uv_mask == 2 && composed[3].native_animated_uv_mask == 0,
      "shared animated provenance follows the actual material selection and reset");
  number(table+28,.125f);
  Require(!instance_source::ReadMaterialUVs(registry,binding,visual,4,read) &&
      ReadMaterialTextureInputs<uint32_t>(visual,read,capture)->overrides[0].uv->at(0) == .125f,
      "unknown late writer invalidates native eye publication and explicit legacy route reads current value");
  copy(); Require(!instance_source::ReadMaterialUVs(registry,binding,visual,4,read), "late write cannot resurrect an older gaze");
  for (uint32_t offset : {4u,8u,20u}) {
    Require(publish(), "republish binding"); const auto saved=words[table+offset]; words[table+offset]=0;
    if (saved == 0) words[table+offset]=1;
    Require(!instance_source::ReadMaterialUVs(registry,binding,visual,4,read), "selector/channel/enable rebind invalidates native values");
    words[table+offset]=saved;
  }
  Require(publish(), "republish before table replacement"); words[visual+3560]=table+152;
  Require(!instance_source::ReadMaterialUVs(registry,binding,visual,4,read), "table replacement invalidates even at same instance");
  words[visual+3560]=table; Require(publish(), "republish before count replacement"); words[visual+3564]=2;
  Require(!instance_source::ReadMaterialUVs(registry,binding,visual,4,read), "table count change invalidates publication");
  words[visual+3564]=1;
  const auto one=material_uv_source::ReadEyeControl(visual,gaze,read);
  Require(one && one->material.count == 1 && one->output[1][0] == .75f, "one record still computes four scratch floats");
  for (uint32_t count : {0u,~0u,257u}) {
    words[visual+3564]=count;
    Require(!material_uv_source::ReadEyeControl(visual,gaze,read), "empty/signed negative/oversized table refuses native ownership");
  }
  words[visual+3564]=2; words[table+76]=0x7fc00000;
  Require(!material_uv_source::ReadEyeControl(visual,gaze,read), "nonfinite authored limits refused");
  number(table+76,.5f); words.erase(gaze+4);
  Require(!material_uv_source::ReadEyeControl(visual,gaze,read) &&
      !material_uv_source::ReadEyeControl(UINT32_MAX-100,gaze,read), "missing input and visual overflow refuse safely");
  Require(publish() && !registry.ReadMaterialUVs(binding.instance,5), "wrong model generation cannot borrow eye material");
  auto bad=publication->material; bad.count=257;
  Require(!registry.PublishMaterialUVs(binding.instance,4,bad) && !registry.ReadMaterialUVs(binding.instance,4),
      "refused replacement clears old owned values");
  Require(publish(), "republish before retirement");
  registry.Retire(binding.instance);
  const auto replacement=registry.Create(5);
  Require(replacement != binding.instance && !registry.ReadMaterialUVs(binding.instance,4) && !registry.ReadMaterialUVs(replacement,5),
      "reload cannot inherit retired eye state");
  words.clear(); publication.reset();
  Require(owned->entries[0].uv[0] == .25f && inputs->overrides[1].uv->at(0) == .75f,
      "immutable material snapshots survive source destruction and instance reload");
  registry.Retire(replacement);
  Require(registry.Stats().bytes == material_bytes, "retired instance cannot uncharge a material lease still in use");
  owned.reset(); Require(registry.Stats().bytes == 0, "last material lease releases all retained bytes");
}
void TestMaterialUVBudgets() {
  NativeMaterialUVs material{{{0,1,0,{0,0},true,NativeMaterialUVOrigin::Effect},
      {255,2,1,{1,2},true,NativeMaterialUVOrigin::Eye}},256};
  Require(material.Valid() && material.Find(255) && !material.Find(254) && material.HasEyes(),
      "sparse material slots preserve the full bounded table identity");
  auto bad=material; bad.entries[1].slot=0;
  Require(!bad.Valid(), "duplicate slots refuse");
  bad=material; std::swap(bad.entries[0],bad.entries[1]); Require(!bad.Valid(), "unsorted slots refuse");
  bad=material; bad.entries[1].slot=256; Require(!bad.Valid(), "out-of-table slots refuse");
  bad=material; bad.entries[0].uv[0]=NAN; Require(!bad.Valid(), "nonfinite owned UV refuses");
  bad=material; bad.entries[0].origin=NativeMaterialUVOrigin(2); Require(!bad.Valid(), "unknown writer provenance refuses");
  bad=material; bad.entries[0].uv[0]=-0.0f; Require(!bad.Same(material), "signed-zero replacement is not elided");
  NativeInstanceRegistry measured;
  const auto id=measured.Create(7), base=measured.Stats().bytes;
  Require(measured.PublishMaterialUVs(id,7,material), "measure material lease in existing registry");
  const size_t bytes=measured.Stats().bytes-base;
  NativeInstanceRegistry tight(base+bytes);
  const auto limited=tight.Create(7);
  Require(tight.PublishMaterialUVs(limited,7,material), "one lease fits exact accounted budget");
  auto pinned=tight.ReadMaterialUVs(limited,7);
  Require(tight.PublishMaterialUVs(limited,7,material) && tight.ReadMaterialUVs(limited,7) == pinned,
      "unchanged data needs no replacement allocation even at full budget");
  auto changed=material; changed.entries[0].uv[0]=3;
  Require(!tight.PublishMaterialUVs(limited,7,changed) && !tight.ReadMaterialUVs(limited,7) &&
      tight.Stats().bytes == base+bytes && pinned->entries[0].uv[0] == 0,
      "pinned replacement backpressure clears visibility without changing old lease or exceeding budget");
  pinned.reset(); Require(tight.Stats().bytes == base && tight.PublishMaterialUVs(limited,7,changed),
      "released overlap permits a fresh publication without replaying producers");
  pinned=tight.ReadMaterialUVs(limited,7); tight.Retire(limited);
  Require(tight.Stats().bytes == bytes && pinned->entries[0].uv[0] == 3, "retirement retains pinned material accounting");
  pinned.reset(); Require(tight.Stats().bytes == 0, "last lease returns tight registry to zero");
  std::shared_ptr<const NativeMaterialUVs> survives;
  {
    NativeInstanceRegistry local;
    const auto transient=local.Create(8);
    Require(local.PublishMaterialUVs(transient,8,material), "publish before registry destruction");
    survives=local.ReadMaterialUVs(transient,8);
  }
  Require(survives && survives->Same(material), "material and accounting survive registry destruction");
}
void TestMaterialUVProgramOwnership() {
  constexpr uint32_t visual=0x10000, table=0x20000;
  std::unordered_map<uint64_t,uint32_t> words{{visual+3560,table},{visual+3564,2},
      {0x8208EA64,std::bit_cast<uint32_t>(.017453292f)}};
  for (uint32_t offset=0; offset<304; offset+=4) words[table+offset]=0;
  const auto read=[&](uint64_t address)->std::optional<uint32_t> {
    const auto it=words.find(address); return it == words.end() ? std::nullopt : std::optional(it->second);
  };
  words[table+4]=3; words[table+20]=2; words[table+36]=std::bit_cast<uint32_t>(.25f);
  words[table+152+4]=7; words[table+152+20]=1; words[table+152+120]=0x41000000;
  words[table+152+12]=1; words[table+152+16]=1;
  const auto publication=material_uv_source::ReadProgram(visual,read);
  Require(publication && publication->program.Valid() && publication->program.slots[0].selector == 3 &&
      publication->program.slots[1].joint == 1 && publication->program.slots[1].mode == NativeEffectUVMode::Rotation,
      "binding decodes selectors, enabled modes and dense joint identity");
  NativeInstanceRegistry registry;
  instance_source::Binding binding{registry.Create(11),11,{}};
  const auto base=registry.Stats().bytes;
  const auto publish=[&] {
    binding.material_program=publication->binding;
    return registry.PublishMaterialUVProgram(binding.instance,11,publication->program);
  };
  Require(publish(), "program enters same bounded instance registry");
  auto pinned=instance_source::ReadMaterialUVProgram(registry,binding,visual,11,read);
  const auto bytes=registry.Stats().bytes-base;
  Require(pinned && bytes > sizeof(NativeMaterialUVProgram) && publish() &&
      registry.ReadMaterialUVProgram(binding.instance,11) == pinned && registry.Stats().bytes == base+bytes,
      "identical binding reuses immutable descriptor lease and accounted bytes");
  NativeMaterialUVs uv{{{0,3,0,{0,0},true,NativeMaterialUVOrigin::Eye}},2};
  for (uint32_t offset : {4u,8u,20u,36u,40u,44u,48u,52u,56u,60u,64u,68u,72u,76u,80u,120u}) {
    Require(publish() && registry.PublishMaterialUVs(binding.instance,11,uv), "republish program and UVs before late write");
    const auto before=words[table+offset]; words[table+offset]=offset == 120 ? 0x41000000 : before+1;
    // Nonzero mode changes 2->3 are semantically identical enables.
    if (offset == 20) words[table+offset]=0;
    Require(!instance_source::ReadMaterialUVProgram(registry,binding,visual,11,read) &&
        !registry.ReadMaterialUVs(binding.instance,11), "late descriptor write retires both program and affected UV visibility");
    words[table+offset]=before;
    Require(!instance_source::ReadMaterialUVProgram(registry,binding,visual,11,read), "matching old descriptor words cannot resurrect a binding");
  }
  Require(publish(), "republish before joint rebind"); words[table+152+12]=2;
  Require(!instance_source::ReadMaterialUVProgram(registry,binding,visual,11,read), "joint identity change invalidates descriptors");
  words[table+152+12]=1; Require(publish(), "republish before table identity change"); words[visual+3560]=table+152;
  Require(!instance_source::ReadMaterialUVProgram(registry,binding,visual,11,read), "table replacement invalidates native program");
  words[visual+3560]=table; Require(publish(), "republish before unit change"); words[0x8208EA64]^=1;
  Require(!instance_source::ReadMaterialUVProgram(registry,binding,visual,11,read), "unit conversion late write cannot change native evaluation silently");
  words[0x8208EA64]^=1;
  Require(publish() && !registry.ReadMaterialUVProgram(binding.instance,12) &&
      !instance_source::ReadMaterialUVProgram(registry,binding,visual,12,read) &&
      !registry.ReadMaterialUVProgram(binding.instance,11), "wrong generation invalidates descriptor visibility without borrowing another model");
  Require(publish(), "republish before count change"); words[visual+3564]=1;
  Require(!instance_source::ReadMaterialUVProgram(registry,binding,visual,11,read), "descriptor table count changes invalidate the whole binding");
  words[visual+3564]=2;
  const auto unit=words[0x8208EA64]; words.erase(0x8208EA64);
  Require(!material_uv_source::ReadProgram(visual,read) &&
      !material_uv_source::ReadProgram(UINT32_MAX-100,read), "missing unit and overflowing visual refuse complete descriptor import");
  words[0x8208EA64]=unit;
  auto dormant=publication->program; dormant.slots[1].rate[0]=NAN;
  Require(dormant.Valid(), "dormant nonfinite scroll payload remains admissible for a channel driver");
  auto bad=publication->program; bad.slots.resize(257); Require(!bad.Valid(), "program slot capacity is bounded");
  bad=publication->program; bad.slots[1].joint=kMaxNativeJoints; Require(!bad.Valid(), "joint identity is bounded");
  bad=publication->program; bad.slots[0].rate[1]=-0.0f; Require(!bad.Same(publication->program), "binding identity preserves signed zero");
  NativeInstanceRegistry tight(base+bytes);
  const auto id=tight.Create(11);
  Require(tight.PublishMaterialUVProgram(id,11,publication->program), "program fits exact accounted budget");
  auto held=tight.ReadMaterialUVProgram(id,11);
  Require(!tight.PublishMaterialUVs(id,11,uv) && tight.ReadMaterialUVProgram(id,11) == held,
      "program and UV allocations compete under the same instance budget");
  auto changed=publication->program; changed.slots[0].rate[0]=2;
  Require(!tight.PublishMaterialUVProgram(id,11,changed) && !tight.ReadMaterialUVProgram(id,11),
      "pinned descriptor replacement refuses without stale visibility");
  held.reset(); Require(tight.PublishMaterialUVProgram(id,11,changed), "released descriptor overlap permits republish");
  tight.Retire(id); Require(tight.Stats().bytes == 0, "tight descriptor budget returns to zero");
  registry.Retire(binding.instance);
  const auto reloaded=registry.Create(12);
  Require(reloaded != binding.instance && !registry.ReadMaterialUVProgram(binding.instance,11) &&
      !registry.ReadMaterialUVProgram(reloaded,12), "replacement model generation cannot inherit retired descriptors");
  registry.Retire(reloaded); words.clear();
  Require(pinned->slots[0].rate[0] == .25f && registry.Stats().bytes == bytes,
      "native descriptors survive source/instance destruction with pinned residency charged");
  pinned.reset(); Require(registry.Stats().bytes == 0, "last descriptor lease releases accounted storage");
}
void TestMaterialImageOwnership() {
  Require(NativeImageWindowActive({2,3,5,0,false},2) == true &&
      NativeImageWindowActive({2,3,5,0,false},5) == true &&
      NativeImageWindowActive({2,3,5,0,false},6) == false, "inclusive image window endpoints");
  Require(NativeImageWindowActive({2,0,5,0,false},999) == true &&
      NativeImageWindowActive({2,0,5,0,false},1) == false, "zero-duration hold starts at authored time");
  Require(NativeImageWindowActive({2,3,5,2,true},8) == true &&
      NativeImageWindowActive({2,3,5,2,true},11) == false &&
      NativeImageWindowActive({-2,3,0,2,true},4) == true, "integer repeat and negative-start loop");
  Require(!NativeImageWindowActive({0,.25f,-1,1,true},5) &&
      !NativeImageWindowActive({NAN,1,1,0,false},1), "trapping period and nonfinite windows refuse");
  std::unordered_map<uint64_t,uint32_t> words;
  const uint32_t visual=10000, table=20000, catalog=30000, owner=31000, windows=32000, cache=33000;
  for (uint32_t n=0; n<3752; n+=4) words[visual+n]=0;
  for (uint32_t n=0; n<4*152; n+=4) words[table+n]=0;
  words[visual+3560]=table; words[visual+3564]=4; words[visual+2264]=catalog;
  words[visual+2212]=7; words[visual+2216]=8; words[visual+2224]=std::bit_cast<uint32_t>(4.75f);
  words[catalog+4]=0; words[catalog+8]=7; words[catalog+12]=0; words[catalog+16]=owner;
  words[owner+316]=6; words[owner+80]=windows; words[owner+84]=windows+8;
  words[owner+324]=cache; words[owner+328]=cache+4; words[owner+332]=cache+4; words[cache]=34000;
  words[windows]=34000; words[windows+4]=34400; words[0x82055230]=0;
  words[0x8208EA64]=std::bit_cast<uint32_t>(.017453292f);
  for (uint32_t n=0; n<2; ++n) {
    const uint32_t window=34000+n*400, texture=36000+n*100, holder=37000+n*100;
    words[window+60]=std::bit_cast<uint32_t>(float(n));
    words[window+64]=std::bit_cast<uint32_t>(5.0f);
    words[window+68]=std::bit_cast<uint32_t>(5.0f);
    words[window+124]=0; words[window+128]=0; words[window+212]=0;
    words[window+260]=texture; words[texture+4]=holder; words[holder+24]=41000+n*1000;
  }
  words[table+4]=3; words[table+24]=1; words[table+84]=43000;
  words[table+152]=uint32_t(-1); words[table+152+4]=9; words[table+152+24]=1; words[table+152+84]=43000;
  words[table+3*152+4]=3; words[table+3*152+24]=1; words[table+3*152+84]=43000;
  auto read=[&](uint64_t address)->std::optional<uint32_t> {
    const auto it=words.find(address); return it == words.end() ? std::nullopt : std::optional(it->second);
  };
  // Opaque GPU leases: CPU tests compare/pin identities, never pretend to render.
  auto token_a=std::make_shared<int>(1), token_b=std::make_shared<int>(2), token_c=std::make_shared<int>(3);
  NativeTextureBinding a{std::shared_ptr<const NativeTextureGpu>(token_a,reinterpret_cast<const NativeTextureGpu *>(token_a.get()))};
  NativeTextureBinding b{std::shared_ptr<const NativeTextureGpu>(token_b,reinterpret_cast<const NativeTextureGpu *>(token_b.get()))};
  NativeTextureBinding c{std::shared_ptr<const NativeTextureGpu>(token_c,reinterpret_cast<const NativeTextureGpu *>(token_c.get()))};
  size_t captures=0;
  auto capture=[&](uint32_t image) {
    ++captures;
    if (image == 41000) return MaterialImageSelection<NativeTextureBinding>{MaterialImageAction::Bind,a};
    if (image == 42000) return MaterialImageSelection<NativeTextureBinding>{MaterialImageAction::Bind,b};
    if (image == 43000) return MaterialImageSelection<NativeTextureBinding>{MaterialImageAction::Bind,c};
    return MaterialImageSelection<NativeTextureBinding>{};
  };
  auto bound=material_uv_source::ReadProgram(visual,read);
  Require(bound.has_value(), "bind image enable/animation slot alongside UV descriptors");
  auto producer_read=[&](uint64_t address) {
    if (address >= table && address < table+4*152 && (address-table)%152 != 84)
      throw std::runtime_error("image producer reimported bind-owned descriptors");
    return read(address);
  };
  auto update=material_image_source::Prepare(visual,bound->program,producer_read,capture);
  Require(update && update->selected == 2 && update->held == 0 && captures == 3 && words[cache] == 34000 &&
      words[table+84] == 43000, "complete immutable image transaction before source mutation");
  Require(update->images.entries[0].image.image == b && update->images.entries[1].image.image == c &&
      update->images.entries[3].image.image == b, "last active window wins, repeated catalog sees staged scratch");
  auto write=[&](uint32_t address,uint32_t value) { words[address]=value; };
  update->Publish(write);
  Require(words[cache] == 34400 && words[table+84] == 42000 && update->Matches(read) &&
      material_image_source::Matches(visual,update->binding,update->images,read), "exact source cache/table comparison");
  NativeInstanceRegistry registry;
  const auto id=registry.Create(11);
  instance_source::Binding association{id,11,{}}; association.material_images=update->binding;
  Require(registry.PublishMaterialImages(id,11,update->images), "image publication into existing instance registry");
  auto lease=instance_source::ReadMaterialImages(registry,association,visual,11,read);
  const auto bytes=registry.Stats().bytes;
  Require(lease && registry.PublishMaterialImages(id,11,update->images) &&
      registry.ReadMaterialImages(id,11) == lease && registry.Stats().bytes == bytes, "identical immutable image lease reuse");
  constexpr uint32_t defaults=(uint32_t(-32035)<<16)-25620;
  for (uint32_t n=0; n<4; ++n) words[defaults+n*4]=0;
  auto material_read=[&](uint64_t address) {
    if (address >= table && address < table+4*152 && (address-table)%152 != 20)
      throw std::runtime_error("material reimported image pointer or descriptor");
    return read(address);
  };
  auto forbidden_capture=[](uint32_t)->MaterialImageSelection<NativeTextureBinding> {
    throw std::runtime_error("native material recaptured source texture");
  };
  auto inputs=ReadMaterialTextureInputs<NativeTextureBinding>(visual,material_read,forbidden_capture,nullptr,lease.get());
  Require(inputs.has_value(), "real material importer consumes owned image leases without source-image reads");
  std::array<MaterialImageAssignment,3> commands{{{MaterialImageSource::Table,0,3},
      {MaterialImageSource::Table,1,9},{MaterialImageSource::Table,0,99}}};
  struct Range { uint32_t texture_assignment_end; };
  std::array<Range,3> ranges{{{1},{2},{3}}};
  auto lookup=[](uint32_t) { return MaterialImageSelection<NativeTextureBinding>{MaterialImageAction::Keep}; };
  std::vector<MaterialTextureValues<NativeTextureBinding>> values;
  Require(ComposeMaterialTextures<NativeTextureBinding,Range>(commands,ranges,*inputs,lookup,values) &&
      values[0].images[0] == b && values[1].images[1] == c && values[2].images[0] == b &&
      values[2].native_image_mask == 3, "actual composition retains native images across Keep commands");
  inputs->overrides[0].image={}; inputs->overrides[2].image={};
  Require(ComposeMaterialTextures<NativeTextureBinding,Range>(commands,ranges,*inputs,lookup,values) &&
      !(values[0].image_mask&1) && !(values[0].native_image_mask&1), "unknown nonnull replacement invalidates ownership");
  words[visual+2224]=std::bit_cast<uint32_t>(100.0f);
  const auto hold=material_image_source::Prepare(visual,bound->program,producer_read,capture);
  Require(hold && hold->selected == 0 && hold->held == 2 && hold->images.entries[0].image.image == b,
      "no active key preserves cached selection");
  words[owner+316]=5;
  const auto not_ready=material_image_source::Prepare(visual,bound->program,producer_read,capture);
  Require(not_ready && not_ready->selected == 0 && not_ready->held == 0 &&
      not_ready->images.entries[0].image.image == b, "not-ready clears scratch, preserves outgoing material image");
  not_ready->Publish(write); Require(words[owner+328] == cache, "not-ready cache clear exported");
  words[owner+316]=6; words[visual+2224]=std::bit_cast<uint32_t>(4.75f);
  const auto before=words; captures=0;
  words[34400+212]=1;
  Require(!material_image_source::Prepare(visual,bound->program,producer_read,capture) && captures == 0,
      "procedural selected key refuses before mutation/capture");
  words=before; words[owner+332]=cache;
  Require(!material_image_source::Prepare(visual,bound->program,producer_read,capture) && captures == 0,
      "required source scratch allocation retains original whole call");
  words=before; words[catalog+8]=99; words[catalog+4]=catalog;
  Require(!material_image_source::Prepare(visual,bound->program,producer_read,capture), "cyclic catalog bounded");
  words=before; words[owner+84]=windows+4097*4;
  Require(!material_image_source::Prepare(visual,bound->program,producer_read,capture), "oversized window catalog refuses");
  words=before; words[0x82055230]=std::bit_cast<uint32_t>(1.0f);
  Require(!material_image_source::Prepare(visual,bound->program,producer_read,capture), "changed comparison constant refuses");
  words=before;
  for (uint32_t field : {4u,8u,24u,84u}) {
    association.material_images=update->binding; registry.PublishMaterialImages(id,11,update->images);
    words[table+field]^=1;
    Require(!instance_source::ReadMaterialImages(registry,association,visual,11,read), "late image descriptor/source invalidates");
    words[table+field]^=1;
    Require(!instance_source::ReadMaterialImages(registry,association,visual,11,read), "old image publication cannot resurrect");
  }
  registry.PublishMaterialImages(id,11,update->images); association.material_images=update->binding;
  Require(!instance_source::ReadMaterialImages(registry,association,visual,12,read) &&
      !registry.ReadMaterialImages(id,11), "wrong generation invalidates source association");
  auto bad=update->images; bad.entries[0].image={MaterialImageAction::Bind};
  Require(!registry.PublishMaterialImages(id,11,bad), "bind without a lease is invalid");
  bad=update->images; bad.entries.resize(257);
  Require(!registry.PublishMaterialImages(id,11,bad), "image slots bounded");
  NativeInstanceRegistry tight(bytes);
  const auto tight_id=tight.Create(11);
  Require(tight.PublishMaterialImages(tight_id,11,update->images), "exact combined instance/image budget");
  auto pinned=tight.ReadMaterialImages(tight_id,11);
  auto changed=update->images; changed.entries[0].image.image=a;
  Require(!tight.PublishMaterialImages(tight_id,11,changed) && !tight.ReadMaterialImages(tight_id,11) &&
      tight.Stats().bytes == bytes, "pinned replacement charged; budget refusal clears visibility");
  pinned.reset(); Require(tight.PublishMaterialImages(tight_id,11,changed), "released image budget reused");
  tight.Retire(tight_id); Require(tight.Stats().bytes == 0, "image retirement releases charged ownership");
  words.clear(); registry.Retire(id);
  const auto reloaded=registry.Create(12);
  Require(!registry.ReadMaterialImages(reloaded,12) && lease->entries[0].image.image == b &&
      lease->entries[1].image.image == c, "reload has no inherited images; pinned native leases outlive source");
}
void TestSourceHandoff() {
  using namespace instance_source;
  constexpr uint32_t visual = 0x10000, container = visual + 2632;
  // Independent literal addresses model the actual producer/getter/copy
  // layout. +2624 is deliberately a valid but unrelated pointer.
  std::unordered_map<uint64_t, uint32_t> words{
      {visual + 2620, 0x20000}, {visual + 1868, 2}, {visual + 2624, 0xDEAD},
      {kUpdateThread, 37}, {container + 8, 0x30000}, {container + 20, 0x40000},
      {0x30000, 0x50000}, {0x40000, 0x60000}, {0x30008, 2}};
  auto read = [&](uint64_t address) -> std::optional<uint32_t> {
    if ((address & 3) || address > UINT32_MAX) return {};
    const auto it = words.find(address);
    return it == words.end() ? std::nullopt : std::optional(it->second);
  };
  const auto update = ReadPublication(visual, 37, read);
  const auto render = ReadPublication(visual, 98, read);
  Require(update && update->graph == 0x20000 && update->count == 2 &&
          update->lane == 0 && update->palette == 0x50000 && render &&
          render->lane == 1 && render->palette == 0x60000, "actual source layout and thread selection");
  Require(!ReadPublication(UINT32_MAX - 100, 37, read) && !Palette(container, 2, read),
          "source address overflow and lane limits");
  NativeInstanceRegistry registry;
  Binding binding{registry.Create(4), 4, {update->palette, 0}};
  std::vector<RenderMatrix> matrices{World(2), World(4)};
  Require(registry.Publish(binding.instance, update->lane, matrices), "publish decoded producer input");
  Require(!Find(registry, binding, 4, render->palette),
          "producer publication alone is not a render handoff (original regression)");
  const auto bytes = registry.Stats().bytes;
  const auto transfer = ReadTransfer(container, read);
  for (uint32_t flags : {0, 1, 2, 3, 4, 7})
    Require(TransferReady(flags) == (flags == 3), "actual derived copy requires exactly dirty state 3");
  Require(!TransferReady(std::nullopt), "unreadable dirty word cannot advance render pose");
  matrices[1] = World(44); // late pose writer after InitBones, before render copy
  Require(transfer && transfer->source == 0x50000 && transfer->destination == 0x60000 &&
          transfer->count == 2 && PublishCompletedTransfer(registry, binding, transfer, matrices),
          "actual copy boundary imports the final pose, including late writers");
  auto pose = Find(registry, binding, 4, render->palette);
  Require(pose == registry.Read(binding.instance, 0) && registry.Stats().bytes == bytes,
          "render handoff shares the immutable producer pose without copying or residency growth");
  Require(pose->transforms[1][12] == 44 && !Find(registry, binding, 4, update->palette),
          "final publication includes late edits, and does not expose the changing update source");
  matrices[0] = World(8);
  Require(registry.Publish(binding.instance, 0, matrices), "next update before render handoff");
  Require(Find(registry, binding, 4, render->palette) == pose && pose->transforms[0][12] == 2,
          "update cannot advance the render snapshot early");
  Require(ApplyTransfer(registry, binding, transfer) &&
          Find(registry, binding, 4, render->palette)->transforms[0][12] == 8,
          "render advances only at the explicit handoff");
  Require(!Find(registry, binding, 5, render->palette) && !Find(registry, binding, 4, 0x9999),
          "wrong generation or untracked secondary palette cannot use primary pose");
  words[0x30008] = 1;
  Require(!ApplyTransfer(registry, binding, ReadTransfer(container, read)) &&
          !Find(registry, binding, 4, render->palette), "partial copy cannot publish stale render pose");
  words[0x30008] = 2;
  Require(ApplyTransfer(registry, binding, ReadTransfer(container, read)), "handoff recovers");
  words[0x30000] = 0x70000;
  Require(!ApplyTransfer(registry, binding, ReadTransfer(container, read)),
          "unpublished source reallocation invalidates render publication");
  words.erase(0x40000);
  Require(!ReadTransfer(container, read), "missing destination refused");
  words[visual + 1868] = NativeInstanceRegistry::kMaxTransforms + 1;
  Require(!ReadPublication(visual, 37, read), "source count bounded before import");
  words.clear(); matrices.clear(); matrices.shrink_to_fit();
  Require(pose->transforms[0][12] == 2, "leased native pose survives source destruction");
  registry.Retire(binding.instance);
  Require(!Find(registry, binding, 4, render->palette), "unload removes consumer visibility");
  pose.reset();
  Require(registry.Stats().bytes == 0, "handoff leases retire once, without duplicate accounting");
}

void TestRenderPoses() {
  NativeInstanceRegistry registry;
  const auto id = registry.Create(41);
  std::vector<RenderMatrix> matrices{World(0), World(.25f)};
  const auto publish = [&](uint64_t tick, bool active = true) {
    Require(registry.Publish(id, 0, matrices) && registry.Transfer(id, 0, 1, matrices.size()) &&
            registry.ObserveRenderTick(id, tick, active), "completed render tick publication");
    return registry.Read(id, 1);
  };
  auto first = publish(10);
  Require(registry.ReadRender(first, {10,100,.5f,true}) == first, "first pose snaps without invented history");
  matrices[0] = World(1); matrices[1] = World(1.25f);
  auto current = publish(11);
  const NativePosePhase phase{11,101,.25f,true};
  auto quarter = registry.ReadRender(current, phase);
  Require(quarter && quarter != current && quarter->transforms[0][12] == .25f &&
          quarter->transforms[1][12] == .5f, "whole native pose interpolates the completed endpoints");
  const auto bytes = registry.Stats().bytes;
  Require(registry.ReadRender(current, phase) == quarter && registry.Stats().bytes == bytes,
          "culling and both draw views reuse exactly one frame pose");
  Require(!registry.ReadRender(current, {11,101,.75f,true}) &&
          !registry.ReadRender(current, {11,100,.25f,true}), "conflicting or older frame request refuses");
  auto later = registry.ReadRender(current, {11,102,.75f,true});
  Require(later && later->transforms[0][12] == .75f && quarter->transforms[0][12] == .25f &&
          registry.Read(id,1)->transforms[0][12] == 1, "render phases never mutate source comparison or pinned poses");
  NativeObjectPrimitive<int> packet; packet.pose = current; packet.node = 1; packet.world = matrices[1];
  Require(BindNativeRenderPose(packet, quarter) && packet.pose == quarter && packet.world[12] == .5f,
          "production packet binds matching rigid world and skin pose before planning");
  auto foreign = std::make_shared<NativeInstancePose>(*quarter); ++foreign->instance;
  Require(!BindNativeRenderPose(packet, foreign) && packet.pose == quarter, "foreign render pose cannot replace a packet");
  foreign = std::make_shared<NativeInstancePose>(*quarter); ++foreign->model_generation;
  Require(!BindNativeRenderPose(packet, foreign), "render packet generation must match");
  foreign = std::make_shared<NativeInstancePose>(*quarter); foreign->transforms.pop_back();
  Require(!BindNativeRenderPose(packet, foreign), "incomplete render pose cannot bind a packet");
  foreign = std::make_shared<NativeInstancePose>(*current);
  Require(!registry.ReadRender(foreign, {11,103,.5f,true}), "matching IDs do not impersonate the exact completed publication");
  matrices[0] = World(1.5f); matrices[1] = World(1.75f);
  current = publish(11); // another completed writer in the SAME logic tick
  auto late = registry.ReadRender(current, {11,103,.5f,true});
  Require(late && late->transforms[0][12] == .75f,
          "same-tick late writes replace current without advancing previous");
  Require(registry.ObserveRenderTick(id,11,true) && registry.ReadRender(current,{11,103,.5f,true}) == late,
          "repeated unchanged handoff preserves the frame lease");
  current = publish(12);
  Require(registry.ReadRender(current,{12,104,.5f,true}) == current, "unchanged tick endpoints share storage");
  matrices[0] = World(1.75f); current = publish(14);
  Require(registry.ReadRender(current,{14,105,.5f,true}) == current, "skipped ticks snap rather than borrow stale history");
  matrices[0] = World(2); current = publish(15);
  Require(registry.ReadRender(current,{15,106,.5f,true})->transforms[0][12] == 1.875f, "contiguous history recovers");
  matrices[0] = World(2.25f); current = publish(13);
  Require(registry.ReadRender(current,{13,107,.5f,true}) == current, "clock rewind snaps");
  current = publish(13,false);
  Require(registry.ReadRender(current,{13,108,0,false}) == current, "disabled interpolation uses current authored pose");
  matrices[0] = World(2.5f); current = publish(14);
  Require(registry.ReadRender(current,{14,109,.5f,true}) == current, "re-enable does not interpolate across a coupled event");
  matrices[0] = World(20); current = publish(15);
  Require(registry.ReadRender(current,{15,110,.5f,true}) == current, "teleport snaps the whole pose");
  matrices[0] = World(20.25f); matrices[1][15] = 2; current = publish(16);
  Require(registry.ReadRender(current,{16,111,.5f,true}) == current, "nonaffine endpoint is never blended");
  Require(!registry.ReadRender(current,{16,112,std::numeric_limits<float>::quiet_NaN(),true}) &&
          !registry.ReadRender(current,{16,112,-.1f,true}) && !registry.ReadRender(current,{16,112,1.1f,true}),
          "invalid render phase refuses");
  Require(registry.ReadRender(current,{16,112,0,true}) == current &&
          registry.ReadRender(current,{16,113,1,true}) == current, "existing alpha-zero and completed-phase contract");
  registry.Invalidate(id,1);
  Require(!registry.ReadRender(current,{16,114,.5f,true}), "invalidated render publication cannot retain a current interpolation");
  Require(packet.pose->transforms[0][12] == .25f, "queued pose remains immutable after invalidation");
  registry.Retire(id);
  Require(!registry.ReadRender(current,{16,115,.5f,true}) && registry.Stats().bytes > 0,
          "retired in-flight render poses stay charged but cannot be rediscovered");
  packet.pose.reset(); first.reset(); current.reset(); quarter.reset(); later.reset(); late.reset(); foreign.reset();
  Require(registry.Stats().bytes == 0, "source, history and render leases retire without duplicate accounting");

  NativeInstanceRegistry probe;
  const auto probe_id = probe.Create(1);
  auto value = World(0);
  Require(probe.Publish(probe_id,0,{&value,1}), "render budget probe");
  const auto pose_bytes = probe.Stats().bytes - NativeInstanceRegistry::kEntryBytes;
  NativeInstanceRegistry bounded(NativeInstanceRegistry::kEntryBytes + 3*pose_bytes);
  const auto bounded_id = bounded.Create(1);
  Require(bounded.Publish(bounded_id,0,{&value,1}) && bounded.Transfer(bounded_id,0,1,1) &&
          bounded.ObserveRenderTick(bounded_id,1,true), "bounded first render endpoint");
  value = World(1);
  Require(bounded.Publish(bounded_id,0,{&value,1}) && bounded.Transfer(bounded_id,0,1,1) &&
          bounded.ObserveRenderTick(bounded_id,2,true), "bounded second render endpoint");
  auto endpoint = bounded.Read(bounded_id,1);
  auto pinned = bounded.ReadRender(endpoint,{2,2,.5f,true});
  Require(pinned && bounded.Stats().bytes == NativeInstanceRegistry::kEntryBytes + 3*pose_bytes,
          "history plus rendered result fits the exact shared byte cap");
  Require(!bounded.ReadRender(endpoint,{2,3,.75f,true}) && pinned->transforms[0][12] == .5f,
          "pinned frame overlap refuses instead of exceeding budget or mutating a GPU lease");
  pinned.reset();
  auto recovered = bounded.ReadRender(endpoint,{2,3,.75f,true});
  Require(recovered && recovered->transforms[0][12] == .75f, "render budget recovers after actual reader release");
  bounded.Retire(bounded_id); endpoint.reset(); recovered.reset();
  Require(bounded.Stats().bytes == 0, "bounded render history and outputs release once");
}

void TestSkeleton() {
  auto near = [](float a, float b) { return std::abs(a-b) < 2e-5f; };
  // Independent scalar spelling of the source's reversed-lane sign/dot
  // construction (82283A10..82283ADC), not the production quaternion helper.
  auto source_append = [](JointQuaternion old, JointQuaternion axis) {
    const std::array<float,4> b{old[3],old[2],old[1],old[0]}, a{axis[3],axis[2],axis[1],axis[0]};
    return JointQuaternion{
        a[0]*b[3]+a[1]*b[2]-a[2]*b[1]+a[3]*b[0],
        a[0]*b[2]-a[1]*b[3]+a[2]*b[0]+a[3]*b[1],
        a[0]*b[1]+a[1]*b[0]+a[2]*b[3]-a[3]*b[2],
        a[0]*b[0]-a[1]*b[1]-a[2]*b[2]-a[3]*b[3]};
  };
  for (int n=1; n<41; ++n) {
    const JointVector angles{n*.013f,-n*.017f,n*.023f};
    auto q = source_append({0,0,std::sin(angles[2]/2),std::cos(angles[2]/2)},
                           {0,std::sin(angles[1]/2),0,std::cos(angles[1]/2)});
    q = source_append(q,{std::sin(angles[0]/2),0,0,std::cos(angles[0]/2)});
    const auto expected = JointRotation(q), actual = JointEulerRotation(angles);
    for (size_t c=0; c<16; ++c) Require(near(actual[c],expected[c]), "source Euler append order, off-axis radians");
  }
  constexpr float half_pi = 1.5707963267948966f;
  const auto turn = JointEulerRotation({0,0,half_pi});
  Require(near(turn[0],0) && near(turn[1],1) && near(turn[4],-1), "row-vector positive Z rotation");

  std::unordered_map<uint64_t,uint32_t> words;
  auto put = [&](uint64_t p, JointVector v) { for (float f : v) { words[p] = std::bit_cast<uint32_t>(f); p += 4; } };
  auto read = [&](uint64_t p) -> std::optional<uint32_t> {
    if ((p&3) || p > UINT32_MAX-3) return {};
    const auto found = words.find(p);
    return found == words.end() ? std::nullopt : std::optional(found->second);
  };
  const RenderMatrix root{1.1f,.2f,.3f,0, -.4f,1.3f,.5f,0, .6f,-.7f,1.4f,0, 12,34,56,1};
  std::array<uint64_t,5> pairs;
  for (size_t n=0; n<pairs.size(); ++n)
    pairs[n] = (uint64_t(std::bit_cast<uint32_t>(root[n*2]))<<32) | std::bit_cast<uint32_t>(root[n*2+1]);
  for (size_t n=10; n<16; ++n) words[0x5000+88+(n-10)*4] = std::bit_cast<uint32_t>(root[n]);
  Require(skeleton_source::ReadRoot(pairs,0x5000,read) == root,
          "by-value root preserves register pair endianness and six caller stack words");
  words.erase(0x5000+108);
  Require(!skeleton_source::ReadRoot(pairs,0x5000,read) && !skeleton_source::ReadRoot(pairs,0xfffffff0,read),
          "root tail truncation and stack address overflow refuse");

  // Noncommuting rotations and an off-axis parent verify the complete chain,
  // using an independent double-precision scalar matrix product as the oracle.
  auto product = [](const RenderMatrix &a, const RenderMatrix &b) {
    RenderMatrix result{};
    for (size_t r=0; r<4; ++r) for (size_t c=0; c<4; ++c) {
      double sum = 0;
      for (size_t k=0; k<4; ++k) sum += double(a[r*4+k])*b[k*4+c];
      result[r*4+c] = float(sum);
    }
    return result;
  };
  NativeSkeletonJoint authored;
  authored.translation = {3,4,5}; authored.scale = {-2,0,3};
  authored.before_rotation = JointEulerRotation({.3f,-.7f,.1f});
  authored.rotation = JointEulerRotation({-.2f,.4f,.8f});
  authored.after_rotation = JointEulerRotation({.5f,.6f,-.9f});
  NativeJointChannels animated;
  std::vector<RenderMatrix> composed;
  auto local = JointIdentity(); local[12]=3; local[13]=4; local[14]=5;
  auto scaling = JointIdentity(); scaling[0]=-2; scaling[5]=0; scaling[10]=3;
  for (bool dynamic_rotation : {false,true}) {
    animated.rotated = dynamic_rotation; animated.rotation = {.2f,-.3f,.1f,.7f};
    const auto rotation = dynamic_rotation ? JointRotation(animated.rotation) : authored.rotation;
    const auto expected = product(scaling,product(authored.after_rotation,
        product(rotation,product(authored.before_rotation,product(local,root)))));
    Require(EvaluateNativeSkeleton({&authored,1},{&animated,1},root,composed), "complete authored/native rotation chain evaluates");
    for (size_t c=0; c<16; ++c) Require(near(composed[0][c],expected[c]),
        "pre/selected/post rotation order, non-unit quaternion and zero/negative scale are preserved");
  }
  // Reordered IDs, a child and a root sibling: parent ordinals are not pose IDs.
  for (uint32_t p : {0x1000,0x2000,0x3000}) {
    words[p+8] = 1|8; words[p+56] = words[p+60] = 0;
    words[p+64]=0x61726D00; // "arm", deliberately independent of its numeric key
    put(p+16,{0,0,0}); put(p+44,{1,1,1});
  }
  words[0x1000] = 2; words[0x1038] = 0x2000; words[0x103c] = 0x3000;
  words[0x2000] = 0; words[0x3000] = 1;
  words[0x1004] = 12; words[0x2004] = 10; words[0x3004] = 11;
  put(0x1010,{5,0,0}); put(0x102c,{2,3,4}); put(0x2010,{1,2,3}); put(0x202c,{5,1,1});
  std::vector<uint32_t> animation_targets, joint_sources;
  auto skeleton = skeleton_source::ReadSkeleton(0x1000,read,&animation_targets,&joint_sources);
  Require(animation_targets == std::vector<uint32_t>{10,11,12},"load-owned animation names use dense pose order, not tree traversal order");
  Require(joint_sources == std::vector<uint32_t>{0x2000,0x3000,0x1000},
          "joint pointer exports follow dense pose identities, including child and root siblings");
  words[0x3004] = 12;
  std::vector<uint32_t> ambiguous;
  Require(skeleton_source::ReadSkeleton(0x1000,read,&ambiguous).has_value() && ambiguous.empty(),
          "duplicate animation names cannot invalidate independent skeleton geometry");
  words[0x3004] = 11;
  Require(skeleton && skeleton->size() == 3 && (*skeleton)[1].parent == 0 &&
          (*skeleton)[2].parent == kNativeSkeletonRoot, "load-owned topology preserves child and root sibling");
  std::vector<NativeJointChannels> channels(3);
  std::vector<RenderMatrix> pose;
  Require(EvaluateNativeSkeleton(*skeleton,channels,World(10),pose), "whole native hierarchy evaluation");
  Require(pose[2][12] == 15 && pose[2][0] == 2 && pose[0][12] == 17 && pose[0][13] == 6 &&
          pose[0][14] == 12 && pose[0][0] == 5 && pose[1][12] == 10,
          "parent scale affects child translation but not compensated child basis or root siblings");
  (*skeleton)[1].inherit_parent_scale = true;
  Require(EvaluateNativeSkeleton(*skeleton,channels,World(10),pose) && pose[0][0] == 10 && pose[0][5] == 3,
          "explicit inherited scale affects the full child linear transform");
  (*skeleton)[1].inherit_parent_scale = false; channels[0].reset_parent = true;
  Require(EvaluateNativeSkeleton(*skeleton,channels,World(10),pose) && pose[0][12] == 2 && pose[0][13] == 6,
          "reset parent still applies separately passed parent scale");
  channels[0] = {}; channels[2].translated = channels[2].scaled = true;
  channels[2].translation = {1,0,0}; channels[2].scale = {3,2,1};
  Require(EvaluateNativeSkeleton(*skeleton,channels,World(10),pose) && pose[0][12] == 14 && pose[0][13] == 4,
          "fresh authored channel overrides replace load-owned defaults");
  auto before = pose;
  channels[2].translation[0] = std::numeric_limits<float>::infinity();
  Require(!EvaluateNativeSkeleton(*skeleton,channels,World(10),pose) && pose == before,
          "invalid channel refuses whole pose without partial output");
  channels[2] = {};
  auto bad = *skeleton; bad[1].parent = 2;
  Require(!EvaluateNativeSkeleton(bad,channels,World(10),pose) && pose == before, "forward/cyclic native parent refuses");
  bad = *skeleton; bad[1].pose_index = 2;
  Require(!ValidNativeSkeleton(bad), "duplicate native pose identity refuses");
  bad = *skeleton; bad[1].pose_index = 3;
  Require(!ValidNativeSkeleton(bad), "sparse or out-of-range native pose identity refuses");
  words[0x2038] = 0x1000;
  auto unchanged_sources=joint_sources;
  Require(!skeleton_source::ReadSkeleton(0x1000,read,&animation_targets,&joint_sources) && joint_sources == unchanged_sources,
          "source cycle refuses without publishing a partial joint binding table");
  words[0x2038] = 0; words[0x2008] |= 0x200000;
  Require(!skeleton_source::ReadSkeleton(0x1000,read), "camera-facing bones require a real owned view contract");
  words[0x2008] = 1|8; words[0x1008] |= 16|32;
  put(0x1050,{0,0,half_pi}); put(0x105c,{0,0,-half_pi});
  const auto extended = skeleton_source::ReadSkeleton(0x1000,read);
  Require(extended && near((*extended)[0].before_rotation[1],1) && near((*extended)[0].after_rotation[1],-1),
          "104-byte authored pre/post rotations import as radians, not degrees");

  words[0x4000] = 1|2|4|64; put(0x4008,{3,4,5}); put(0x4024,{2,2,2});
  put(0x4014,{0,0,0}); words[0x4020] = std::bit_cast<uint32_t>(1.0f);
  const auto dynamic = skeleton_source::ReadChannels(0x4000,1,read);
  Require(dynamic && (*dynamic)[0].reset_parent && (*dynamic)[0].rotation[3] == 1 &&
          (*dynamic)[0].translation[1] == 4 && (*dynamic)[0].scale[2] == 2, "48-byte channel layout is independent of native representation");
  words[0x4000] = 0; words.erase(0x4008);
  Require(bool(skeleton_source::ReadChannels(0x4000,1,read)), "inactive authored bytes are never imported");
  Require(!skeleton_source::ReadChannels(0xfffffff0,1,read), "channel range overflow refuses");
  words.clear(); // all subsequent work must survive complete source destruction

  ModelMaterialRegistry models;
  Require(models.Publish(51,{}, {},*skeleton,animation_targets,joint_sources), "skeleton and outgoing joint aliases share the load-owned model and budget");
  auto model = models.FindModel(51);
  Require(model && model->Skeleton().size() == 3, "model holds the native hierarchy");
  Require(std::ranges::equal(model->AnimationTargets(),animation_targets),"animation bindings share the immutable model generation after source destruction");
  Require(model->FindJoint(0) == &model->Skeleton()[1] && !model->FindJoint(3),
          "native joint selection uses pose identity, not tree ordinal");
  auto selected=models.FindJointSource(51,0), absent=models.FindJointSource(51,UINT32_MAX);
  Require(selected && selected->source_node == 0x2000 && selected->model == model &&
          selected->model->FindJoint(0)->animation_name.View() == "arm" &&
          absent && !absent->source_node && !models.FindJointSource(99,0),
          "owned selection survives all source destruction; known absence differs from unavailable model");
  selected.reset(); absent.reset();
  Require(EvaluateNativeSkeleton(model->Skeleton(),channels,World(10),pose), "evaluate without any remaining source bytes");
  NativeInstanceRegistry instances;
  const auto id = instances.Create(model->Generation(),model);
  Require(instances.Publish(id,0,pose) && instances.Transfer(id,0,1,pose.size()) && instances.ObserveRenderTick(id,1,true),
          "evaluated native values feed existing instance handoff and render owner");
  auto completed = instances.Read(id,1);
  models.Retire(51); model.reset();
  Require(!models.FindJointSource(51,0),"retirement removes outgoing aliases despite retained native pose leases");
  Require(models.Stats().bytes > 0 && completed->model->Skeleton().size() == 3,
          "queued/native pose pins exactly its skeletal model after source retirement");
  channels[2].translated = true; channels[2].translation = {5.5f,0,0};
  Require(EvaluateNativeSkeleton(completed->model->Skeleton(),channels,World(10),pose) &&
          instances.Publish(id,0,pose) && instances.Transfer(id,0,1,pose.size()) && instances.ObserveRenderTick(id,2,true),
          "next owned evaluation advances existing timed pose history");
  auto current = instances.Read(id,1), render = instances.ReadRender(current,{2,2,.5f,true});
  Require(render && render->transforms[2][12] == 15.25f && completed->transforms[2][12] == 15,
          "native skeletal output reaches immutable render interpolation without guest scratch");
  instances.Retire(id); completed.reset(); current.reset(); render.reset();
  Require(models.Stats().bytes == 0 && instances.Stats().bytes == 0, "model and pose lifetime accounting releases once");
  const auto required = ModelMaterialRegistry::RetainedBytes({},0,0,skeleton->size());
  ModelMaterialRegistry too_small(required-1);
  Require(!too_small.Publish(51,{}, {},*skeleton) && too_small.Stats().bytes == 0,
          "skeletal vector residency cannot escape existing aggregate model budget");
  ModelMaterialRegistry names_too_small(required+animation_targets.size()*sizeof(uint32_t)-1);
  Require(!names_too_small.Publish(51,{}, {},*skeleton,animation_targets),"authored animation bindings debit model capacity budget");
  Require(!models.Publish(51,{}, {},*skeleton,{10}),"partial animation bindings cannot be published");
  for (auto invalid : {std::vector<uint32_t>{0x1000}, std::vector<uint32_t>{0x1000,0x1000,0x3000},
                       std::vector<uint32_t>{0,0x2000,0x3000}, std::vector<uint32_t>{0x1001,0x2000,0x3000},
                       std::vector<uint32_t>{0xfffffffc,0x2000,0x3000}})
    Require(!models.Publish(51,{}, {},*skeleton,animation_targets,std::move(invalid)) && !models.FindJointSource(51,0),
            "partial, aliased, null, misaligned and overflowing outgoing joint bindings refuse");
  const auto alias_bytes=ModelMaterialRegistry::RetainedBytes({},0,0,skeleton->size(),animation_targets.size(),joint_sources.size());
  ModelMaterialRegistry exact(alias_bytes);
  Require(exact.Publish(51,{}, {},*skeleton,animation_targets,joint_sources) && exact.Stats().bytes == alias_bytes,
          "joint alias capacity is charged to the existing aggregate model budget");
  auto pinned=exact.FindJointSource(51,0);
  Require(!exact.Publish(51,{}, {},*skeleton,animation_targets,joint_sources) && !exact.FindJointSource(51,0) &&
          pinned->model->FindJoint(0)->animation_name.View() == "arm",
          "retired pinned generation charges its bindings and cannot leak through a reused graph key");
  const auto old_generation=pinned->model->Generation(); pinned.reset();
  auto replacement=joint_sources; replacement[0]=0x4000;
  Require(exact.Publish(51,{}, {},*skeleton,animation_targets,replacement),"released generation makes room for replacement");
  const auto replaced=exact.FindJointSource(51,0);
  Require(replaced && replaced->source_node == 0x4000 && replaced->model->Generation() != old_generation,
          "source graph reuse publishes only the new generation's outgoing joint binding");
  ModelMaterialRegistry aliases_too_small(alias_bytes-1);
  Require(!aliases_too_small.Publish(51,{}, {},*skeleton,animation_targets,joint_sources),
          "joint bindings cannot exceed the model budget by one byte");
}
}
void TestNativeInstances() {
  TestEyeMaterialOwnership();
  TestMaterialUVBudgets();
  TestMaterialUVProgramOwnership();
  TestMaterialImageOwnership();
  TestSourceHandoff();
  TestRenderPoses();
  TestSkeleton();
  TestAnimationClips();
  NativeInstanceRegistry registry;
  Require(!registry.Create(0), "instance needs a published native model generation");
  const auto first = registry.Create(100), second = registry.Create(100);
  Require(first && second && first != second, "shared model has distinct native instances");
  std::vector<RenderMatrix> source{World(2), World(4)};
  Require(registry.Publish(first, 0, source), "initial pose");
  auto pose = registry.Read(first, 0);
  Require(pose && pose->instance == first && pose->model_generation == 100, "native identity");
  Require(registry.Publish(first, 0, source) && registry.Read(first, 0) == pose,
          "unchanged pose reuses immutable storage");
  source[0] = World(9);
  Require(registry.Publish(first, 1, source), "separate update lane");
  source.clear(); source.shrink_to_fit();
  Require(pose->transforms[0][12] == 2 && registry.Read(first, 1)->transforms[0][12] == 9,
          "source-free consumers and independent thread lanes");
  Require(!registry.Read(second, 0), "model sharing cannot share mutable instance pose");
  Require(!registry.Publish(first, 2, {&pose->transforms[0], 1}), "lane bound");
  auto invalid = World(0); invalid[3] = std::numeric_limits<float>::infinity();
  Require(!registry.Publish(first, 0, {&invalid, 1}) && !registry.Read(first, 0),
          "invalid update cannot leave a stale current pose");
  Require(pose->transforms[0][12] == 2, "invalid publication cannot damage existing lease");
  registry.Retire(first); registry.Retire(second);
  Require(!registry.Read(first, 1) && registry.Stats().bytes > 0, "retired lease stays charged");
  pose.reset();
  Require(registry.Stats().bytes == 0, "retired last reader releases pose storage");
  const auto replacement = registry.Create(101);
  Require(replacement > second && !registry.Read(first, 0), "retired IDs never alias a reload");
  registry.Retire(replacement);

  NativeInstanceRegistry count_limit(4096, 1);
  const auto only = count_limit.Create(1);
  Require(only && !count_limit.Create(1), "instance count backpressure");
  count_limit.Retire(only);
  Require(count_limit.Create(1) > only, "slot reuse has a fresh ID");
  NativeInstanceRegistry tiny(NativeInstanceRegistry::kEntryBytes - 1);
  Require(!tiny.Create(1), "bookkeeping is part of byte budget");

  NativeInstanceRegistry size_probe;
  const auto probe = size_probe.Create(1);
  auto value = World(1);
  Require(size_probe.Publish(probe, 0, {&value, 1}), "budget size probe");
  NativeInstanceRegistry bounded(size_probe.Stats().bytes);
  const auto id = bounded.Create(1);
  Require(bounded.Publish(id, 0, {&value, 1}), "exact bounded publication");
  auto pinned = bounded.Read(id, 0);
  value = World(3);
  Require(!bounded.Publish(id, 0, {&value, 1}) && !bounded.Read(id, 0),
          "pinned replacement overlap cannot exceed budget or retain stale current pose");
  pinned.reset();
  Require(bounded.Publish(id, 0, {&value, 1}), "backpressure recovers after reader releases");
  bounded.Invalidate(id, 0);
  Require(!bounded.Read(id, 0), "producer invalidation removes publication");
  const std::vector<RenderMatrix> oversized(NativeInstanceRegistry::kMaxTransforms + 1, World(0));
  Require(!bounded.Publish(id, 0, oversized), "transform count bound before copying");

  std::shared_ptr<const NativeInstancePose> surviving;
  {
    NativeInstanceRegistry temporary;
    const auto t = temporary.Create(7);
    Require(temporary.Publish(t, 0, {&value, 1}), "temporary owner publication");
    surviving = temporary.Read(t, 0);
  }
  Require(surviving->model_generation == 7 && surviving->transforms[0][12] == 3,
          "pose lease outlives registry");
  surviving.reset();

  const auto concurrent = registry.Create(2);
  Require(registry.Publish(concurrent, 0, {&value, 1}), "concurrent setup");
  std::barrier rendezvous(2);
  bool preserved = false;
  std::thread reader([&] {
    const auto old = registry.Read(concurrent, 0);
    rendezvous.arrive_and_wait(); rendezvous.arrive_and_wait();
    preserved = old->transforms[0][12] == 3 &&
        registry.Read(concurrent, 0)->transforms[0][12] == 8;
  });
  rendezvous.arrive_and_wait();
  value = World(8);
  const bool updated = registry.Publish(concurrent, 0, {&value, 1});
  rendezvous.arrive_and_wait(); reader.join();
  Require(updated && preserved, "render lease and producer update remain independent");
  registry.Retire(concurrent);
  Require(registry.Stats().bytes == 0, "all native instance ownership retired");
  std::cout << "native instance identity, lanes, source-free poses, reload and backpressure passed\n";
}
