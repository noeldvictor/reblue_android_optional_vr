#include "gpu/scene/native_animation_source.h"
#include "gpu/scene/native_animation_controller_source.h"
#include "gpu/scene/native_instance.h"
#include "gpu/scene/native_skeleton_source.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <unordered_map>

using namespace bd::gpu::scene;
namespace {
void Require(bool valid, const char *message) {
  if (!valid) throw std::runtime_error(message);
}
bool Near(float a, float b, float epsilon = 1e-5f) { return std::abs(a-b) < epsilon; }
struct ClipSource {
  std::unordered_map<uint64_t,uint32_t> words;
  std::optional<uint32_t> Read(uint64_t address) const {
    if ((address & 3) || address > UINT32_MAX-3) return {};
    const auto it = words.find(address);
    return it == words.end() ? std::nullopt : std::optional(it->second);
  }
  void Byte(uint64_t address, uint8_t value) {
    const auto shift = unsigned(3-(address & 3))*8;
    auto &word = words[address & ~uint64_t(3)];
    word = (word & ~(uint32_t(255)<<shift)) | (uint32_t(value)<<shift);
  }
  void Half(uint64_t address, uint16_t value) { Byte(address,uint8_t(value>>8)); Byte(address+1,uint8_t(value)); }
  void Word(uint64_t address, uint32_t value) {
    for (unsigned b=0; b<4; ++b) Byte(address+b,uint8_t(value>>(24-8*b)));
  }
  void Vector(uint64_t address, uint16_t frame, JointVector values) {
    Half(address,frame);
    for (unsigned axis=0; axis<3; ++axis) Word(address+4+axis*4,std::bit_cast<uint32_t>(values[axis]));
  }
  void Rotation(uint64_t address, uint16_t frame, std::array<int16_t,3> angles) {
    Half(address,frame);
    for (unsigned axis=0; axis<3; ++axis) Half(address+2+axis*2,uint16_t(angles[axis]));
  }
  ClipSource() {
    Word(0x1000,0x2000); Half(0x1004,30); Half(0x1006,2); Half(0x1008,3);
    Word(0x100C,std::bit_cast<uint32_t>(2.0f)); // 60 samples/sec, 30 logic ticks/sec
    for (uint32_t r : {0x2000,0x2024,0x2048}) {
      Word(r,0); Word(r+4,0); Word(r+8,0);
      Half(r+12,0); Half(r+14,0); Half(r+16,0);
    }
    // Independent literal source order C,B,A, while model preorder is A,B,C
    // and dense pose IDs are 2,0,1. Hashes deliberately cross word boundaries.
    Word(0x2012,0xCC332211); Word(0x2036,0xBB776655); Word(0x205A,0xAA998877);
    Word(0x2024,0x5000); Half(0x2030,1); Vector(0x5000,123,{1,2,3});
    Word(0x2048,0x3000); Word(0x204C,0x4000); Word(0x2050,0x6000);
    Half(0x2054,3); Half(0x2056,3); Half(0x2058,3);
    Vector(0x3000,0,{0,0,0}); Vector(0x3010,30,{4,0,0}); Vector(0x3020,60,{8,0,0});
    Rotation(0x4000,0,{0,0,0}); Rotation(0x4008,30,{0,16384,0}); Rotation(0x4010,60,{0,-32768,0});
    Vector(0x6000,0,{1,1,1}); Vector(0x6010,30,{3,3,3}); Vector(0x6020,60,{5,5,5});
  }
};
constexpr std::array bindings{animation_source::JointBinding{2,0xAA998877},
    animation_source::JointBinding{0,0xBB776655},animation_source::JointBinding{1,0xCC332211}};
std::optional<NativeAnimationClip> Import(const ClipSource &source, size_t budget = 128*1024) {
  return animation_source::ReadKeyedClip(0x1000,bindings,budget,
      [&](uint64_t address) { return source.Read(address); });
}
void TestImportedAnimation() {
  ClipSource source;
  auto clip = Import(source);
  Require(clip && clip->JointCount() == 3 && clip->Duration() == 1 && clip->Tracks().size() == 3,
          "whole relocated keyed clip binds by authored hash, not descriptor order");
  Require(clip->Tracks()[2].translation.size() == 3 && clip->Tracks()[2].rotation.size() == 3,
          "loader counts import exactly three keys, not inclusive search bound plus one");
  const auto bytes = clip->RetainedBytes();
  Require(Import(source,bytes).has_value() && !Import(source,bytes-1), "exact retained-capacity budget boundary");
  auto lease = std::make_shared<const NativeAnimationClip>(std::move(*clip));
  clip.reset(); source.words.clear();
  std::vector<NativeJointChannels> channels;
  Require(lease->Sample(.25f,channels) && channels.size() == 3, "sample after complete source destruction");
  Require(channels[2].translated && channels[2].rotated && channels[2].scaled &&
          channels[2].translation[0] == 2 && channels[2].scale[0] == 2 &&
          channels[0].translation == JointVector{1,2,3} && !channels[1].translated &&
          !channels[1].rotated && !channels[1].scaled && !channels[2].reset_parent,
          "native channel activation, constant time, scaled clock and missing tracks");
  const auto rotation = JointRotation(channels[2].rotation);
  Require(Near(rotation[0],std::sqrt(.5f)) && Near(rotation[2],-std::sqrt(.5f)),
          "packed quarter-turn keys become half-angle radians at fractional sampling time");
  std::array<NativeSkeletonJoint,3> skeleton;
  skeleton[0].pose_index=2; skeleton[1].pose_index=0; skeleton[1].parent=0;
  skeleton[2].pose_index=1; skeleton[2].translation={-3,0,0};
  auto root = JointIdentity(); root[12]=10;
  std::vector<RenderMatrix> pose;
  Require(EvaluateNativeSkeleton(skeleton,channels,root,pose) && pose[2][12] == 12 && pose[1][12] == 7,
          "owned clip channels feed the existing production hierarchy evaluator");
  Require(Near(pose[0][12],12+8*std::sqrt(.5f)) && Near(pose[0][13],4) &&
          Near(pose[0][14],4*std::sqrt(.5f)), "animated parent scale/rotation reaches child world pose");
  NativeInstanceRegistry registry;
  const auto instance=registry.Create(91);
  Require(registry.Publish(instance,0,pose) && registry.Transfer(instance,0,1,pose.size()),
          "sampled animation reaches existing immutable completed instance owner");
  const auto published=registry.Read(instance,1);
  Require(lease->Sample(.5f,channels) && channels[2].translation[0] == 4 &&
          Near(JointRotation(channels[2].rotation)[0],0), "exact interior key is preserved");
  Require(lease->Sample(9,channels) && channels[2].translation[0] == 8 &&
          lease->Sample(-1,channels) && channels[2].translation[0] == 0, "clip clock clamps, never implicitly loops");
  const auto before=channels;
  Require(!lease->Sample(std::numeric_limits<float>::quiet_NaN(),channels) &&
          channels[2].translation == before[2].translation, "bad sample is transactional");
  registry.Retire(instance); lease.reset();
  Require(published && published->transforms[2][12] == 12,
          "completed frame outlives clip/source/instance retirement");
}
void TestAnimationRefusals() {
  auto refuse = [](auto change, const char *message) { ClipSource source; change(source); Require(!Import(source),message); };
  refuse([](auto &s){s.Half(0x1006,3);},"compressed clip is not silently treated as keyed linear data");
  refuse([](auto &s){s.Word(0x100C,std::bit_cast<uint32_t>(0.0f));},"zero sample rate refuses");
  refuse([](auto &s){s.Word(0x100C,std::bit_cast<uint32_t>(std::numeric_limits<float>::infinity()));},"nonfinite rate refuses");
  refuse([](auto &s){s.Half(0x1008,4097);},"unbounded track count refuses");
  refuse([](auto &s){s.Word(0x1000,UINT32_MAX-12);},"descriptor address overflow refuses");
  refuse([](auto &s){s.Word(0x2048,UINT32_MAX-12);},"key address overflow refuses");
  refuse([](auto &s){s.Half(0x2054,0);},"nonnull empty track refuses");
  refuse([](auto &s){s.words.erase(0x302C);},"truncated last key refuses the whole clip");
  refuse([](auto &s){s.Word(0x3004,std::bit_cast<uint32_t>(std::numeric_limits<float>::quiet_NaN()));},"nonfinite key refuses");
  refuse([](auto &s){s.Half(0x3010,0);},"duplicate key times refuse");
  refuse([](auto &s){s.Half(0x3010,uint16_t(-1));},"negative source key time refuses");
  refuse([](auto &s){s.Half(0x3000,1);},"missing predecessor is not synthesized");
  refuse([](auto &s){s.Half(0x3020,59);},"missing terminal bracket cannot authorize an extra source key");
  ClipSource source;
  auto bad=bindings; bad[1].pose_index=2;
  auto read=[&](uint64_t address){ return source.Read(address); };
  Require(!animation_source::ReadKeyedClip(0x1000,bad,128*1024,read),"duplicate model-local pose identity refuses");
  bad=bindings; bad[1].name_hash=bad[0].name_hash;
  Require(!animation_source::ReadKeyedClip(0x1000,bad,128*1024,read),"ambiguous model joint hash refuses");
  Require(!animation_source::Word(UINT32_MAX-1,read) && !animation_source::Half(UINT32_MAX,read),
          "unaligned boundary reads cannot wrap");
  source.Half(0x200C,65535); // null channel's inactive count is not allocation authority
  Require(Import(source).has_value(),"null channel ignores unused count");
}
void TestAngularSegments() {
  constexpr float pi=std::numbers::pi_v<float>;
  auto sample = [&](JointVector a, JointVector b, float time) {
    for (auto &angle : a) angle /= 2*pi;
    for (auto &angle : b) angle /= 2*pi;
    NativeAnimationTrack track; track.rotation={{0,a},{1,b}};
    std::vector<NativeAnimationTrack> tracks; tracks.push_back(std::move(track));
    auto clip=NativeAnimationClip::Create(1,1,std::move(tracks),4096);
    std::vector<NativeJointChannels> result;
    Require(clip && clip->Sample(time,result),"angular clip admission"); return result[0].rotation;
  };
  const auto forward=sample({0,0,pi*.9f},{0,0,-pi*.9f},.5f);
  const auto reverse=sample({0,0,-pi*.9f},{0,0,pi*.9f},.5f);
  Require(Near(forward[2],1) && Near(forward[3],0) && Near(reverse[2],1),
          "both wrap directions use the short arc with the source's positive-turn sign");
  const auto endpoint=sample({0,0,pi*.9f},{0,0,-pi*.9f},1);
  Require(endpoint[2]<0,"exact endpoint keeps authored quaternion sign, not global unwrapping");
  const auto tie=sample({0,0,0},{0,0,-pi},.5f);
  Require(tie[2]<0 && Near(tie[2],-std::sqrt(.5f)),"exact negative half-turn does not wrap");
  const auto composed=sample({.6f,-.4f,.8f},{.6f,-.4f,.8f},.25f);
  const auto actual=JointRotation(composed);
  // Independent explicit qZ*qY*qX expression, not a matrix assembled by the sampler.
  const float sx=std::sin(.3f),cx=std::cos(.3f),sy=std::sin(-.2f),cy=std::cos(-.2f),sz=std::sin(.4f),cz=std::cos(.4f);
  const auto expected=JointRotation({cz*cy*sx-sz*sy*cx,cz*sy*cx+sz*cy*sx,sz*cy*cx-cz*sy*sx,cz*cy*cx+sz*sy*sx});
  for(size_t n=0;n<16;++n) Require(Near(actual[n],expected[n]),"noncommuting Euler order is preserved");
}
void TestPackedAngularReference() {
  // Independent source-domain reference: wrap before converting to radians.
  // Include many nonzero exact half-turns: comparing rounded radians instead
  // can choose the opposite arc even though both endpoint matrices are right.
  for (int a=-32768; a<0; a+=257) for (int delta : {32767,32768,32769}) {
    const int b=int(int16_t(uint16_t(a+delta)));
    ClipSource source;
    source.Rotation(0x4000,0,{0,0,int16_t(a)});
    source.Rotation(0x4008,30,{0,0,int16_t(b)});
    source.Rotation(0x4010,60,{0,0,int16_t(b)});
    auto clip=Import(source);
    std::vector<NativeJointChannels> channels;
    Require(clip && clip->Sample(.25f,channels),"source-domain angular fixture admission");
    int left=a,right=b;
    if (right-left < -32768) right+=65536;
    else if (right-left > 32768) left+=65536;
    const float units=float(std::fma(double(right),.5,double(float(left*.5f))));
    const float half_angle=(units*0x1.921fb6p-14f)*.5f;
    Require(Near(channels[2].rotation[2],std::sin(half_angle)) &&
            Near(channels[2].rotation[3],std::cos(half_angle)),
            "packed half-turn tie and adjacent arcs match independent source arithmetic");
  }
}
void TestLoadedAnimationAssets() {
  ClipSource source;
  auto read = [&](uint64_t address){return source.Read(address);};
  auto import = [&](size_t budget = NativeAnimationResidency::kMaxBytes){
    return animation_source::ReadKeyedAsset(0x1000,budget,read);
  };
  auto asset = import();
  Require(asset && asset->FindTrack(0xAA998877)->pose_index == 2 && !asset->FindTrack(0x12345678),
          "completed load owns all named tracks before model/slot selection");
  const size_t bytes = asset->RetainedBytes();
  Require(import(bytes).has_value() && !import(bytes-1),"whole asset metadata and key capacity share exact byte budget");
  NativeAnimationResidency residency(bytes+NativeAnimationResidency::kEntryBytes);
  Require(residency.Publish(7,0x1000,std::move(*asset)),"completed loader publishes owned clip");
  auto lease = residency.Find(0x1000), alias = residency.Find(0x1000);
  Require(lease && lease == alias && residency.Size() == 1 && residency.AvailableAssetBytes() == 0,
          "packed aliases share the one resident payload and aggregate cap");
  residency.Retire(8);
  Require(residency.Find(0x1000) == lease,"unrelated loader retirement cannot erase shared motion");
  residency.Retire(7);
  Require(!residency.Find(0x1000) && residency.Bytes() == bytes+NativeAnimationResidency::kEntryBytes,
          "retired but leased motion still counts against residency");
  auto replacement = import();
  Require(replacement && !residency.Publish(9,0x1000,std::move(*replacement)) && !residency.Find(0x1000),
          "reused source cannot bypass retired lease budget or expose stale generation");
  source.words.clear();
  // Model dense order B,C,A plus a joint absent from this clip. No source reads
  // remain possible. Sentinel values expose accidental writes to inactive bytes.
  constexpr std::array names{0xBB776655u,0xCC332211u,0xAA998877u,0x12345678u};
  std::vector<animation_source::ChannelRecord> records(4);
  for (auto &record : records) { record.fill(0x7FC01234); record[0]=64|7; }
  const auto before = records;
  Require(animation_source::ApplyKeyedAsset(*lease,names,.25f,true,records),"source-free preserve-mode application");
  Require(records[0][0] == (64|1) && records[0][5] == before[0][5] &&
          records[1][0] == 64 && records[1][2] == before[1][2] &&
          records[2][0] == (64|128|7) && std::bit_cast<float>(records[2][2]) == 2 &&
          records[3][0] == before[3][0] && records[3][1] == names[3] && records[3][2] == before[3][2],
          "matched absent channels clear only activation, unmatched tracks preserve flags/values, animated tracks dirty");
  Require(animation_source::ApplyKeyedAsset(*lease,names,.25f,false,records) && records[0][0] == 1 &&
          records[0][5] == 0 && records[1][0] == 0 && records[1][2] == 0 &&
          records[2][0] == (128|7) && records[3][0] == 0 && records[3][2] == 0,
          "whole-model application clears all bytes before assigning authored names and channels");
  const auto valid = records;
  Require(!animation_source::ApplyKeyedAsset(*lease,names,std::numeric_limits<float>::quiet_NaN(),true,records) &&
          records == valid,"sampling failure leaves output transactional");
  auto duplicate = names; duplicate[0]=duplicate[1];
  Require(!animation_source::ApplyKeyedAsset(*lease,duplicate,.25f,true,records) && records == valid,
          "duplicate model names cannot guess traversal cursor semantics");
  Require(!animation_source::ApplyKeyedAsset(*lease,std::span(names).first(3),.25f,true,records) && records == valid,
          "preserve mode cannot resize an incompatible channel array");
  // Decode exactly the records produced by the runtime boundary into the real
  // native hierarchy consumer, then publish through the existing instance owner.
  auto channels = skeleton_source::ReadChannels(0x8000,records.size(),[&](uint64_t address)->std::optional<uint32_t>{
    if (address<0x8000 || (address&3) || address>=0x8000+records.size()*48) return {};
    const auto offset = address-0x8000; return records[offset/48][(offset%48)/4];
  });
  std::vector<NativeSkeletonJoint> skeleton(4);
  for (uint32_t n=0; n<4; ++n) skeleton[n].pose_index=n;
  std::vector<RenderMatrix> pose;
  Require(channels && EvaluateNativeSkeleton(skeleton,*channels,JointIdentity(),pose) && pose[2][12] == 2,
          "runtime outgoing channel representation drives actual native hierarchy evaluation");
  NativeInstanceRegistry instances;
  auto id=instances.Create(91);
  Require(instances.Publish(id,0,pose) && instances.Transfer(id,0,1,4),"owned clip reaches completed native pose owner");
  auto completed=instances.Read(id,1);
  lease.reset(); alias.reset(); instances.Retire(id);
  Require(residency.Bytes() == 0 && completed->transforms[2][12] == 2,
          "completed pose outlives motion/model/instance sources without retaining curve residency");
  source = ClipSource{}; replacement=import();
  Require(replacement && residency.Publish(9,0x1000,std::move(*replacement)),"source-address reuse loads fresh asset after leases retire");
  const auto reloaded = residency.Find(0x1000);
  residency.Invalidate(0x1000);
  source.Half(0x1006,3);
  Require(!import() && !residency.Find(0x1000) && reloaded->Duration() == 1,
          "malformed compressed replacement invalidates lookup while prior native lease remains immutable");
}
ClipSource CubicSource(float rate = 1) {
  ClipSource source;
  source.Half(0x1006,3); source.Word(0x100c,std::bit_cast<uint32_t>(rate));
  auto key = [&](uint32_t address, int16_t frame, uint16_t value, float tangent) {
    source.Half(address,uint16_t(frame)); source.Half(address+2,value);
    source.Word(address+4,std::bit_cast<uint32_t>(tangent));
  };
  for (uint32_t table : {0x7000,0x7100,0x7200})
    for (unsigned axis=0; axis<3; ++axis) { source.Word(table+axis*8,0); source.Word(table+axis*8+4,0); }
  for (unsigned channel=0; channel<3; ++channel) {
    source.Word(0x2048+channel*4,0x7000+channel*0x100); source.Half(0x2054+channel*2,2);
  }
  source.Word(0x7000,3); source.Word(0x7004,0x8000);
  key(0x8000,0,0,3); key(0x8008,15,0x4400,-2); key(0x8010,30,0x4000,7); // T: 0 -> 4 -> 2, asymmetric tangents
  source.Word(0x7008,1); source.Word(0x700c,0x8100);
  key(0x8100,9,0xc000,std::numeric_limits<float>::quiet_NaN()); // constant -2, unused tangent
  source.Word(0x7010,UINT32_MAX); // null Z ignores inactive count
  source.Word(0x7110,3); source.Word(0x7114,0x8200);
  key(0x8200,0,30000,500); key(0x8208,1,uint16_t(-30000),0); key(0x8210,30,30000,0);
  source.Word(0x7200,2); source.Word(0x7204,0x8300);
  key(0x8300,0,0x3c00,2); key(0x8308,30,0x4200,2); // S: 1 -> 3, derivative 2 / authored sec
  for (uint32_t offset : {8,16}) { source.Word(0x7200+offset,1); source.Word(0x7204+offset,0x8400); }
  key(0x8400,-7,0x3c00,0); // cubic keys may precede the start; constant holds
  return source;
}
void TestSelectedAnimationResidency() {
  auto source=CubicSource();
  auto read=[&](uint64_t address){return source.Read(address);};
  source.Word(0x9000+1920+56,0xa000); source.Word(0xa00c,0x1000);
  Require(animation_source::SelectedSlotSource(0x9000,1,read) == 0x1000 &&
          !animation_source::SelectedSlotSource(0x9000,0,read) &&
          !animation_source::SelectedSlotSource(0x9000,UINT32_MAX,read),
          "actual selected slot stride and ready-entry pointer, not motion ID or an in-flight lookup");
  const auto bytes=animation_source::ReadKeyedAsset(0x1000,128*1024,read)->RetainedBytes();
  constexpr size_t entry_bytes=NativeAnimationResidency::kEntryBytes;
  NativeAnimationResidency residency(16*entry_bytes+2*bytes,16);
  for (uint32_t n=1; n<=16; ++n) Require(residency.Register(7,n*16),"dormant pack entries register without decoding curves");
  Require(residency.Bytes() == 16*entry_bytes && residency.ResidentCount() == 0 && !residency.Register(8,0x200),
          "catalog metadata and count are bounded inside the same aggregate cap");
  unsigned imports=0;
  auto import=[&](uint32_t,size_t budget){ ++imports; return animation_source::ReadKeyedAsset(0x1000,budget,read); };
  Require(!residency.Prepare(0x9999,10,import) && imports == 0,"unregistered source cannot trigger late discovery");
  Require(residency.Prepare(0x10,10,import) && residency.Prepare(0x20,10,import) && imports == 2 &&
          residency.Bytes() == 16*entry_bytes+2*bytes,"selected working set gets budget ahead of dormant load order");
  auto lease=residency.Find(0x10), alias=residency.Find(0x10);
  Require(lease == alias && !residency.Prepare(0x30,11,import) && imports == 2,
          "full active working set refuses before allocation; recent selections and aliases stay pinned");
  Require(residency.Prepare(0x30,13,import) && imports == 3 && residency.Find(0x10) == lease &&
          !residency.Find(0x20) && residency.Find(0x30),"evict only dormant unleased payloads, not live source registration");
  source.words.clear();
  for (uint32_t frame=14; frame<120; ++frame)
    Require(residency.Prepare(0x10,frame,import) && residency.Prepare(0x30,frame,import),
            "selected resident curves survive source destruction without reimport on slot ticks");
  Require(imports == 3,"steady-state preparation does not invoke the boundary decoder");
  residency.Retire(7);
  Require(residency.Size() == 0 && residency.Bytes() == bytes+entry_bytes &&
          !residency.Prepare(0x10,120,import) && imports == 3,"loader retirement removes dormant and resident lookup without resurrecting backing");
  std::vector<NativeJointChannels> channels;
  Require(lease->Clip().Sample(.25f,channels),"retired native motion lease remains source-free");
  lease.reset(); alias.reset(); Require(residency.Bytes() == 0,"retired index/payload charges release with last alias");

  source=CubicSource(); imports=0;
  NativeAnimationResidency tight(bytes+entry_bytes-1,1);
  Require(tight.Register(8,0x10),"register oversized candidate's lifetime within cap");
  for (uint32_t frame=0; frame<120; ++frame) Require(!tight.Prepare(0x10,frame,import),"oversized selected clip refuses");
  Require(imports == 1 && tight.Bytes() == entry_bytes,"unchanged failed budget cannot cause per-tick decode/allocation retries");
  Require(tight.Register(9,0x10) && !tight.Prepare(0x10,120,import) && imports == 2,
          "source-address generation reuse resets refusal without borrowing the retired payload");
}
void TestCompressedAnimations() {
  for (uint32_t bits=0; bits<=65535; ++bits) {
    const int exponent=(bits>>10)&31;
    float expected=exponent ? std::ldexp(1.0f+float(bits&1023)/1024,exponent-15) : 0;
    if (bits&0x8000) expected=-expected;
    Require(std::bit_cast<uint32_t>(animation_source::CompactFloat(uint16_t(bits))) == std::bit_cast<uint32_t>(expected),
            "all compact-float bit patterns match independent finite/flush-to-zero reference");
  }
  auto hermite = [](double a,double b,double ta,double tb,double duration,double phase) {
    const double square=phase*phase,cube=square*phase;
    return (2*cube-3*square+1)*a+(cube-2*square+phase)*duration*ta+
        (-2*cube+3*square)*b+(cube-square)*duration*tb;
  };
  for (float rate : {1.0f,2.0f,3.0f,10.0f}) {
    auto source=CubicSource(rate);
    auto read=[&](uint64_t address){return source.Read(address);};
    auto asset=animation_source::ReadKeyedAsset(0x1000,128*1024,read);
    Require(asset && asset->FindTrack(0xAA998877)->splines,"type3 loads owned per-axis cubic keys into existing clip asset");
    const auto bytes=asset->RetainedBytes();
    Require(animation_source::ReadKeyedAsset(0x1000,bytes,read).has_value() &&
            !animation_source::ReadKeyedAsset(0x1000,bytes-1,read),"cubic key/spline allocations debit exact residency budget");
    source.words.clear();
    std::vector<NativeJointChannels> channels;
    for (unsigned step=0; step<=60; ++step) {
      const float authored_seconds=float(step)/60;
      Require(asset->Clip().Sample(authored_seconds/rate,channels),"native cubic sampling after source destruction");
      const auto &sample=channels[2];
      const float expected=authored_seconds<=.5f ? float(hermite(0,4,3,-2,.5,authored_seconds*2)) :
          float(hermite(4,2,-2,7,.5,(authored_seconds-.5)*2));
      Require(Near(sample.translation[0],expected) && sample.translation[1] == -2 && sample.translation[2] == 0 &&
              Near(sample.scale[0],1+2*authored_seconds) && sample.scale[1] == 1 && sample.scale[2] == 1,
              "cubic translation/scale match independent Hermite basis across rates and segments");
      Require(sample.translated && sample.rotated && sample.scaled && !channels[0].rotated,
              "constant and compressed channels coexist with exact activation");
    }
    Require(asset->Clip().Sample((.5f/30)/rate,channels) && Near(channels[2].rotation[2],1),
            "adjacent packed angular keys override tangents and take the short arc");
    Require(asset->Clip().Sample((1.0f/30)/rate,channels) && channels[2].rotation[2]<0,
            "exact cubic angular endpoint preserves authored sign");
    Require(asset->Clip().Sample((15.5f/30)/rate,channels) && Near(channels[2].rotation[2],0),
            "long angular segments preserve authored cubic path instead of globally unwrapping");
    constexpr std::array names{0xBB776655u,0xCC332211u,0xAA998877u};
    std::vector<animation_source::ChannelRecord> records;
    Require(animation_source::ApplyKeyedAsset(*asset,names,.25f/rate,false,records) && records[2][0] == (128|7),
            "compressed motion reaches the existing runtime channel application and dirty contract");
  }
  auto refuse=[](auto mutate,const char *message) {
    auto source=CubicSource(); mutate(source);
    Require(!animation_source::ReadKeyedAsset(0x1000,128*1024,[&](uint64_t address){return source.Read(address);}),message);
  };
  refuse([](auto &s){s.Word(0x7000,0);},"nonnull empty scalar curve cannot read an imaginary predecessor");
  refuse([](auto &s){s.Word(0x7000,65537);},"cubic key count bounded before allocation");
  refuse([](auto &s){s.Half(0x8008,0);},"duplicate cubic times refuse transactionally");
  refuse([](auto &s){s.Word(0x8004,0x7fc00000);},"active cubic tangent must be finite");
  refuse([](auto &s){s.words.erase(0x8014);},"truncated cubic key refuses before source retirement");
  refuse([](auto &s){s.Word(0x7004,UINT32_MAX-6);},"cubic key address overflow refused");
}
void TestWeightedChannels() {
  NativeJointChannels previous, incoming, rest, result;
  rest.translated=rest.rotated=rest.scaled=true;
  rest.translation={10,20,30}; rest.scale={2,3,4};
  previous.reset_parent=true;
  incoming.translated=incoming.rotated=incoming.scaled=true;
  incoming.translation={2,4,6}; incoming.scale={4,5,6};
  incoming.rotation={0,0,1,0};
  Require(BlendNativeChannels(previous,incoming,rest,.25f,result) && result.reset_parent &&
          result.translation == JointVector{8,16,24} && result.scale == JointVector{2.5f,3.5f,4.5f} &&
          Near(result.rotation[2],std::sin(std::numbers::pi_v<float>/8)) &&
          Near(result.rotation[3],std::cos(std::numbers::pi_v<float>/8)),
          "inactive previous channels blend from authored rest, not identity/zero");
  previous=result; incoming=NativeJointChannels{};
  Require(BlendNativeChannels(previous,incoming,rest,.5f,result) && result.translated && result.rotated && result.scaled &&
          result.translation == JointVector{9,18,27} && result.scale == JointVector{2.25f,3.25f,4.25f},
          "missing incoming channels blend active previous values toward rest and remain enabled");
  Require(BlendNativeChannels(previous,incoming,rest,1,result) && !result.translated && !result.rotated && !result.scaled &&
          result.translation == previous.translation && result.rotation == previous.rotation,
          "unit weight clears absent activation without rewriting inactive values");
  const auto saved=result;
  Require(BlendNativeChannels(previous,incoming,{},0,result) && result.translation == previous.translation &&
          BlendNativeChannels(previous,incoming,{},-kNativeAnimationWeightEpsilon*.5f,result) &&
          result.translation == previous.translation,"exact dispatcher epsilon is a no-op even without rest");
  Require(!BlendNativeChannels(previous,incoming,{},kNativeAnimationWeightEpsilon,result),
          "epsilon boundary is not silently treated as no-op");
  result=saved;
  Require(!BlendNativeChannels(previous,incoming,{},.5f,result) && result.translation == saved.translation &&
          !BlendNativeChannels(previous,incoming,rest,std::numeric_limits<float>::infinity(),result),
          "missing required rest/nonfinite weight refuse transactionally");
  previous=NativeJointChannels{}; incoming=NativeJointChannels{};
  Require(BlendNativeChannels(previous,incoming,{},.5f,result) && !result.translated,
          "both channels absent need no rest value");
  previous.rotated=incoming.rotated=true; previous.rotation={0,0,0,1}; incoming.rotation={0,0,0,-1};
  Require(BlendNativeChannels(previous,incoming,{},.25f,result) && result.rotation == previous.rotation &&
          BlendNativeChannels(previous,incoming,{},1,result) && result.rotation == incoming.rotation,
          "shortest quaternion hemisphere except exact unit-weight authored sign");
  previous.rotation={0,0,0,2}; incoming.rotation={0,0,0,2};
  Require(BlendNativeChannels(previous,incoming,{},.5f,result) && result.rotation[3] == 2,
          "near-parallel blend does not normalize authored accumulated values");
  // Independent axis-angle reference over interpolation and extrapolation.
  previous.rotation={0,0,0,1};
  for (float angle : {.001f,.2f,1.4f,3.0f}) for (float weight : {-.25f,.1f,.5f,.9f,1.25f}) {
    incoming.rotation={0,std::sin(angle*.5f),0,std::cos(angle*.5f)};
    Require(BlendNativeChannels(previous,incoming,{},weight,result) &&
            Near(result.rotation[1],std::sin(angle*weight*.5f)) &&
            Near(result.rotation[3],std::cos(angle*weight*.5f)),
            "weighted quaternion channels match independent axis-angle reference");
  }
}
void TestWeightedLayerConsumption() {
  ClipSource source;
  auto asset=animation_source::ReadKeyedAsset(0x1000,128*1024,[&](uint64_t address){return source.Read(address);});
  Require(asset.has_value(),"weighted source admission"); source.words.clear();
  // Independent dense identity order; two roots and two sibling subtrees.
  constexpr std::array poses{4u,2u,0u,5u,1u,3u};
  constexpr std::array parents{kNativeSkeletonRoot,0u,1u,0u,3u,kNativeSkeletonRoot};
  constexpr std::array names{0xBB776655u,0x11111111u,0xAA998877u,0x33333333u,0x44444444u,0xCC332211u};
  std::vector<NativeSkeletonJoint> skeleton(6);
  for (size_t n=0; n<6; ++n) {
    auto &joint=skeleton[n]; joint.pose_index=poses[n]; joint.parent=parents[n];
    joint.blend_rest.translated=joint.blend_rest.rotated=joint.blend_rest.scaled=true;
    joint.blend_rest.translation={10,20,30}; joint.blend_rest.scale={2,2,2};
  }
  Require(SelectNativeAnimationSubtree(skeleton,2,true) == std::vector<uint8_t>{0,1,1,0,0,0} &&
          SelectNativeAnimationSubtree(skeleton,2,false) == std::vector<uint8_t>{0,1,1,1,1,0} &&
          SelectNativeAnimationSubtree(skeleton,4,false) == std::vector<uint8_t>{1,1,1,1,1,1} &&
          !SelectNativeAnimationSubtree(skeleton,99,true),"subtree and following-sibling masks use hierarchy, not dense identity order");
  std::vector<animation_source::ChannelRecord> records(6);
  for (auto &record : records) { record.fill(0x7FC01234); record[0]=64|8; }
  const auto before=records;
  Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,.25f,.5f,2,true,records),
          "owned weighted subtree applies after complete clip source destruction");
  Require(records[2][0] == (64|8|128|7) && std::bit_cast<float>(records[2][2]) == 6 &&
          std::bit_cast<float>(records[2][3]) == 10 && records[0][0] == (64|8|1) &&
          std::bit_cast<float>(records[0][2]) == 5.5f && records[0][5] == before[0][5],
          "weighted activation, missing channels, animated dirty bit and inactive payload preservation");
  for (unsigned pose : {1,3,4,5}) Require(records[pose] == before[pose],"unselected records remain byte-identical, including names");
  auto channels=skeleton_source::ReadChannels(0x8000,records.size(),[&](uint64_t address)->std::optional<uint32_t> {
    if (address<0x8000 || address>=0x8000+records.size()*48 || (address&3)) return {};
    const auto offset=address-0x8000; return records[offset/48][(offset%48)/4];
  });
  std::vector<RenderMatrix> pose;
  Require(channels && EvaluateNativeSkeleton(skeleton,*channels,JointIdentity(),pose) && pose[2][12] == 6,
          "weighted outgoing records drive the actual native skeleton evaluator");
  NativeInstanceRegistry instances; const auto id=instances.Create(91);
  Require(instances.Publish(id,0,pose) && instances.Transfer(id,0,1,6),"weighted native hierarchy reaches immutable instance consumer");
  const auto completed=instances.Read(id,1);
  const auto valid=records;
  auto broken=skeleton; broken[2].blend_rest.translated=false;
  records=before;
  Require(!animation_source::ApplyKeyedLayer(*asset,names,broken,.25f,.5f,2,true,records) && records == before,
          "late missing-rest failure rolls back earlier joint writes");
  Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,.25f,.5f,2,false,records) &&
          records[5][1] == names[5] && records[5][0] == before[5][0] && records[5][2] == before[5][2] &&
          records[1][1] == names[1] && records[1][0] == before[1][0] && records[3] == before[3],
          "following siblings and descendants get names; unmatched/empty channels stay untouched; other roots do not");
  auto unit=before, direct=before;
  Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,.25f,1,4,false,unit) &&
          animation_source::ApplyKeyedAsset(*asset,names,.25f,true,direct) && unit == direct,
          "unit full traversal preserves existing direct channel behavior exactly");
  records=valid;
  Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,.25f,0,2,true,records) && records == valid,
          "zero weight leaves all outgoing bytes untouched");
  asset.reset(); skeleton.clear(); instances.Retire(id);
  Require(completed && completed->transforms[2][12] == 6,"completed weighted pose survives all animation/model owners");
}
void TestOwnedBlendRest() {
  ClipSource source;
  for (uint32_t offset=0; offset<80; offset+=4) source.Word(0x9000+offset,0);
  const JointVector translation{10,20,30}, angles{0,0,std::numbers::pi_v<float>*.5f}, scale{2,3,4};
  for (unsigned n=0; n<3; ++n) {
    source.Word(0x9010+n*4,std::bit_cast<uint32_t>(translation[n]));
    source.Word(0x901c+n*4,std::bit_cast<uint32_t>(angles[n]));
    source.Word(0x902c+n*4,std::bit_cast<uint32_t>(scale[n]));
  }
  auto read=[&](uint64_t address){return source.Read(address);};
  auto skeleton=skeleton_source::ReadSkeleton(0x9000,read);
  Require(skeleton && (*skeleton)[0].translation == JointVector{} && (*skeleton)[0].scale == JointVector{1,1,1} &&
          (*skeleton)[0].blend_rest.translation == translation && (*skeleton)[0].blend_rest.scale == scale &&
          Near((*skeleton)[0].blend_rest.rotation[2],std::sqrt(.5f)),
          "rest values are captured even when base transform flags disable all channels");
  source.words.erase(0x9010);
  auto partial=skeleton_source::ReadSkeleton(0x9000,read);
  Require(partial && !(*partial)[0].blend_rest.translated && (*partial)[0].blend_rest.rotated,
          "unavailable optional rest channel does not invalidate unrelated base geometry");
  source.words.clear(); NativeJointChannels incoming,result; incoming.translated=true;
  Require(BlendNativeChannels({},incoming,(*skeleton)[0].blend_rest,.5f,result) &&
          result.translation == JointVector{5,10,15},"owned rest is independent of source lifetime and base activation");
}
void TestLayerMixing() {
  // All channel-presence combinations have independent literal expectations:
  // T/R copy a sole input while S blends a sole input with identity, not rest.
  for (uint32_t flags_a=0; flags_a<8; ++flags_a) for (uint32_t flags_b=0; flags_b<8; ++flags_b)
    for (float weight : {-1.0f,0.0f,kNativeAnimationWeightEpsilon*.5f,kNativeAnimationWeightEpsilon,.25f,1.0f,2.0f}) {
      std::array<animation_source::ChannelRecord,1> a{},b{};
      a[0][0]=flags_a|64; b[0][0]=flags_b|128; a[0][1]=91; b[0][1]=77;
      for (unsigned word=2; word<12; ++word) { a[0][word]=0x7fc01234; b[0][word]=0x7fc05678; }
      for (unsigned axis=0; axis<3; ++axis) {
        if (flags_a&1) a[0][2+axis]=std::bit_cast<uint32_t>(4.0f+axis);
        if (flags_b&1) b[0][2+axis]=std::bit_cast<uint32_t>(8.0f+axis);
        if (flags_a&4) a[0][9+axis]=std::bit_cast<uint32_t>(2.0f+axis);
        if (flags_b&4) b[0][9+axis]=std::bit_cast<uint32_t>(6.0f+axis);
      }
      for (unsigned axis=0; axis<4; ++axis) {
        if (flags_a&2) a[0][5+axis]=std::bit_cast<uint32_t>(axis == 3 ? 1.0f : 0.0f);
        if (flags_b&2) b[0][5+axis]=std::bit_cast<uint32_t>(axis == 3 ? 1.0f : 0.0f);
      }
      std::vector<animation_source::ChannelRecord> result;
      Require(animation_source::MixChannelRecords(a,b,weight,false,false,result) &&
              result[0][0] == (flags_a|flags_b|64|128) && result[0][1] == 91,
              "mixed output unions all flags and retains the first layer identity");
      auto blend=[&](float left,float right) { const float target=right*weight;
        return float(std::fma(double(left),double(1-weight),double(target))); };
      for (unsigned axis=0; axis<3; ++axis) {
        float t=0,s=1;
        if (flags_a&1) t=!(flags_b&1) || std::abs(weight)<kNativeAnimationWeightEpsilon ? 4.0f+axis :
            weight == 1 ? 8.0f+axis : blend(4.0f+axis,8.0f+axis);
        else if (flags_b&1) t=8.0f+axis;
        if ((flags_a&4) && (flags_b&4)) s=std::abs(weight)<kNativeAnimationWeightEpsilon ? 2.0f+axis :
            weight == 1 ? 6.0f+axis : blend(2.0f+axis,6.0f+axis);
        else if (flags_a&4) s=float(std::fma(double(2.0f+axis),double(1-weight),double(weight)));
        else if (flags_b&4) s=float(std::fma(double(6.0f+axis),double(weight),double(1-weight)));
        Require(result[0][2+axis] == std::bit_cast<uint32_t>(t) && result[0][9+axis] == std::bit_cast<uint32_t>(s),
                "layer T/S composition and inactive canonical defaults match independent source arithmetic");
      }
      for (unsigned axis=0; axis<4; ++axis)
        Require(result[0][5+axis] == std::bit_cast<uint32_t>(axis == 3 ? 1.0f : 0.0f),
                "both absent quaternion inputs produce inactive identity without reading poisoned bytes");
    }
  std::vector<animation_source::ChannelRecord> a(1),b(1),out;
  b[0][0]=7; a[0][1]=91; a[0][8]=b[0][8]=std::bit_cast<uint32_t>(1.0f);
  for (unsigned axis=0; axis<3; ++axis) {
    a[0][2+axis]=a[0][9+axis]=std::bit_cast<uint32_t>(10.0f);
    b[0][2+axis]=b[0][9+axis]=std::bit_cast<uint32_t>(2.0f);
  }
  Require(animation_source::MixChannelRecords(a,b,.5f,false,false,out) &&
          std::bit_cast<float>(out[0][2]) == 2 && std::bit_cast<float>(out[0][9]) == 1.5f,
          "distinct output uses actual activation, not stale inactive values");
  Require(animation_source::MixChannelRecords(a,b,.5f,true,false,out) &&
          std::bit_cast<float>(out[0][2]) == 6 && std::bit_cast<float>(out[0][9]) == 6,
          "in-place left output preserves union-flags-before-input-read ABI");
  Require(animation_source::MixChannelRecords(b,a,.5f,false,true,out) &&
          std::bit_cast<float>(out[0][2]) == 6 && std::bit_cast<float>(out[0][9]) == 6,
          "in-place right output preserves the same flag publication ordering");
  Require(animation_source::MixChannelRecords(b,b,.5f,true,true,out) &&
          std::bit_cast<float>(out[0][2]) == 2,"all three buffers may alias exactly");
  const auto valid=out; b[0][2]=0x7fc01234;
  Require(!animation_source::MixChannelRecords(a,b,.5f,false,false,out) && out == valid,
          "nonfinite active layer refuses before any outgoing mutation");
  std::array<NativeSkeletonJoint,1> skeleton;
  auto channels=skeleton_source::ReadChannels(0x8000,1,[&](uint64_t address)->std::optional<uint32_t> {
    if (address<0x8000 || address>=0x8030 || (address&3)) return {};
    return valid[0][(address-0x8000)/4];
  });
  std::vector<RenderMatrix> pose;
  Require(channels && EvaluateNativeSkeleton(skeleton,*channels,JointIdentity(),pose) && pose[0][12] == 2,
          "mixed layer records connect to the production native hierarchy consumer");
}
void TestConstantTimesAndScaleTail() {
  for (uint16_t type : {2,3}) {
    ClipSource source;
    source.Half(0x1006,type);
    // Make every enabled channel constant, with ignored negative timestamps.
    for (uint32_t count : {0x2054u,0x2056u,0x2058u}) source.Half(count,1);
    for (uint32_t key : {0x3000u,0x4000u,0x5000u,0x6000u}) source.Half(key,65535);
    source.words.erase(0x5000); // T timestamp/pad can be unavailable entirely.
    auto asset=animation_source::ReadKeyedAsset(0x1000,128*1024,[&](uint64_t address){return source.Read(address);});
    Require(asset.has_value(),"constant channels do not read or validate ignored timestamps");
    source.words.clear(); std::vector<NativeJointChannels> channels;
    Require(asset->Clip().Sample(.75f,channels) && channels[1].translation == JointVector{1,2,3} &&
            channels[2].scale == JointVector{1,1,1} && asset->FindTrack(0xAA998877)->translation.front().seconds == 0,
            "constant source-free values hold for type2 and type3 with canonical native time");
  }
  ClipSource source;
  source.Half(0x6010,10); source.Half(0x6020,20);
  auto clip=Import(source); source.words.clear();
  std::vector<NativeJointChannels> channels;
  Require(clip && clip->Sample(.9f,channels) && channels[2].scale == JointVector{5,5,5},
          "bounded scale scan holds a terminal key before clip end without importing an imaginary key");
  source=ClipSource{}; source.words.erase(0x302c);
  animation_source::ImportTrace trace;
  uint64_t first_missing=0;
  Require(!animation_source::ReadKeyedAsset(0x1000,128*1024,[&](uint64_t address) {
    const auto value=source.Read(address); if (!value && !first_missing) first_missing=address; return value;
  },&trace) && std::string_view(trace.stage) == "key-value" && trace.track == 2 && trace.channel == 0 &&
          trace.key == 2 && first_missing == 0x302c,"import refusal records bounded validation stage and exact missing word");
  source=ClipSource{};
  Require(animation_source::ReadKeyedAsset(0x1000,128*1024,[&](uint64_t address){return source.Read(address);},&trace).has_value() &&
          std::string_view(trace.stage) == "complete","reused diagnostic cannot mislabel a successful import");
}
void TestNamedAnimationSelection() {
  ClipSource source;
  auto asset=animation_source::ReadKeyedAsset(0x1000,128*1024,[&](uint64_t address){return source.Read(address);});
  Require(asset.has_value(),"named selection clip admission");
  std::array<NativeSkeletonJoint,4> skeleton;
  constexpr std::array labels{std::string_view("root"),std::string_view("arm"),std::string_view("finger"),std::string_view("other")};
  constexpr std::array poses{2u,0u,1u,3u}, parents{kNativeSkeletonRoot,0u,1u,kNativeSkeletonRoot};
  constexpr std::array names{0xBB776655u,0xCC332211u,0xAA998877u,0x12345678u};
  for (size_t n=0; n<skeleton.size(); ++n) {
    auto &joint=skeleton[n]; joint.pose_index=poses[n]; joint.parent=parents[n];
    for (size_t b=0; b<=labels[n].size(); ++b) source.Byte(0x9000+n*16+b,b == labels[n].size() ? 0 : uint8_t(labels[n][b]));
    joint.animation_name=skeleton_source::ReadJointName(0x9000+n*16,[&](uint64_t address){return source.Read(address);});
    Require(joint.animation_name.Valid() && joint.animation_name.View() == labels[n],"bounded owned inline joint name import");
    joint.blend_rest.translated=joint.blend_rest.rotated=joint.blend_rest.scaled=true;
    joint.blend_rest.translation={10,20,30};
  }
  source.words.clear(); // Filters and model names cannot borrow source storage.
  const NativeAnimationFilter direct{"arm",{},true}, weighted{"arm",{},false};
  auto selected=SelectNativeAnimationNodes(skeleton,2,false,direct);
  Require(selected && selected->headers == std::vector<uint8_t>{1,1,1,1} &&
          selected->channels == std::vector<uint8_t>{0,1,1,0},"direct named traversal searches through unmatched parents and writes their headers");
  selected=SelectNativeAnimationNodes(skeleton,2,false,weighted);
  Require(selected && selected->headers == std::vector<uint8_t>{0,0,0,0} &&
          selected->channels == std::vector<uint8_t>{0,0,0,0},"weighted named traversal prunes unmatched parents, not a deep name search");
  std::vector<animation_source::ChannelRecord> records(4);
  for (auto &record : records) { record.fill(0x7fc01234); record[0]=64; }
  const auto before=records;
  Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,.25f,1,2,false,records,direct) &&
          records[2][0] == 64 && records[2][1] == names[2] && records[2][2] == before[2][2] &&
          records[0][0] == (64|1) && std::bit_cast<float>(records[0][2]) == 1 &&
          records[1][0] == 64 && records[1][1] == names[1],"named direct application preserves filtered channels but updates traversal headers");
  Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,.25f,1,2,false,records,direct,true) &&
          records[2][0] == 0 && records[2][1] == names[2] && records[2][2] == 0 && records[0][0] == 1,
          "whole reset clears filtered bytes before the same named direct traversal");
  records=before;
  Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,.25f,.5f,2,false,records,weighted) && records == before,
          "weighted unmatched branch leaves even names/dirty flags untouched");
  constexpr std::array exclusions{std::string_view("arm")};
  const NativeAnimationFilter excluded{{},exclusions,false}, priority{"arm",exclusions,false};
  Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,.25f,.5f,0,true,records,priority) &&
          std::bit_cast<float>(records[0][2]) == 5.5f && records[2] == before[2] && records[3] == before[3],
          "explicit included name overrides exclusions and forced subtree never reaches outside records");
  records=before;
  Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,.25f,.5f,2,false,records,excluded) &&
          records[0] == before[0] && records[1] == before[1] && std::bit_cast<float>(records[2][2]) == 6,
          "excluded node and its descendants remain byte-identical while other roots animate");
  auto invalid=skeleton; invalid[0].animation_name=NativeJointName{};
  Require(!SelectNativeAnimationNodes(invalid,2,false,direct) && SelectNativeAnimationNodes(invalid,2,false,{}),
          "missing optional name refuses only consumers requiring name comparisons");
  std::array<std::string_view,31> excessive;
  Require(!SelectNativeAnimationNodes(skeleton,2,false,{{},excessive,false}),"exclusion allocation/traversal stays within the actual30-entry source cap");
  unsigned reads=0;
  auto read=[&](uint64_t address){++reads; return source.Read(address);};
  Require(!skeleton_source::ReadJointName(UINT32_MAX-14,read).Valid() && reads == 0,"inline name address cannot wrap");
  for (uint32_t n=0; n<16; ++n) source.Byte(0x9000+n,'x');
  Require(!skeleton_source::ReadJointName(0x9000,read).Valid(),"unterminated inline name never reads adjacent node fields");
  source.Byte(0x900f,0);
  Require(skeleton_source::ReadJointName(0x9000,read).View().size() == 15,"full bounded15-byte authored name is preserved");
}
void TestFirstMatchAnimationDescriptors() {
  ClipSource source;
  source.Half(0x1008,4); source.Word(0x206c+18,0xBB776655);
  // Shadowed descriptor has no readable channel pointers/counts at all.
  bool read_shadowed=false;
  auto read=[&](uint64_t address) {
    read_shadowed |= address>=0x206c && address<0x207c;
    return source.Read(address);
  };
  animation_source::ImportTrace trace;
  auto asset=animation_source::ReadKeyedAsset(0x1000,128*1024,read,&trace);
  Require(asset && !read_shadowed && trace.descriptors == 4 && trace.unique_names == 3 &&
          asset->FindTrack(0xBB776655)->translation[0].value == JointVector{1,2,3},
          "duplicate names keep first descriptor and never touch shadowed curve storage");
  const auto bytes=asset->RetainedBytes();
  Require(animation_source::ReadKeyedAsset(0x1000,bytes,read).has_value() &&
          !animation_source::ReadKeyedAsset(0x1000,bytes-1,read),"canonical target/key capacity obeys exact aggregate budget");
  source.words.clear();
  constexpr std::array model_names{0xAA998877u,0xBB776655u,0xCC332211u};
  std::vector<animation_source::ChannelRecord> output;
  Require(animation_source::ApplyKeyedAsset(*asset,model_names,.25f,false,output) &&
          std::bit_cast<float>(output[1][2]) == 1,"canonical named asset applies after source destruction");

  // Independent original cursor reference: 243 descriptor-name sequences and
  // all six unique model traversal orders. The cursor advances only on an exact
  // current-position match; weighted traversal always scans from zero.
  for (unsigned pattern=0; pattern<243; ++pattern) {
    source.words.clear(); source.Word(0x1000,0x2000); source.Half(0x1004,30);
    source.Half(0x1006,2); source.Half(0x1008,5); source.Word(0x100c,std::bit_cast<uint32_t>(1.0f));
    unsigned digits=pattern; std::array<uint32_t,5> descriptor_names;
    for (uint32_t n=0; n<5; ++n) {
      descriptor_names[n]=digits%3+1; digits/=3;
      const auto record=0x2000+n*36, key=0x4000+n*16;
      source.Word(record,key); source.Word(record+4,0); source.Word(record+8,0);
      source.Half(record+12,1); source.Half(record+14,0); source.Half(record+16,0);
      source.Word(record+18,descriptor_names[n]); source.Vector(key,0,{float(n+1),0,0});
    }
    asset=animation_source::ReadKeyedAsset(0x1000,128*1024,read);
    Require(asset.has_value(),"duplicate descriptor sequence imports as an owned first-match asset");
    std::array<uint32_t,3> order{1,2,3};
    do {
      size_t cursor=0;
      for (auto name : order) {
        size_t found=cursor;
        while (found<descriptor_names.size() && descriptor_names[found] != name) ++found;
        const auto *track=asset->FindTrack(name);
        Require((found == descriptor_names.size()) == (track == nullptr),"canonical binding matches original cursor presence");
        if (track) Require(track->translation.front().value[0] == float(found+1),"first-match binding equals direct cursor and weighted scan for unique model names");
        if (found == cursor) ++cursor;
      }
    } while (std::next_permutation(order.begin(),order.end()));
  }
}
void TestIndexedAnimationAssets() {
  constexpr std::array names{0xBB776655u,0xCC332211u,0xAA998877u};
  std::array<NativeSkeletonJoint,3> skeleton;
  skeleton[0].pose_index=2; skeleton[1].pose_index=0; skeleton[1].parent=0;
  skeleton[2].pose_index=1; skeleton[2].parent=0;
  for (auto &joint : skeleton) {
    joint.blend_rest.translated=joint.blend_rest.rotated=joint.blend_rest.scaled=true;
    joint.blend_rest.translation={10,0,0}; joint.blend_rest.scale={2,2,2};
  }
  for (uint16_t type : {0,1}) {
    ClipSource source, named_source;
    for (uint32_t address=0x2000; address<0x2080; address+=4) source.words.erase(address);
    source.Half(0x1006,type);
    const uint32_t stride=type == 0 ? 12 : 20, counts=type == 0 ? 8 : 12;
    for (uint32_t n=0; n<3; ++n) {
      const auto record=0x2000+n*stride;
      source.Word(record,n == 0 ? 0x5000 : n == 2 ? 0x3000 : 0);
      source.Word(record+4,n == 2 ? 0x4000 : 0);
      source.Half(record+counts,n == 0 ? 1 : n == 2 ? 3 : 65535);
      source.Half(record+counts+2,n == 2 ? 3 : 0);
      if (type == 1) { source.Word(record+8,n == 2 ? 0x6000 : 0); source.Half(record+16,n == 2 ? 3 : 0); }
    }
    if (type == 0) { named_source.Word(0x2050,0); named_source.Half(0x2058,0); }
    auto read=[&](uint64_t address){ return source.Read(address); };
    animation_source::ImportTrace trace;
    auto asset=animation_source::ReadKeyedAsset(0x1000,128*1024,read,&trace);
    auto named=animation_source::ReadKeyedAsset(0x1000,128*1024,[&](uint64_t address){return named_source.Read(address);});
    Require(asset && named && asset->Indexed() && !named->Indexed() && asset->ChannelMask() == (type == 0 ? 3u : 7u) &&
            trace.descriptors == 3 && trace.unique_names == 3,"12/20-byte indexed descriptors import without reading names or absent scale fields");
    Require(!asset->FindTrack(0) && asset->FindTrack(999,0)->translation.front().value == JointVector{1,2,3} &&
            !asset->FindTrack(0,3),"indexed lookup requires a bounded joint identity, never a name or preorder ordinal");
    const auto bytes=asset->RetainedBytes();
    Require(animation_source::ReadKeyedAsset(0x1000,bytes,read).has_value() &&
            !animation_source::ReadKeyedAsset(0x1000,bytes-1,read),"indexed metadata and curves share exact retained-byte accounting");
    auto invalid=source; invalid.words.erase(0x302c);
    Require(!animation_source::ReadKeyedAsset(0x1000,128*1024,[&](uint64_t address){return invalid.Read(address);}),
            "indexed import refuses incomplete active keys transactionally");
    invalid=source; invalid.Word(0x1000,UINT32_MAX-stride);
    Require(!animation_source::ReadKeyedAsset(0x1000,128*1024,[&](uint64_t address){return invalid.Read(address);}),
            "indexed descriptor extent cannot wrap");
    source.words.clear(); named_source.words.clear();
    for (unsigned step=0; step<=128; ++step) {
      std::vector<animation_source::ChannelRecord> actual(3), expected(3);
      Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,float(step)/128,1,2,false,actual,{},true) &&
              animation_source::ApplyKeyedLayer(*named,names,skeleton,float(step)/128,1,2,false,expected,{},true) &&
              actual == expected,"indexed and named source-free imports reach identical whole channels through reordered native joints");
      animation_source::ControllerLayer working(3);
      Require(working.Apply(*asset,names,skeleton,float(step)/128,1,2,false,{},true) && working.Encode() == actual,
              "native controller working channels match the indexed outgoing ABI without intermediate packing");
    }
    std::vector<animation_source::ChannelRecord> records(3);
    for (size_t n=0; n<3; ++n) {
      auto &record=records[n]; record.fill(0x7fc01234); record[0]=64|1|4; record[1]=uint32_t(0xDEAD0000+n);
      record[2]=std::bit_cast<uint32_t>(20.0f); record[3]=record[4]=0;
      if (type == 1) for (unsigned word=9; word<12; ++word) record[word]=std::bit_cast<uint32_t>(4.0f);
    }
    const auto before=records;
    auto working=animation_source::ControllerLayer::Decode(before);
    const std::array<animation_source::ChannelRecord,1> root_input{before[2]};
    auto root=animation_source::SampleRootMotion(*asset,names[2],skeleton[0],.25f,
        animation_source::ControllerLayer::Decode(root_input));
    auto root_expected=before[2];
    root_expected[0]=(root_expected[0]&~(type == 0 ? 3u : 7u))|1u;
    for (unsigned axis=0; axis<3; ++axis) root_expected[2+axis]=std::bit_cast<uint32_t>(float(axis+1));
    Require(root && root->Encode()[0] == root_expected,
            "indexed one-joint request uses descriptor zero, not model pose two; TR masks and dormant payloads survive");
    const std::array<std::string_view,31> invalid_exclusions{};
    const NativeAnimationFilter ignored{"an overlong ignored indexed filter",invalid_exclusions,false};
    Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,.25f,.5f,0,true,records,ignored) &&
            records[2] == before[2] && std::bit_cast<float>(records[0][2]) == 10.5f &&
            std::bit_cast<float>(records[1][2]) == 15.0f && records[0][1] == before[0][1] && records[1][1] == before[1][1],
            "indexed weighted traversal ignores keyed-only filters/forced subtree and preserves headers while including following siblings");
    if (type == 0) {
      Require(records[0][0] == before[0][0] && records[0][9] == before[0][9] && records[1][11] == before[1][11],
              "TR-only layer does not read, blend, clear or rewrite even nonfinite active scale bytes");
    } else {
      Require(std::bit_cast<float>(records[0][9]) == 3 && std::bit_cast<float>(records[1][9]) == 3,
              "indexed missing scale blends against owned authored rest scale");
    }
    Require(working.Apply(*asset,names,skeleton,.25f,.5f,0,true,ignored,false) && working.Encode() == records,
            "working layers preserve ignored TR-only NaN scale payloads, activation, filters and headers");
    records=before;
    Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,.25f,1,2,false,records) &&
            records[0][1] == before[0][1] && !(records[1][0]&1) && (records[2][0]&128),
            "full-weight indexed preserve path retains headers, clears absent channels and marks animated curves");
    const std::array duplicate_names{1u,1u,1u};
    Require(animation_source::ApplyKeyedLayer(*asset,duplicate_names,skeleton,.25f,1,2,false,records,{},true) &&
            std::bit_cast<float>(records[0][2]) == 1 && std::bit_cast<float>(records[2][2]) == 2,
            "indexed consumer does not introduce a false unique-name binding requirement");
    auto enlarged=skeleton; std::vector<NativeSkeletonJoint> too_many(enlarged.begin(),enlarged.end());
    NativeSkeletonJoint extra; extra.pose_index=3; too_many.push_back(extra);
    std::vector<animation_source::ChannelRecord> oversized(4); const auto untouched=oversized;
    const std::array four_names{1u,2u,3u,4u};
    Require(!animation_source::ApplyKeyedLayer(*asset,four_names,too_many,.25f,1,2,false,oversized,{},true) && oversized == untouched,
            "a model selecting beyond indexed clip storage refuses before publishing partial channels");
    NativeAnimationResidency residency(bytes+NativeAnimationResidency::kEntryBytes);
    Require(residency.Publish(7,9,std::move(*asset)),"indexed clips use the existing bounded residency owner");
    auto lease=residency.Find(9); residency.Retire(7);
    Require(lease && residency.Bytes() == bytes+NativeAnimationResidency::kEntryBytes,
            "retired indexed assets remain charged while leased");
    std::vector<animation_source::ChannelRecord> channels(3);
    Require(animation_source::ApplyKeyedLayer(*lease,names,skeleton,.25f,1,2,false,channels,{},true),
            "retired indexed asset produces channels without source or catalog");
    auto native=skeleton_source::ReadChannels(0x8000,3,[&](uint64_t address)->std::optional<uint32_t>{
      if (address < 0x8000 || address >= 0x8090 || (address&3)) return {};
      return channels[(address-0x8000)/48][((address-0x8000)%48)/4];
    });
    std::vector<RenderMatrix> pose;
    Require(native && EvaluateNativeSkeleton(skeleton,*native,JointIdentity(),pose),"indexed channels feed the production native hierarchy");
    NativeInstanceRegistry instances; const auto instance=instances.Create(91);
    Require(instances.Publish(instance,0,pose) && instances.Transfer(instance,0,1,3),"indexed hierarchy reaches immutable native render-pose ownership");
    const auto retained=instances.Read(instance,1); instances.Retire(instance); lease.reset();
    Require(residency.Bytes() == 0 && retained && retained->transforms == pose,
            "published indexed poses outlive source, asset and instance retirement");
  }
}

NativeJointName ControllerName(std::string_view text) {
  NativeJointName name{{},uint8_t(text.size())};
  std::ranges::copy(text,name.bytes.begin()); return name;
}
void TestSelectedTrackAndRootMotion() {
  constexpr std::array names{0xBB776655u,0xCC332211u,0xAA998877u};
  std::array<NativeSkeletonJoint,3> skeleton;
  skeleton[0].pose_index=2; skeleton[1].pose_index=0; skeleton[1].parent=0;
  skeleton[2].pose_index=1; skeleton[2].parent=0;
  for (bool cubic : {false,true}) {
    auto source=cubic ? CubicSource() : ClipSource();
    auto asset=animation_source::ReadKeyedAsset(0x1000,128*1024,[&](uint64_t address){return source.Read(address);});
    Require(asset.has_value(),"selected sampling imports existing keyed/cubic representations");
    source.words.clear();
    std::vector<animation_source::ChannelRecord> initial(3);
    for (auto &record : initial) { record.fill(0x7fc01234); record[0]=256|64|7; record[1]=0xDEADBEEF; }
    for (int step=-1; step<=129; ++step) {
      const float seconds=float(step)/128;
      auto expected=initial;
      Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,seconds,1,2,false,expected),
              "whole-layer reference evaluates source-free keyed/cubic root records");
      for (const auto &joint : skeleton) {
        const auto index=joint.pose_index;
        const std::array<animation_source::ChannelRecord,1> input{initial[index]};
        auto root=animation_source::SampleRootMotion(*asset,names[index],joint,seconds,
            animation_source::ControllerLayer::Decode(input));
        Require(root && root->Encode()[0] == expected[index],
                "selected root matches all whole-layer words through clamped endpoints, interior samples and dormant payloads");
      }
    }
    const std::array<animation_source::ChannelRecord,1> input{initial[2]};
    auto missing=animation_source::SampleRootMotion(*asset,0x1234,skeleton[0],.25f,
        animation_source::ControllerLayer::Decode(input));
    auto expected=input[0]; expected[1]=0x1234;
    Require(missing && missing->Encode()[0] == expected,
            "missing named root track changes only the authored header and preserves even active dormant bytes");
    Require(!animation_source::SampleRootMotion(*asset,names[2],skeleton[0],NAN,
                animation_source::ControllerLayer::Decode(input)) &&
            !animation_source::SampleRootMotion(*asset,names[2],skeleton[0],0,animation_source::ControllerLayer(2)),
            "root refuses nonfinite clocks and non-single output extents");
    NativeJointChannels selected; selected.translation={42,43,44};
    Require(!asset->SampleTarget(0x1234,2,0,selected) && selected.translation == JointVector{42,43,44},
            "missing selected target never modifies caller output");
  }
  ClipSource source;
  auto clip=Import(source); source.words.clear();
  Require(clip.has_value(),"model-bound clip for nonordinal pose selection");
  std::vector<NativeJointChannels> all;
  Require(clip->Sample(.25f,all),"whole clip sampling remains source-free");
  for (size_t ordinal=0; ordinal<clip->Tracks().size(); ++ordinal) {
    NativeJointChannels selected;
    Require(clip->SampleTrack(ordinal,.25f,selected),"select immutable clip ordinal");
    const auto &expected=all[clip->Tracks()[ordinal].pose_index];
    Require(selected.translation == expected.translation && selected.rotation == expected.rotation && selected.scale == expected.scale &&
            selected.translated == expected.translated && selected.rotated == expected.rotated && selected.scaled == expected.scaled,
            "selected ordinal respects sparse/reordered model pose identities");
  }
  NativeJointChannels sentinel; sentinel.translation={42,43,44};
  Require(!clip->SampleTrack(clip->Tracks().size(),0,sentinel) && !clip->SampleTrack(0,INFINITY,sentinel) &&
          sentinel.translation == JointVector{42,43,44},"invalid ordinal/time refuses transactionally");

  std::vector<NativeAnimationTrack> tracks(2);
  tracks[0].translation.push_back({0,{1,2,3}}); tracks[1].pose_index=1;
  auto splines=std::make_unique<NativeAnimationSplines>(); splines->translation.active=true;
  splines->translation.axes[0]={{0,0,std::numeric_limits<float>::max(),false},{10000,0,-std::numeric_limits<float>::max(),false}};
  tracks[1].splines=std::move(splines);
  auto selective_clip=NativeAnimationClip::Create(2,10000,std::move(tracks),128*1024);
  Require(selective_clip.has_value(),"finite imported curves can overflow only when evaluated");
  auto selective=NativeAnimationAsset::Create(std::move(*selective_clip),{1,2},128*1024);
  Require(selective.has_value(),"selected tracks reuse the same bounded native asset");
  std::array<NativeSkeletonJoint,2> siblings; siblings[1].pose_index=1;
  const std::array selected_names{1u,2u};
  animation_source::ControllerLayer layer(2);
  Require(!selective->Clip().Sample(5000,all) && layer.Apply(*selective,selected_names,siblings,5000,1,0,true,{},false) &&
          layer.channels[0].translation == JointVector{1,2,3} && !layer.channels[1].translated,
          "selected subtree does not evaluate an unrelated overflowing curve or allocate whole-clip samples");
  const auto before=layer.Encode();
  Require(!layer.Apply(*selective,selected_names,siblings,5000,1,1,true,{},false) && layer.Encode() == before,
          "selected overflow still refuses without partial layer publication");
  const std::array missing_names{3u,4u};
  Require(!layer.Apply(*selective,missing_names,siblings,NAN,1,0,true,{},false) && layer.Encode() == before,
          "missing named tracks cannot bypass nonfinite clock refusal");
}
void TestNativeControllerPlan() {
  Require(animation_source::ControllerSourceExtentFits(512,6,false) &&
          !animation_source::ControllerSourceExtentFits(513,2,false) &&
          animation_source::ControllerSourceExtentFits(4096,2,true) &&
          animation_source::ControllerSourceExtentFits(4096,1,false) &&
          !animation_source::ControllerSourceExtentFits(4097,1,false),
          "source multi-layer overlap is refused without imposing console stride limits on native sequential plans");
  const auto offsets=AdvanceNativeAnimationOffsets({1.75f,-1.75f,2,0},{.5f,-.5f,0,std::numeric_limits<float>::denorm_min()});
  Require(offsets[0] == .25f && offsets[1] == -.25f && std::bit_cast<uint32_t>(offsets[2]) == 0x80000000u &&
          std::bit_cast<uint32_t>(offsets[3]) == 0x80000000u,"native UV phase preserves signed fractional wrap and vector denormal flush");
  NativeAnimationControllerInput input; input.active=3; input.delta_ticks=1;
  for (auto &slot : input.slots) {
    slot.present=true; slot.loop=true; slot.weight=.5f; slot.target_weight=1; slot.weight_rate=.125f;
    slot.time_ticks=29; slot.time_rate=2; slot.duration_ticks=30; slot.contribution=1;
  }
  input.slots[0].contribution=2; input.slots[1].contribution=3; input.slots[2].contribution=5;
  auto plan=PlanNativeAnimationController(input);
  Require(plan && plan->count == 7 && plan->slots[0].loops == 1 && plan->slots[0].time_ticks == 1 &&
          plan->slots[0].weight == .625f,"native plan owns all active clocks and complete three-layer composition");
  Require(plan->steps[4].kind == NativeAnimationStep::Kind::Mix && plan->steps[4].weight == .6f &&
          plan->steps[4].left == 0 && plan->steps[5].weight == .625f && plan->steps[5].left == kNativeAnimationCombined &&
          plan->steps[6].weight == .625f,"three-layer order retains adjacent-slot denominator and final base weight");
  input.slots[0].loops=UINT32_MAX; input.slots[0].time_ticks=100;
  plan=PlanNativeAnimationController(input);
  Require(plan && plan->slots[0].loops == 0 && plan->slots[0].time_ticks == 72 && plan->steps[0].seconds == 1,
          "clock wraps unsigned loop count once, retains overshoot state, clamps only whole sampling");
  input.slots[0].loop=false; input.slots[0].weight=-2; input.slots[0].weight_rate=-.25f;
  plan=PlanNativeAnimationController(input);
  Require(plan && plan->slots[0].time_ticks == 30 && plan->slots[0].weight == -2.25f,
          "nonlooping clocks clamp upper duration; negative weight ramps are not clamped to zero");
  auto broken=input; broken.slots[1].present=false;
  Require(!PlanNativeAnimationController(broken),"positive absent layer refuses stale scratch before any publication");
  broken=input; broken.active=0;
  Require(!PlanNativeAnimationController(broken),"empty multi-layer plan must not invent stale combined scratch");
  broken=input; broken.active=7;
  Require(!PlanNativeAnimationController(broken),"physical slot bounds are checked");
  broken=input; broken.delta_ticks=std::numeric_limits<float>::infinity();
  Require(!PlanNativeAnimationController(broken),"nonfinite clocks refuse transactionally");
  input.active=2; input.sequential=true; input.overlay_count=3;
  input.overlays[1]=ControllerName("arm"); input.overlays[2]=ControllerName("root");
  input.slots[0].weight=.5f; input.slots[0].weight_rate=0;
  plan=PlanNativeAnimationController(input);
  Require(plan && plan->count == 4 && plan->slots[1].time_ticks == 3 && plan->slots[1].weight == .75f &&
          plan->slots[2].time_ticks == 1 && plan->steps[2].subtree && plan->steps[2].root.View() == "arm",
          "active overlay advances twice in original order; separate overlay advances once with owned name");
  input.active=1; input.overlay_count=0; input.slots[0].time_ticks=0; input.slots[0].time_rate=-2;
  plan=PlanNativeAnimationController(input);
  Require(plan && plan->slots[0].time_ticks == -2 && plan->steps[0].seconds == 0,
          "negative authored clock survives while slot sampling independently clamps to zero");
  input.slots[0].time_ticks=30; input.slots[0].time_rate=0; input.slots[0].loop=true; input.slots[0].loops=4;
  plan=PlanNativeAnimationController(input);
  Require(plan && plan->slots[0].loops == 4,"exact duration does not increment loop counter");
  input.slots[0].weight=-1; input.replace_slots=true;
  plan=PlanNativeAnimationController(input);
  Require(plan && plan->count == 1 && plan->steps[0].reset && plan->steps[0].weight == 1,
          "replacement mode samples even a negative-weight slot");
  std::array<bool,6> late_enabled{}; late_enabled[3]=late_enabled[4]=late_enabled[5]=true;
  auto late=PlanNativeAnimationLateLayers(input.slots,late_enabled,false,ControllerName("arm"));
  Require(late && late->count == 3 && late->steps[0].slot == 4 && late->steps[1].slot == 5 && late->steps[2].slot == 3 &&
          late->slots[4].time_ticks == 1 && late->slots[4].weight == .625f && late->steps[0].included.View() == "arm",
          "late layers advance with unit step in authored 4,5,3 order, inheriting global inclusion");
  late_enabled[5]=false;
  input.slots[3].present=false; input.slots[4].weight=-2; input.slots[4].time_ticks=-4;
  late=PlanNativeAnimationLateLayers(input.slots,late_enabled,false,ControllerName(""));
  Require(late && late->count == 0 && late->advanced[4] && !late->advanced[5] && !late->advanced[3] &&
          late->slots[4].time_ticks == -2 && late->slots[4].weight == -1.875f,
          "disabled/absent late slots do not advance; negative-weight slot advances but does not sample");
  late=PlanNativeAnimationLateLayers(input.slots,late_enabled,true,ControllerName(""));
  Require(late && late->count == 1 && late->steps[0].reset && late->steps[0].seconds == 0 && late->steps[0].weight == 1,
          "late replacement samples nonpositive weight with ordinary clip-domain clamp");
  input.slots[4].time_rate=std::numeric_limits<float>::infinity();
  Require(!PlanNativeAnimationLateLayers(input.slots,late_enabled,false,ControllerName("")),
          "nonfinite late clock refuses before any earlier slot can publish");
}
void TestNativeControllerConsumption() {
  ClipSource source;
  auto imported=animation_source::ReadKeyedAsset(0x1000,128*1024,[&](uint64_t address){ return source.Read(address); });
  Require(imported.has_value(),"controller imports through production asset boundary");
  auto asset=std::make_shared<const NativeAnimationAsset>(std::move(*imported));
  source.words.clear();
  constexpr std::array names{0xBB776655u,0xCC332211u,0xAA998877u};
  std::array<NativeSkeletonJoint,3> skeleton;
  skeleton[0].pose_index=2; skeleton[0].animation_name=ControllerName("root");
  skeleton[1].pose_index=0; skeleton[1].parent=0; skeleton[1].animation_name=ControllerName("arm");
  skeleton[2].pose_index=1; skeleton[2].parent=0; skeleton[2].animation_name=ControllerName("hand");
  for (auto &joint : skeleton) {
    joint.blend_rest.translated=joint.blend_rest.rotated=joint.blend_rest.scaled=true;
    joint.blend_rest.translation={10,0,0};
  }
  std::array<std::shared_ptr<const NativeAnimationAsset>,6> assets; assets.fill(asset);
  NativeAnimationControllerInput input; input.active=3; input.delta_ticks=.5f;
  for (size_t n=0; n<6; ++n) {
    auto &slot=input.slots[n]; slot.present=true; slot.weight=.5f; slot.target_weight=1;
    slot.time_ticks=float(n)*3; slot.time_rate=1; slot.duration_ticks=30; slot.contribution=float(n+1);
  }
  for (unsigned frame=0; frame<65; ++frame) {
    input.slots[0].time_ticks=float(frame)/4;
    const auto plan=PlanNativeAnimationController(input);
    Require(plan.has_value(),"advancing native controller plan");
    std::vector<animation_source::ChannelRecord> initial(3);
    for (auto &record : initial) { record[0]=128|64; record[1]=0xFEED; record[5]=0x7fc01234; }
    const auto result=animation_source::ExecuteController(*plan,assets,names,skeleton,
        animation_source::ControllerLayer::Decode(initial),0,{});
    Require(result && result->sampled == 3 && result->mixed == 3 && result->interior == 3,
            "owned controller executes all sampled/mixed interior layers");
    std::array<std::vector<animation_source::ChannelRecord>,8> reference;
    reference[7]=initial; for (auto &record : reference[7]) record[0]&=~128u;
    // Independent, previously live-checked ABI operations validate the new
    // working representation. Plan ordering has separate literal checks above.
    for (size_t n=0; n<plan->count; ++n) {
      const auto &step=plan->steps[n]; auto &destination=reference[step.destination];
      if (step.kind == NativeAnimationStep::Kind::Copy) destination=reference[step.left];
      else if (step.kind == NativeAnimationStep::Kind::Mix) {
        Require(animation_source::MixChannelRecords(reference[step.left],reference[step.right],step.weight,
            step.destination == 6 && step.left == 6,false,destination),"reference layer composition");
      } else {
        if (step.reset) destination.resize(3);
        Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,step.seconds,step.weight,2,false,destination,{{},{},true},step.reset),
                "reference whole clip application");
      }
    }
    Require(result->output.Encode() == reference[7],"native working layers equal every outgoing channel word across advancing frames");
    std::vector<RenderMatrix> pose;
    Require(EvaluateNativeSkeleton(skeleton,result->output.channels,JointIdentity(),pose),
            "completed controller values feed native hierarchy directly, without channel record decoding");
    NativeInstanceRegistry instances; const auto instance=instances.Create(91);
    Require(instances.Publish(instance,0,pose) && instances.Transfer(instance,0,1,3),"controller pose enters existing instance/render owners");
    auto retained=instances.Read(instance,1); instances.Retire(instance);
    Require(retained && retained->transforms == pose,"controller-derived poses outlive retirement");
  }
  input.active=1; input.overlay_count=2; input.overlays[1]=ControllerName("arm");
  const auto plan=PlanNativeAnimationController(input);
  const std::array excluded{ControllerName("arm")};
  auto result=animation_source::ExecuteController(*plan,assets,names,skeleton,animation_source::ControllerLayer(3),0,excluded);
  Require(result && result->subtree == 1 && result->exclusions == 0 && result->output.channels[0].translated,
          "whole-body exclusions followed by named forced-subtree overlay use the same native values");
  auto missing=assets; missing[1].reset();
  Require(!animation_source::ExecuteController(*plan,missing,names,skeleton,animation_source::ControllerLayer(3),0,excluded),
          "missing overlay asset refuses whole transaction, not a partially published base pose");
  auto bad_plan=*plan; bad_plan.steps[1].seconds=-1;
  Require(!animation_source::ExecuteController(bad_plan,assets,names,skeleton,animation_source::ControllerLayer(3),0,excluded),
          "direct overlay does not silently clamp out-of-range clock");
  const auto records=result->output.Encode();
  animation_source::ControllerHandoff handoff;
  auto publish=[&] { return handoff.Publish({1,2,0x8000,3,result->output.channels,records}); };
  unsigned reads=0;
  auto read=[&](uint64_t address)->std::optional<uint32_t> {
    ++reads;
    if (address < 0x8000 || address >= 0x8090 || (address&3)) return {};
    return records[(address-0x8000)/48][((address-0x8000)%48)/4];
  };
  bool changed=false;
  Require(publish() && !handoff.Take(1,2,4,0x8000,read,changed) && reads == 0,
          "generation reuse rejects stale handoff before source reads");
  Require(publish() && handoff.Take(1,2,3,0x8000,read,changed).has_value() && reads == 36 &&
          !handoff.Take(1,2,3,0x8000,read,changed),"completed native channel handoff is validated and consumed exactly once");
  Require(publish() && !handoff.Take(1,2,3,0x8000,[&](uint64_t address) {
    auto word=read(address); if (address == 0x8008) *word^=1; return word;
  },changed) && changed,"late channel writes reject rather than silently reuse stale native state");
  Require(publish(),"publish before retirement"); handoff.Retire(1);
  Require(!handoff.Take(1,2,3,0x8000,read,changed),"visual retirement invalidates pending native channels");
  Require(publish() && !handoff.Publish({1,2,UINT32_MAX-3,3,result->output.channels,records}) &&
          !handoff.Take(1,2,3,0x8000,read,changed),"overflowing failed replacement clears stale pending ownership");

  // Reproduce the former ownership gap: a post-controller writer changes valid
  // channels, then bones must consume the UPDATED native owner, not stale base.
  std::array<bool,6> enabled{}; enabled[3]=enabled[4]=enabled[5]=true;
  for (unsigned frame=0; frame<65; ++frame) {
    auto outgoing=records;
    outgoing[1][0]|=128; // No matching animated track: late work must retain dirtiness.
    auto base_layer=animation_source::ControllerLayer::Decode(outgoing);
    auto boundary_read=[&](uint64_t address)->std::optional<uint32_t> {
      if (address < 0x8000 || address >= 0x8090 || (address&3)) return {};
      return outgoing[(address-0x8000)/48][((address-0x8000)%48)/4];
    };
    Require(handoff.Publish({1,2,0x8000,3,base_layer.channels,outgoing}),"publish controller before authored late layers");
    auto continued=handoff.TakeLayer(1,2,3,0x8000,boundary_read,changed);
    Require(continued && !continued->late && continued->layer.Encode() == outgoing,
            "validated handoff retains native payloads and dormant boundary values without decoding payloads");
    auto late_slots=input.slots;
    for (uint8_t n : {4,5,3}) {
      late_slots[n].time_ticks=float(frame)/4; late_slots[n].time_rate=.25f;
      late_slots[n].weight=float(n)/8; late_slots[n].weight_rate=.03125f;
    }
    const auto late_plan=PlanNativeAnimationLateLayers(late_slots,enabled,false,ControllerName(""));
    Require(late_plan.has_value(),"advancing late layer plan");
    const auto late_result=animation_source::ExecuteController(*late_plan,assets,names,skeleton,
        std::move(continued->layer),0,{},false);
    Require(late_result && late_result->sampled == 3,"three late consumers use one native working layer");
    auto reference=outgoing;
    for (uint8_t n : {4,5,3}) {
      const float seconds=float(double(float(double(late_slots[n].time_ticks)+double(late_slots[n].time_rate)))/30.0);
      const float weight=float(double(late_slots[n].weight)+double(late_slots[n].weight_rate));
      Require(animation_source::ApplyKeyedLayer(*asset,names,skeleton,seconds,weight,2,false,reference,{},false),
              "independent legacy layer reference in authored late order");
    }
    outgoing=late_result->output.Encode();
    Require(outgoing == reference && (outgoing[1][0]&128),"late layers match old ABI and preserve dirty flags on untouched joints");
    Require(handoff.Publish({1,2,0x8000,3,late_result->output.channels,outgoing,true}),"late writer republishes completed native owner");
    auto bones=handoff.TakeLayer(1,2,3,0x8000,boundary_read,changed);
    std::vector<RenderMatrix> pose;
    Require(bones && bones->late && !changed && EvaluateNativeSkeleton(skeleton,bones->layer.channels,JointIdentity(),pose),
            "late native channels reach existing skeleton evaluation without stale-base rejection");
    NativeInstanceRegistry instances; const auto instance=instances.Create(91);
    Require(instances.Publish(instance,0,pose) && instances.Transfer(instance,0,1,3),"late pose feeds existing immutable render instance");
    auto retained=instances.Read(instance,1); instances.Retire(instance);
    Require(retained && retained->transforms == pose,"completed late pose survives source/instance retirement");
    Require(!handoff.TakeLayer(1,2,3,0x8000,boundary_read,changed),"late pose handoff remains exactly once");
    Require(handoff.Publish({1,2,0x8000,3,late_result->output.channels,outgoing,true}),"publish before untracked writer");
    outgoing[0][2]^=1;
    animation_source::ControllerHandoff::Difference difference;
    Require(!handoff.TakeLayer(1,2,3,0x8000,boundary_read,changed,&difference) && changed &&
            difference.joint == 0 && difference.word == 2 && difference.actual == outgoing[0][2],
            "untracked late write is still rejected with bounded causal provenance");
  }
}
} // namespace
void TestAnimationClips() {
  TestImportedAnimation(); TestAnimationRefusals(); TestAngularSegments(); TestPackedAngularReference(); TestLoadedAnimationAssets(); TestSelectedAnimationResidency(); TestCompressedAnimations();
  TestWeightedChannels(); TestWeightedLayerConsumption(); TestOwnedBlendRest();
  TestLayerMixing();
  TestConstantTimesAndScaleTail(); TestNamedAnimationSelection();
  TestFirstMatchAnimationDescriptors();
  TestIndexedAnimationAssets();
  TestSelectedTrackAndRootMotion();
  TestNativeControllerPlan(); TestNativeControllerConsumption();
  std::cout << "native keyed clips: source-free channels, hierarchy/instance consumption, lifetime and budgets passed\n";
}
