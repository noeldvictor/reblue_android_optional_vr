/**
 * @file    host_parameter_bridge.h
 * @brief   Temporary engine entry point for host float-parameter publication.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
struct PPCContext;
namespace bd::gpu::scene {
void SetHostFloatParameters(PPCContext &ctx, uint8_t *base, bool vertex);
// Temporary descriptor adapter for remaining non-native draw consumers.
// Native execution only: no guest dispatch, comparison call or fallback.
// Returns false before parameter writes if the import/settings are unsupported.
bool FlushHostParameterDescriptor(uint32_t descriptor, uint32_t stack);
bool CanFlushHostParameterDescriptor(uint32_t descriptor, uint32_t stack);
} // namespace bd::gpu::scene
