# 0.1.10

Correct the SPU noise clock's phase handling when its frequency changes. A
slow-to-fast transition could hold the preceding noise signal for about 424 ms.
The corrected divider and fractional phases match reference PCM across all 64
noise clock controls and dynamic frequency changes, within one rounding unit
after output-gain normalization. Current-build restore and audio-block
partition checks pass. Battle entry and Cloud's attack were exercised with
captured PCM; this does not establish complete audible SFX accuracy.

Fresh settings retain Fit to screen and 4x internal resolution. Existing saved
choices remain authoritative. CD Speed and Fast Loading are excluded.

# 0.1.9

Restore hardware pitch modulation used by the battle-entry sweep and Cloud's
sword attack. The SPU now uses the preceding voice's post-envelope output before
channel volume, applies the full pitch register before saturation, and preserves
sample-position overshoot across ADPCM blocks.

Widescreen defaults to Fit to screen for fresh settings. Existing saved mod and
aspect choices remain authoritative. Internal Resolution still defaults to 4x;
Automatic Disc Switching stays enabled. CD Speed and Fast Loading are excluded.

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
