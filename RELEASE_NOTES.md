# 0.1.8

This rollout updates the shared engine to the merged optimization bundle.
Internal Resolution is enabled at 4x by default with saved Native (1x) through
32x choices and the renderer's hardware/memory guards. Disabling it uses native
resolution. CD Speed and Fast Loading packages are excluded.

The existing digital controller, native memory cards and experimental
widescreen feature remain. This build also retains the independently validated
Automatic Disc Switching change from upstream PR #7, enabled by default.
The separate paused expanded-party and character branches are not included.

The maintained release recipe regenerates the resident program from the owned
disc and produces a fresh selective runtime PGO build. The short boot used to
produce profiles is a build step, not new gameplay coverage or a speedup claim.

The Windows release is published through the owner's public fork; upstream
integration remains a pull request. No disc images, private saves or compiler
profiles are included in the archive.

Retain the compiled Vulkan backend option; OpenGL remains the default.
