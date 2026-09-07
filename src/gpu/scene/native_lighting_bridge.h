/**
 * @file    native_lighting_bridge.h
 * @brief   Temporary engine source and shader boundaries for native lighting.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include "gpu/scene/native_lighting.h"
#include "gpu/scene/native_selected_lights.h"
namespace bd::gpu::scene {
struct NodeTag;
std::optional<NativeLightingPass> FindNativeLightingPass(uint32_t render_view);
std::optional<NativeLightingPass> NativeNodeLightingPass(const NodeTag &tag);
bool CheckNativeLightingPass(const NativeLightingPass &pass, const uint8_t *pixel_constants);
void NoteNativeLightingPassDraw();
// Source selection identity is checked only at object publication, not drawing.
std::optional<NativeSelectedLights> FindNativeSelectedLights(uint32_t selection);
void NativeSelectedLightsReport();
void CheckNativeSelectedLights(const NativeSelectedLights &lights, const uint8_t *pixel_constants);
std::optional<LightingVector> NativeNodeShadowSampling(const NodeTag &tag);
void CheckNativeShadowSampling(const LightingVector &expected,
                                const uint8_t *pixel_constants);
void NoteNativeShadowSamplingReplay();
} // namespace bd::gpu::scene
