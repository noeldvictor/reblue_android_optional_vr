/**
 * @file    deferred_consumer.h
 * @brief   Host deferred-list consumption with explicit engine adapters.
 * @copyright Copyright (c) 2026 reblue contributors
 * @license BSD 3-Clause, see LICENSE
 */
#pragma once
#include <cstdint>
struct PPCContext;
namespace bd::gpu::scene {
struct NativeRigidSceneSubmission;
// Temporary source bridge only: removes delayed plans from the submission and
// retains them without allocating/copying a compatibility render-list entry.
bool StageNativeDeferredScene(uint32_t visual, NativeRigidSceneSubmission &submission);
bool HasNativeDeferredScene();
void CloseDeferredCompatibilityCapture();
// False only before any side effects, when the initial import is invalid.
bool ConsumeDeferredList(PPCContext &ctx, uint8_t *base);
void RecordDeferredConsumerFallback();
} // namespace bd::gpu::scene
