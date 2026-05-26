# Lirah-1 (NTS-1 mkII)

`Lirah-1` is a Lyra-8 inspired oscillator for the Korg NTS-1 mkII (`osc` module).

This project is based on the original Lyre-1 project by James D. Cheetham:
https://github.com/jamesdcheetham/Lyre-1

Lyra-8 from SOMA Laboratory https://somasynths.com/

Ported to NTS-1 mkII by Daniel Majid Mirzakhani.

## Highlights

- Sine-core voice with FM, wave folding, and feedback.
- Hyper LFO behavior from two internal LFOs that are AND-gated.
- Extra assignable modulation source (`LFO 3`) with selectable target and rate.
- Playable range tuning for both modulator pitch and carrier pitch.

## Controls

- `Knob A (FM DEPTH)`: FM amount from the modulator into the carrier.
- `Knob B (HYPER LFO)`: Depth of the Hyper LFO pitch jump.
- `LFO1 RATE`: Rate of Hyper LFO lane 1.
- `LFO2 RATE`: Rate of Hyper LFO lane 2.
- `FOLD`: Amount of wave folding.
- `FM TUNE`: Relative tuning of the modulator (up to one octave).
- `PITCH`: Relative tuning of the carrier (up to one octave).
- `FEEDBACK`: Feedback amount in the carrier path.
- `LFO3 TARGET`: Chooses what `LFO 3` modulates.
- `LFO3 RATE`: Rate of `LFO 3` with slow-focus scaling (`0-50%` = `0-1 Hz`, `50-100%` = `1-25 Hz`).

### LFO TARGET Map

- `OFF`
- `FMDEPTH`
- `HYPER LFO DEPTH`
- `HYPER LFO 1`
- `HYPER LFO 2`
- `FOLD`
- `FM TUNE`
- `CARRIER TUNE`
- `FEEDBACK`

## To build this project

- Clone this repo 
    In desired directory:

    Download this repo

    git clone --recurse-submodules https://github.com/DanielMajid/Lirah-1.git

- Download the ARM GCC toolchain

    cd logue-sdk/tools/gcc/
    ./get_gcc_osx.sh
    Run Make command to build binary

- Compile project
    Run "make install"
    Open Korg Kontrol Editor

- Load Project
    Drag .nts1mkiiunit file into the appropriate module category
    Click sync