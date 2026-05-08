# Lirah-1

Lyra-8 style custom oscillator for the Korg NTS-1 mkii. Originally built for the mki by JamesDCheetham https://github.com/jamesdcheetham/Lyre-1.</p> Ported to the mkii by Daniel Majid Mirzakhani. </p>

Sine wave with FM hyper LFO (AND gate: high if lfo1 AND lfo2 are high) and single stage wave folder. </p>
knob A: FM depth</p>
knob B: LFO depth, ranges from 0 to 12 semitones</p>
lfo1: LFO rate 1</p>
lfo2: LFO Rate 2</p>
wave fold: 1 stage wave folder</p>
fm tune: relative tuning (unquantized) of modulator oscillator (1 octave range)</p>
pitch: unquantized pitch adjustment for main oscillator (1 octave range)</p>
feedback: fm feedback for main oscillator</p>
</br>
To build this project:

In desired directory:

Download this repo

- git clone --recurse-submodules https://github.com/DanielMajid/Lirah-1.git

Download the ARM GCC toolchain
- cd logue-sdk/tools/gcc/
- ./get_gcc_osx.sh

Run Make command to build binary</br>
- run "make install"</p>

Open Korg Kontrol Editor</p>
- Drag .nts1mkiiunit file into the appropriate module category</p>
- Click sync</p>
