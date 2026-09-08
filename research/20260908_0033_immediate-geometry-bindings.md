# Complete immediate GPU geometry binding

2026-09-08. Source checkpoint based on18eeb69; host127 is stamped18eeb6994 dirty.
This fixes a concrete native/queued-to-immediate handoff contract. It does not
establish the cause of run962's title-logo image or run956's tree-trunk gaps.

## Contract and connected change

`src/gpu/draw_queue.cpp::EmitBindings` selects physical pipelines and vertex/index
views from immutable queue entries, including native and instanced variants.
Queue flush already restores the explicit descriptor/layout resume snapshot.
It does not restore the immediate producer's cached PSO/VB/IB or dirty those
logical inputs. `src/gpu/hooks/draw.cpp` flushes queued work before nondeferred
draws; previously `Video::FlushRenderStateLocked` could then skip geometry binds
when logical dirty flags were clean. Logical state equality does not establish
physical command-list state after another consumer has recorded commands.

The immediate consumer now calls `ApplyImmediateGeometryBindings` with its
complete prepared inputs. Dirty flags still govern CPU pipeline lookup and
stream-union preparation, but no longer gate physical PSO/VB/IB emission.
Pipeline caching, deferred by-value snapshots, device/frame resource ownership
and quad-list expansion are retained. No guest reimport, forced PSO compilation,
new shader ABI or second renderer framework is added.

The small production helper uses real Plume views, validates the bounded range
before emitting any commands and skips null-buffer stream gaps. Offsets, sizes,
strides and index formats remain explicit. Empty/nonindexed inputs are legal;
this helper does not claim ownership of descriptors, framebuffer or viewport.
Remaining immediate preparation and translated consumers are still adapters,
not a complete host-owned frame.

## Verification

- Artifact-free `python -B tools/host_checks.py --all-boundaries`:351 pass.
  These are source/scenario guards, not C++ or GPU proof.
- Draw-intent build21/PID33696 and CPU19/PID27096: terminal0. Existing fixture
  now exercises actual Plume view representations, nonzero offsets/strides,
  sparse ranges, slot15 replacement, changed index format and atomic refusal.
  It changes physical command state without changing logical producer inputs.
- GPU build49/PID28652 and rigid16/PID29332: terminal0. RTX3060,52 cases,
  1.45 s, zero Vulkan validation errors/warnings. The existing46 scene/shadow/
  cutout/instanced two-eye cases keep their unchanged scalar colour/depth oracle.
  Six added cases independently leave a wrong pipeline, collapsed vertex buffer
  or32-bit degenerate index buffer bound. Each control omits restoration and
  must yield blank colour/depth; each restored case calls the production helper
  and matches the normal oracle (maximum colour error0.0000255257, limit0.003).
  This makes each physical input causally necessary in the tested handoff.
  One unrelated missing GOG overlay-manifest loader message remains distinct
  from validation errors/warnings. No image or raw file is exported.
- Host127/PID34664: terminal0; codegen0 written/1 module current, no guest
  objects compiled. EXE48,761,344 B SHA256
  A0792FE25CB53149000151DA5927141A4F38C0CA9DEAA5574FAE2E685AB2C038;
  PDB109,965,312 B SHA256
  DC2B16F2075F15DEAB2BFB05A435B7A5DDD09080454AF1880E4976FB353893C6.

## Limits and next integration

No new game run: fresh scene/reload/interoperation pixels are pending. Run964
used the prior host126, passed text reload gates and refused its post-gamma
image export. It cannot qualify this change. Its preserved request must not be
overwritten; a larger proposed per-image limit still needs owner approval and
independent room within the unchanged aggregate image/diagnostic budgets.
Keep945 as last accepted mono game pixels and all unresolved failures.

Full scene/material/character/animation/effect/UI/pass ownership, asset cooking,
representative events/reloads/both eyes and the complete desktop gate remain
required before Quest2 work. No speedup or full-frame completion is claimed.
The devloop skill kept verification in existing bounded fixtures and one host
integration build; it did not waive the pending live gate.
Storage and retained producer records remain in the
[existing cumulative ledger](20260906_0333_native-scene-state-bridge.md#immediate-gpu-geometry-ownership-2026-09-08).
