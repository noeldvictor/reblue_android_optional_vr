#include "gpu/scene/native_animation_source.h"
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
  refuse([](auto &s){s.Word(0x205A,0xBB776655);},"ambiguous descriptor hash refuses");
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
          "unsupported replacement invalidates lookup while prior native lease remains immutable");
}
} // namespace
void TestAnimationClips() {
  TestImportedAnimation(); TestAnimationRefusals(); TestAngularSegments(); TestPackedAngularReference(); TestLoadedAnimationAssets();
  std::cout << "native keyed clips: source-free channels, hierarchy/instance consumption, lifetime and budgets passed\n";
}
