# Lirah-1

Lyra-8 style custom oscillator for the Korg NTS-1 mkii. Originally built for the mki by JamesDCheetham https://github.com/jamesdcheetham/Lyre-1.</p> Ported to the mkii by Daniel Majid Mirzakhani. </p>

Sine wave with FM hyper LFO (AND gate: high if lfo1 AND lfo2 are high) and single stage wave folder. </br>
knob A: FM depth</br>
knob B: LFO depth, ranges from 0 to 12 semitones</br>
lfo1: LFO rate 1</br>
lfo2: LFO Rate 2</br>
wave fold: 1 stage wave folder</br>
fm tune: relative tuning (unquantized) of modulator oscillator (1 octave range)</br>
pitch: unquantized pitch adjustment for main oscillator (1 octave range)</br>
feedback: fm feedback for main oscillator</br>
</br>
To build this project:

In desired directory:

Download this repo

- clone this repo https://github.com/DanielMajid/Lirah-1.git</p>
- git submodule update --init</p>
- Update logue sdk dependencies

cd logue-sdk</p>
- git submodule update --init</p>
- tools/gcc/get_gcc_osx.sh</p>
Run Make command to build binary</br>

- run "make install"</p>
- Load the unit</p>

Open Korg Kontrol Editor</p>
Drag .nts1mkiiunit file into the appropriate module category</p>
Click sync</p>
