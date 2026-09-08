/**
 * @file    native_lighting_bridge.h
 * @brief   Temporary engine source and shader boundaries for native lighting.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_lighting.h"
#include "gpu/scene/native_selected_lights.h"
#include "gpu/scene/native_scene_lights.h"
namespace bd::gpu::scene {
struct NodeTag;
std::optional<NativeLightingPass> FindNativeLightingPass(uint32_t render_view);
std::optional<NativeLightingPass> NativeNodeLightingPass(const NodeTag &tag);
bool CheckNativeLightingPass(const NativeLightingPass &pass, const uint8_t *pixel_constants);
void NoteNativeLightingPassDraw();
// Source selection identity is checked only at object publication, not drawing.
std::optional<NativeSelectedLights> FindNativeSelectedLights(uint32_t selection);
// Producer invoked after completed light/pose transfer, before DrawStart.
void PublishNativeSceneLights(uint32_t manager);
uint64_t NativeSceneLightUpdate(uint32_t frame);
// Native IDs and owned pass only; no source slots, dirty flags or descriptors.
std::optional<NativeSceneLightRecipe> CaptureNativeSceneLights(uint64_t instance,
    uint64_t model_generation, uint32_t node, const NativeLightingInputs &pass);
// Consumes only owned values/publication identity; no instance/source lookup.
std::optional<NativeSceneLightTicket> ResolveNativeSceneLights(const NativeSceneLightRecipe &recipe);
// Commit at draw participation, never during a speculative/suppressed prepare.
// Stack belongs only to the outgoing compatibility descriptor adapter.
bool CommitNativeSceneLights(const NativeSceneLightTicket &ticket, uint32_t stack);
void ObserveNativeSceneLightParameters(bool vertex, uint32_t first, uint32_t count, const void *words);
void InvalidateNativeSceneLightInheritance();
void NativeSelectedLightsReport();
void CheckNativeSelectedLights(const NativeSelectedLights &lights, const uint8_t *pixel_constants);
std::optional<LightingVector> NativeNodeShadowSampling(const NodeTag &tag);
void CheckNativeShadowSampling(const LightingVector &expected,
                                const uint8_t *pixel_constants);
void NoteNativeShadowSamplingReplay();
} // namespace bd::gpu::scene
