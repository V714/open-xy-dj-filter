# Master DJ Filter for Open-XY

A one-knob resonant LP / neutral / HP replacement for the master EQ. Shift+M2
on MIX's filter page opens the stock `filter` selector. Turn the left encoder
to choose `original eq` or `dj filter`; click the left or white encoder to
confirm. The first encoder controls the sweep; its click returns to neutral.

The filter uses a stereo 12 dB/octave TPT state-variable algorithm, coefficient
ramps and resonance compensation. Neutral is bit-exact dry. The endpoints are
80 Hz LP and 5 kHz HP, retaining the remaining frequency band; there is no kill
mode. Tables avoid runtime trigonometry and allocation in the audio callback.

The room animation runs on the GUI role, separately from audio. Its opaque
floor spans five stock-grid tiles in each direction. Doors and blinds animate
on the LP side; the complete background brightens on the HP side. Reset eases
the room back to neutral in approximately 120 ms.

Selection and sweep are saved beside the stock `.xy` in its `.oxy` companion.
A missing, unsupported or mismatched companion starts with stock EQ and a
neutral DJ sweep, preserving stock EQ's own parameters. The catalog/extension ID is `master_dj_filter`. Its stable project-state ID
remains `dj`, preserving earlier experimental filter companions.

## Develop

Requires the Open-XY SDK master-filter interface, proposed against `staging`.
Version `v1.0.1` accompanies the interface's update to Open-XY 0.0.3 / SDK 1.6.
The master-filter interface from PR #233 is still required; the official 0.0.3
release alone does not provide it. DSP, artwork, controls and the `dj` project-state
identity are unchanged from `v1.0.0`.
From the SDK checkout:

```sh
python sdk.py test --extension ../open-xy-dj-filter
python sdk.py build --machine <machine-folder> --extension ../open-xy-dj-filter --stock-os <private>/os.ldr --out <private>/dj-build-001
python sdk.py kit --machine <machine-folder> --extension ../open-xy-dj-filter --stock-os <private>/os.ldr --out <private>/dj-builder-001 --personal
```

Keep firmware and firmware-containing output outside every checkout. SDK build
documentation covers toolchain and stock-content options. This repository
contains authored artwork and generated source, not stock firmware.

Run `python master_filter_tables.py` and `python master_filter_artwork.py` after
changing their inputs. Tests compare the fixed-point DSP with a floating-point
reference, check filter endpoints and rapid transitions, and drive the public
GUI callback. A native source rebuild is verified separately by the SDK;
CPU timing and acceptance of the migrated build on hardware remain pending.

Publish this repository first, then submit its immutable tag/commit as a
separate `catalog/extensions.json` PR after the required SDK interface merges.
Do not include this DSP or artwork in the SDK PR.
