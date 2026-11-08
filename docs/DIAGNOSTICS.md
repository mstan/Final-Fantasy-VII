# Playtest Diagnostics

This optional mod records the inputs and starting conditions needed to rerun
gameplay in the same build. It uses the shared PSXRecomp recorder and verified
offline replay helper. It does not encode an MP4 or save an audio recording;
replaying the game regenerates its picture and audio.

## Enable and collect a session

1. Open `Final_Fantasy_7.exe`, choose **Mods > Playtest Diagnostics**, and enable
   **Input Recording**. Enable **Input Display** as well if you want to see
   accepted buttons, frame numbers and `REC` status. Display alone does not
   record anything. Both options default off in public releases.
2. Launch the game and reproduce the problem. Starting from a fresh launch and
   an ordinary in-game save gives the simplest recording. If available, press
   **L3 + R3 together** when the problem occurs: this marks an incident and
   requests an `incident-N.png` screenshot. Otherwise note the approximate time.
3. Quit normally so buffered input and telemetry files finish writing. Turn
   **Input Recording** off when you have finished collecting reports.
4. Open `diagnostics-location.txt` beside the executable to find the latest
   session. Normally it is `<game folder>/saves/diagnostics/session-<timestamp>/`.
   If that location cannot be created, the fallback on Windows is
   `%TEMP%/psxrecomp-diagnostics/SCUS-94163/session-<timestamp>/`. The location
   pointer itself may be unavailable in a read-only game folder.
5. Zip the **entire session folder**, rather than just `inputs.csv`. If you
   loaded a save state, include the related session folders too: a load starts
   a new segment. Keep the original release version available for replay.

Recording writes one session per launch or state-load segment and can grow
while you play. Delete your local sessions after the report is resolved if you
no longer need them. If the display says `ERROR`, check the location pointer
and available storage; include that detail in your report.

## What the bundle contains

| File | Purpose |
| --- | --- |
| `inputs.csv` | Accepted controller state at each guest VBlank, plus a rotating guest-RAM digest used to detect replay divergence. |
| `telemetry.csv` | Guest-frame host timings, CD activity, video configuration, drawing/presentation counters and incident markers. |
| `session.json` | Game, executable/build and execution identities, recording format, and segment information. |
| `card1.mcd`, `card2.mcd` or `.absent` markers | Copies of the starting memory cards, or explicit absence of a card. Original cards remain in their usual save location. |
| `settings.toml`, when present | The settings used at the start of the session. |
| `mods-state.toml` | Starting mod choices, including an explicit default selection on a fresh install. |
| `baseline.pst`, when present | The current-build state from which a save-state-load segment began. |
| `incident-N.png`, when capture succeeds | The screenshot requested by an incident marker. |

Input Display is independent of recording. The recorder does not create a
video, microphone recording or speaker-output WAV. For audio problems, include
a separate short audio/video clip when possible and identify its time or
incident number in the diagnostic session.

## Send a report

Attach the session ZIP to your existing report, or share a private download
link with the maintainer if it is too large for an attachment. New reports can
be opened at [the game's issue tracker](https://github.com/TechnicallyComputers/Final-Fantasy-VII/issues/new).
Include the release version, where you were, the actions that trigger the
problem, what you expected, and the approximate time or incident number.

The bundle includes your starting memory-card saves and settings. Use a private
sharing route if you do not want those posted publicly. Send only the session
ZIP and any separate clip; do not add disc images or a BIOS. No files are
uploaded automatically.

## Replay and inspect

Players do not need Python to record or send a report. For investigation, the
release includes `replay-diagnostics.py` beside the executable. With Python 3.11
or newer, the **original release executable**, the session folder and the same
legally obtained disc, run:

```powershell
python .\replay-diagnostics.py "C:\Reports\session-123" --exe .\Final_Fantasy_7.exe --disc "D:\Games\Final Fantasy VII (USA) (Disc 1).cue" --output "C:\Reports\ff7-replay-123"
```

Use a new output directory. The helper copies the starting saves, settings and
mod choices there and starts a hidden, muted replay, preserving your installed
saves. `replay.log` reports completion or the first divergent input/RAM row.
Matching inputs and RAM are a reproducibility check, not a verdict that the
picture or sound is correct. For a telemetry summary without launching a game:

```powershell
python .\replay-diagnostics.py "C:\Reports\session-123" --analyze
```

Verified replay requires the exact executable/build, execution profile, mod
choices and matching starting state. A different release is rejected. Rewind
segments whose full baseline is unavailable and netplay sessions cannot be
verified by this offline helper. Prefer an ordinary offline session without
rewind when collecting a reproducible bug.
