# PassiveElectronicsModeler
This is a way of inserting an electronic compontent in as a vst effect
This is part of a larger project to make some electronic compontent models with a long term goal of making something like livespice but I sort of wanted to play with the idea a bit first to get a feel for how it can be approached.  I'll try to get the formulas to appear once I figure out how to do that on github.   None the less this first test version built and works (juce 8.0.12) for windows. I'll will likely continue to build out the next 5 planned plugins in this series before circling back and fixing the graphics bugs and such , processor wise this performed as expected I still havn't finalized the GUI but its basically showing a basic representation at this point.

Additionally I'm going to try to put some "limits" min max that make sense for standard electronics for human range audio processes.
For example 
a knob that goes from 1 Ω to 10 MΩ or 1 pF to 10 F is really a knob that's mostly "silence" or "no audible difference" at both ends, since the actual audio effect is governed by how the component's impedance compares to the 1 kΩ reference load this plugin already uses.   

what's actually relevant for the scope of the basic effect plugins to model useful component inserts: 

The derivation, using the plugin's own 1 kΩ reference load:

For the capacitor (series C into 1 kΩ, high-pass): f_c = 1/(2π·1000·C)
For the inductor (series L into 1 kΩ, low-pass): f_c = 1000/(2π·L)

Solving those for the audible-plus-headroom range (2 Hz–100 kHz, matching the "high-quality analog audio" row in what you sent, rather than a strict 20–20k which leaves no margin):
```
Component	f_c = 2 Hz	f_c = 100 kHz	Physically realistic ceiling
Capacitor	~79.6 µF	~1.6 nF	electrolytics realistically go to 100s of µF
Inductor	~79.6 H	~1.6 mH	audio inductors realistically top out ~10 H
```
Proposed finite ranges (intersection of the audio-corner math and physical realism, replacing the current 1–10,000,000 blanket range on all three):
```
Resistor: 1 Ω – 1 MΩ
Capacitor: 100 pF – 100 µF
Inductor: 100 µH – 10 H
```
Just posting up some considerations

Nyquist–Shannon, Fourier analysis, calculus, differential equations, convolution, stability theory, noise theory, and limits/minima/maxima all converge on this.

The important distinction is that there is no universal minimum or maximum value for a component. There is instead a useful operating region for a component within an audio system.

Human hearing is conventionally approximated as 20 Hz–20 kHz, although sensitivity is highly frequency-dependent; the ear is considerably more sensitive around roughly 1–4 kHz.

1. The fundamental mathematical "box" for audio

For ordinary music production, we can start with:

$$ 20\text{ Hz}\lesssim f\lesssim20\text{ kHz} $$

but the electronics generally need to extend beyond this.

A good conceptual hierarchy is:
```
Domain	Approximate useful region
DC / control	0 Hz
Subsonic audio	1–20 Hz
Human audio	20 Hz–20 kHz
Engineering audio bandwidth	~5 Hz–40 kHz
High-quality analog audio	~2 Hz–100 kHz
ADC/DAC anti-alias region	dependent on \(f_s\)
RF / switching / parasitic domain	>100 kHz and upward
```
So 20 Hz–20 kHz is not actually the electrical design boundary.

For example, an audio amplifier might intentionally have:

$$ f_L=2\text{ Hz} $$

and

$$ f_H=100\text{ kHz} $$

even though essentially none of that extra bandwidth is directly required for human hearing.

Why?

Because it gives us:
```
phase margin
transient response
filter transition space
anti-aliasing margin
reduced phase distortion in-band
easier filter design
tolerance for component variation
```
This is exactly why 44.1-kHz audio is not simply designed with an ideal brick-wall at 20 kHz. Nyquist says the theoretical minimum sampling frequency for a 20-kHz bandlimited signal is 40 kHz, but real filters aren't ideal, so 44.1 kHz gives a transition region between 20 and 22.05 kHz.

2. The master equation: every passive audio network is basically a differential equation

This is where your calculus intuition becomes extremely powerful.

For a resistor:

$$ V=IR $$

For a capacitor:

$$ I=C\frac{dV}{dt} $$

For an inductor:

$$ V=L\frac{dI}{dt} $$

Those three equations already explain an enormous fraction of analog audio.

Transforming into the frequency domain:

$$ Z_R=R $$ $$ Z_C=\frac{1}{j\omega C} $$ $$ Z_L=j\omega L $$

where

$$ \omega=2\pi f $$

So frequency itself determines how much a component "looks like" a resistor, short circuit, or open circuit.

That gives us the first profound principle:

Audio component values aren't inherently "audio values." They become audio values when their impedance interacts with the 20 Hz–20 kHz frequency domain.

3. The resistor
```
Practical audio range
Parameter	Roughly useful audio-engineering region
Tiny signal resistors	~10 Ω
Common minimum	~10–22 Ω
Extremely common	100 Ω–100 kΩ
High impedance	100 kΩ–1 MΩ
Very high	1–10 MΩ
Extreme	>10 MΩ
```
There are resistors far outside those values, but they become progressively less attractive for conventional audio.

Why?

Because resistor noise is:

$$ e_n=\sqrt{4kTRB} $$

where:

\(k\) = Boltzmann constant
\(T\) = temperature
\(R\) = resistance
\(B\) = bandwidth

Notice something important:

$$ e_n\propto\sqrt{R} $$

So higher resistance produces more thermal noise.

And high resistance also makes the circuit increasingly vulnerable to:
```
input bias currents
leakage
PCB contamination
capacitance
electromagnetic interference
```
Conversely, extremely low resistance causes:
```
high current
loading
power dissipation
driver stress
```
So audio naturally converges toward the kΩ region.

That's why you see so much:

$$ 1k,\ 2.2k,\ 4.7k,\ 10k,\ 22k,\ 47k,\ 100k $$

in audio electronics.

TI likewise notes that reducing resistor values reduces their noise contribution, while higher bandwidth itself generally admits more noise.

4. Capacitors

This is where your Fourier/calculus idea becomes particularly beautiful.

$$ X_C=\frac{1}{2\pi fC} $$

Suppose:

$$ C=1\mu F $$

At 20 Hz:

$$ X_C\approx7.96k\Omega $$

At 20 kHz:

$$ X_C\approx7.96\Omega $$

So the exact same capacitor behaves radically differently across the audio spectrum.

Useful audio capacitor range

Very approximately:
```
Capacitor	Typical audio role
10 pF–100 pF	RF compensation / parasitic control
100 pF–1 nF	HF filtering
1 nF–10 nF	tone shaping / HF filters
10 nF–100 nF	EQ / coupling / filtering
100 nF–1 µF	coupling / bypass
1–10 µF	coupling / filtering
10–100 µF	power supply / large coupling
100–10,000+ µF	power supply / reservoir
```
But we can derive audio-specific values.

For a first-order high-pass:

$$ f_c=\frac{1}{2\pi RC} $$

Suppose:

$$ R=10k\Omega $$

and we want:

$$ f_c=20Hz $$

Then:

$$ C=\frac{1}{2\pi(10,000)(20)} $$ $$ C\approx796nF $$

So a 1 µF coupling capacitor with 10 kΩ input impedance puts the corner around:

$$ 15.9Hz $$

That's a very audio-relevant result.

5. A very useful audio RC map

Here's a particularly useful way to think about component values.

For a target frequency:

$$ RC=\frac{1}{2\pi f} $$

Therefore:
```
Corner	RC product
1 Hz	159 ms
5 Hz	31.8 ms
10 Hz	15.9 ms
20 Hz	7.96 ms
50 Hz	3.18 ms
100 Hz	1.59 ms
1 kHz	159 µs
5 kHz	31.8 µs
10 kHz	15.9 µs
20 kHz	7.96 µs
100 kHz	1.59 µs
```
This is essentially an audio component-value coordinate system.

For example:

20 Hz
$$ RC=7.96ms $$

Possible combinations:

$$ 10k\Omega\times796nF $$

or

$$ 100k\Omega\times79.6nF $$

or

$$ 1M\Omega\times7.96nF $$

All theoretically produce the same corner frequency.

But they are not equivalent practically, because resistor noise, capacitor leakage, op-amp bias current, loading, dielectric behavior, etc. differ.

That's where real engineering begins.

6. Inductors

Inductors are fascinating because they are mathematically the inverse of capacitors:

$$ X_L=2\pi fL $$

At 20 Hz:

$$ X_L=125.7L $$

where \(L\) is in henries.

At 20 kHz:

$$ X_L=125,664L $$

So:
```
Inductor	20 Hz reactance	20 kHz reactance
1 µH	0.000126 Ω	0.126 Ω
10 µH	0.00126 Ω	1.26 Ω
100 µH	0.0126 Ω	12.6 Ω
1 mH	0.126 Ω	126 Ω
10 mH	1.26 Ω	1.26 kΩ
100 mH	12.6 Ω	12.6 kΩ
1 H	126 Ω	126 kΩ
```
This immediately explains why inductors in audio tend to be substantially larger physically than capacitors when used for low-frequency filtering.

A 1-H inductor is enormous compared with a 1-µH RF inductor.

7. Why audio filters often use capacitors instead of inductors

This is a beautiful example of the mathematical constraints becoming engineering constraints.

A low-pass LC filter might require:

$$ L=100mH $$

That can mean:
```
large physical component
magnetic coupling
core saturation
copper resistance
weight
cost
electromagnetic radiation
```
An active filter can replace that inductor with:
```
resistors
capacitors
op amp
```
So the rise of active audio electronics is partly the story of escaping inconvenient physical component values through feedback and gain.

8. Transformers

Transformers don't have a single "audio component value" like a resistor.

Their critical quantities are:
```
turns ratio
inductance
winding resistance
leakage inductance
interwinding capacitance
core material
core size
saturation flux
impedance
frequency response
```
A transformer might need to operate roughly from:

$$ 20Hz\rightarrow20kHz $$

but that is an extraordinarily difficult ratio:

$$ \frac{20,000}{20}=1000:1 $$

That's three decades of frequency.

The low-frequency limit is principally associated with magnetizing inductance/core behavior.

The high-frequency limit is strongly influenced by:

leakage inductance
winding capacitance
core losses

So a transformer is almost a physical frequency-domain computer.

9. Diodes

Diodes have a very different useful range.

The important audio quantities are:

forward voltage
junction capacitance
reverse recovery
leakage
dynamic resistance
current

Typical silicon diode forward voltage:

$$ \sim0.6-0.7V $$

Schottky:

$$ \sim0.2-0.5V $$

Small-signal germanium:

$$ \sim0.2-0.3V $$

This is why diode choice can matter enormously in:

clipping circuits
distortion
rectifiers
compressors
envelope detectors
analog synth circuits
limiters

A diode isn't primarily limited by "20 Hz–20 kHz."

Instead, it is limited by nonlinearity and junction physics.

And that leads to something extremely important:

Linear components are governed largely by impedance and bandwidth. Nonlinear components are governed additionally by harmonic generation and intermodulation.

10. Transistors

For an audio transistor, the useful quantities become:

\(g_m\)
\(r_\pi\)
\(C_{be}\)
\(C_{bc}\)
\(f_T\)
collector/emitter current
voltage swing
noise
distortion
thermal stability

A transistor may have an intrinsic transition frequency:

$$ f_T=100MHz $$

or:

$$ 1GHz $$

while being used to amplify:

$$ 20Hz-20kHz $$

That enormous ratio is deliberate.

The transistor has to be much faster than the signal.

11. Op amps: where active electronics gets really interesting

This is one of the most important parts of your question.

Suppose we want:

$$ A_v=10 $$

and:

$$ f_{max}=20kHz $$

An approximate minimum gain-bandwidth requirement is:

$$ GBW\approx A_vf $$

so:

$$ GBW\approx10(20,000) $$ $$ GBW\approx200kHz $$

But that's merely theoretical.

For high-quality audio, you'd normally want substantial margin.

Something like:

$$ GBW=2MHz-20MHz $$

is extremely comfortable for many audio applications.

For perspective, TI's LME49723 is a 17-MHz audio op amp, while the OPA134 has an 8-MHz bandwidth and 20 V/µs slew rate.

12. Slew rate gives us another calculus-derived limit

A sine wave is:

$$ v(t)=A\sin(2\pi ft) $$

Differentiate:

$$ \frac{dv}{dt}=2\pi fA\cos(2\pi ft) $$

Therefore maximum slope is:

$$ \boxed{\frac{dv}{dt}_{max}=2\pi fA} $$

That's literally calculus determining the minimum slew rate of an audio amplifier.

Suppose:

$$ f=20kHz $$

and:

$$ A=1.736V $$

Then:

$$ SR=2\pi(20,000)(1.736) $$ $$ SR\approx0.218V/\mu s $$

TI gives essentially this exact example for line-level audio and recommends about 10× margin, giving roughly:

$$ 2.18V/\mu s $$

as a conservative design target.

So you can derive an absolute mathematical minimum from the audio waveform.

13. But music makes the requirement more complicated

A sine wave isn't music.

A transient can contain:

$$ f_1+f_2+f_3+\cdots $$

and therefore the instantaneous slope can be substantially greater than what a single 20-kHz sine wave suggests.

Consequently, practical audio electronics often have:

$$ SR\gg2\pi f_{max}V_{peak} $$

This is one reason modern audio op amps can have slew rates like:

$$ 10-50V/\mu s $$

even though human hearing only reaches ~20 kHz.

14. The amplifier's voltage range is another "human audio limit"

Consider professional line level:

$$ +4dBu $$

which corresponds to:

$$ 1.228V_{RMS} $$

or:

$$ 1.736V_{peak} $$

But professional audio equipment often needs 15–25 dB of headroom.

At +20 dBu:

$$ 7.75V_{RMS} $$

and:

$$ 10.95V_{peak} $$

At +24 dBu:

$$ 12.28V_{RMS} $$

and:

$$ 17.37V_{peak} $$

This is why ±15 V and ±18 V analog supplies became so useful in professional audio.

You aren't merely amplifying hearing.

You're accommodating:

$$ \text{signal}+\text{transients}+\text{headroom}+\text{noise margin} $$
15. The dynamic-range boundary

Human hearing introduces another enormous constraint.

A commonly quoted approximate dynamic range is:

$$ \sim120dB $$

from threshold of hearing to extremely loud sound.

That corresponds to a voltage ratio of:

$$ 10^{120/20}=10^6 $$

So approximately:

$$ 1,000,000:1 $$

in amplitude.

This is astonishing.

And it tells us why audio electronics cares so much about:

resistor thermal noise
op-amp voltage noise
current noise
power-supply noise
grounding
shielding
distortion
quantization

A system doesn't need infinite precision.

It needs precision greater than the perceptually and technically useful dynamic range.

16. This produces the famous 16/24-bit boundary

For an ideal \(N\)-bit converter:

$$ SNR\approx6.02N+1.76dB $$

So:

16-bit
$$ 6.02(16)+1.76\approx98.1dB $$
24-bit
$$ 6.02(24)+1.76\approx146.2dB $$

Real converters don't achieve the ideal 146 dB because analog noise and converter imperfections intervene.

This is a wonderful example of mathematical limits turning into engineering limits.

17. Nyquist creates another hard boundary

For sampling frequency:

$$ f_s $$

the Nyquist frequency is:

$$ f_N=\frac{f_s}{2} $$

Therefore:

Sample rate	Nyquist frequency
32 kHz	16 kHz
44.1 kHz	22.05 kHz
48 kHz	24 kHz
88.2 kHz	44.1 kHz
96 kHz	48 kHz
176.4 kHz	88.2 kHz
192 kHz	96 kHz

The critical insight is:

Everything above Nyquist must be removed or controlled before sampling.

Otherwise:

$$ f_{alias}=|f-kf_s| $$

and ultrasonic energy folds back into the audible band. Analog Devices explicitly notes that out-of-band tones and even noise above \(f_s/2\) can fold into the audio band.

18. Fourier analysis explains why this matters so much

Any sufficiently well-behaved audio signal can be represented as a collection of sinusoids:

$$ x(t)=\int_{-\infty}^{+\infty}X(f)e^{j2\pi ft}df $$

So if your circuit has transfer function:

$$ H(f) $$

then:

$$ Y(f)=H(f)X(f) $$

This means that an audio circuit is fundamentally a frequency-dependent mathematical operator.

A resistor:

$$ H(f)=constant $$

A capacitor:

$$ H(f)\propto\frac{1}{j\omega} $$

An RC low-pass:

$$ H(f)=\frac{1}{1+j\omega RC} $$

An op-amp filter can produce much more sophisticated \(H(f)\).

And DSP does exactly the same thing mathematically.

19. This is the beautiful bridge between analog and DSP

Your intuition from our earlier DSP discussions is exactly right.

An analog:

$$ RC $$

network and a digital:

$$ IIR/FIR $$

filter are solving fundamentally related problems.

Analog:

$$ \frac{d}{dt} $$

Digital:

$$ x[n]-x[n-1] $$

The digital system approximates the continuous-time system.

This is why DSP engineers talk about:

poles
zeros
transfer functions
frequency response
impulse response
convolution
stability
bandwidth

They are the same conceptual language.

20. A practical "audio component universe"

Here's the table I think you're really looking for.

These aren't absolute physical limits—they are rough regions where component values commonly become useful in music/audio electronics.
```
Component	Practical audio region	Main limiting factor
Resistor	~10 Ω–10 MΩ	noise, loading, current
Precision resistor	~100 Ω–1 MΩ	noise/tolerance
Potentiometer	~1 kΩ–1 MΩ	noise/loading
Capacitor, HF	~10 pF–10 nF	parasitics
Capacitor, signal	~10 nF–10 µF	size, leakage, dielectric
Electrolytic	~1 µF–10,000 µF	leakage/ESR/size
Inductor	~1 µH–10 H	size, DCR, saturation
Audio transformer	~10 Hz–100 kHz typical design territory	core + parasitics
Diode	µA–A	nonlinear conduction
BJT	µA–A	noise/current/thermal
FET	pA–A	capacitance/noise
Op amp	~Hz–MHz+	GBW/stability/slew
Analog switch	audio–MHz	\(R_{ON}\), charge injection
ADC	~16–24 bit audio	noise/linearity
DAC	~16–24 bit audio	noise/linearity
DSP	44.1–192 kHz commonly	computation/aliasing
Clock	44.1/48 kHz families	jitter
Audio PCB traces	essentially DC–MHz electrically	parasitic C/L
```
21. But we can go one level deeper: component values can be derived from audio frequency

This is where I think your idea gets really interesting.

Suppose we define the human audio interval:

$$ 20\le f\le20,000 $$

Then:

Capacitor impedance
$$ X_C=\frac{1}{2\pi fC} $$

We can ask:

What capacitor makes a 10-kΩ circuit transition within the audio band?

Set:

$$ X_C=10k\Omega $$

At 20 Hz:

$$ C\approx796nF $$

At 20 kHz:

$$ C\approx796pF $$

Therefore the same 10-kΩ resistance paired with:

$$ 796pF\rightarrow796nF $$

sweeps a three-decade frequency range:

$$ 20kHz\rightarrow20Hz $$

That's not coincidence.

It's the mathematical reason audio filter components occupy those familiar decades of values.

22. The "audio decade" structure

Audio itself spans:

$$ 20Hz\rightarrow20kHz $$

which is:

$$ 3\text{ decades} $$

approximately:

$$ 20\rightarrow200\rightarrow2k\rightarrow20k $$

And component values naturally mirror this:

Resistance
$$ 100\Omega\rightarrow1k\rightarrow10k\rightarrow100k\rightarrow1M $$
Capacitance
$$ 100pF\rightarrow1nF\rightarrow10nF\rightarrow100nF\rightarrow1\mu F\rightarrow10\mu F $$
Inductance
$$ 10\mu H\rightarrow100\mu H\rightarrow1mH\rightarrow10mH\rightarrow100mH\rightarrow1H $$

So there's a very real sense in which audio engineering is a three-decade frequency problem mapped onto logarithmic component-value spaces.

23. There is another limit: parasitic capacitance

Suppose you have a 100-kΩ resistor.

Even a tiny:

$$ C=10pF $$

creates:

$$ f_c=\frac{1}{2\pi(100k)(10pF)} $$ $$ f_c\approx159kHz $$

Fine.

But:

$$ C=100pF $$

gives:

$$ f_c\approx15.9kHz $$

Suddenly the parasitic capacitance is inside the audio band.

This is why high-impedance audio circuits become increasingly difficult.

The PCB itself becomes part of the circuit.

24. And this is where "human usable" becomes an engineering concept

A component value is useful only if:

$$ \text{desired signal} > \text{noise} $$

and:

$$ \text{desired bandwidth} < \text{physical bandwidth} $$

and:

$$ \text{signal amplitude} < \text{linear operating range} $$

and:

$$ \text{power} < \text{thermal limit} $$

and:

$$ \text{distortion} < \text{perceptual/technical threshold} $$

and, for digital systems:

$$ f_{signal}<\frac{f_s}{2} $$

with adequate filtering margin.

25. The really interesting part: audio has multiple independent "limits"

We can therefore construct an audio engineering constraint space:

Frequency
$$ 20Hz\lesssim f\lesssim20kHz $$
Sampling
$$ f_s>2f_{max} $$
Amplitude

determined by the particular signal chain, e.g.

$$ \sim1.2V_{RMS} $$

for nominal professional line level, but considerably higher with headroom.

Dynamic range

roughly:

$$ 120dB $$

of human acoustic range.

Resolution

roughly:

$$ 16-24\text{ bits} $$

for digital audio applications.

Analog bandwidth

usually significantly greater than:

$$ 20kHz $$
Slew rate
$$ SR>2\pi f_{max}V_{peak} $$

with engineering margin.

Noise

must remain below the desired signal/dynamic-range requirement.

Distortion

must remain below either:

audibility threshold
desired technical specification
intentional creative target
26. And this explains something fascinating about old DSPs

This ties directly into what you were asking me about the old Yamaha/Motorola audio DSPs.

A 100-MHz DSP doesn't need to "understand" 100 MHz of audio.

If:

$$ f_s=48kHz $$

then each audio sample arrives every:

$$ \frac1{48000}=20.833\mu s $$

A 100-MHz processor has:

$$ 100,000,000/48,000 $$

approximately:

$$ 2083 $$

clock cycles per sample.

So even a relatively primitive DSP can perform a surprisingly large number of operations per audio sample.

And if the DSP is doing something like:

$$ y[n]=a_0x[n]+a_1x[n-1]+b_1y[n-1] $$

it doesn't need GHz.

It needs:

deterministic execution
multiply-accumulate operations
memory bandwidth
sufficient precision
predictable latency

This is why old audio DSPs can look absurdly slow compared with a modern CPU while still being perfectly capable audio processors.

27. The ultimate picture

I would summarize the whole subject like this:
```
                 HUMAN HEARING
              ~20 Hz ─── ~20 kHz
                     │
                     ▼
              AUDIO BANDWIDTH
                ~20 Hz–20 kHz
                     │
          ┌──────────┴──────────┐
          ▼                     ▼
      ANALOG                   DIGITAL
          │                     │
          │                  fS ≥ 2fMAX
          │                     │
     R C L devices          ADC / DAC
          │                     │
          ▼                     ▼
    Differential            Difference
     equations               equations
          │                     │
          ▼                     ▼
     Transfer H(s)          Transfer H(z)
          │                     │
          └──────────┬──────────┘
                     ▼
                FOURIER DOMAIN
                     │
                     ▼
             poles / zeros /
          bandwidth / phase /
           noise / distortion
                     │
                     ▼
              PERCEPTION
                     │
          ┌──────────┴─────────┐
          ▼                    ▼
       audible             inaudible
       useful               waste/
       region              interference
```
And that is why your idea of applying calculus-derived minima and maxima to audio component values is fundamentally sound.

The only correction I'd make is:

Don't define component limits directly from human hearing. Define them from the electrical requirements necessary to faithfully represent the human-audible signal.

That distinction is huge.

A capacitor may need to operate at 100 MHz even though we're processing 20-kHz audio, because its parasitic behavior affects the 20-kHz circuit.

An op amp may need 20 MHz of bandwidth even though the signal stops at 20 kHz.

An ADC running at 96 kHz has a 48-kHz Nyquist limit even though humans nominally stop around 20 kHz.

And a DSP running at 100 MHz can be perfectly adequate for 48-kHz audio because it has thousands of processor cycles available per sample.

So the really interesting next step would be to build an "Audio Electronics Physical Limits Table" where we derive, mathematically, the useful ranges for R, C, L, transformers, diodes, BJTs, FETs, op-amps, ADCs, DACs, DSPs, clock rates, voltage rails, current, noise, THD, slew rate, and sample rates, with minimum → nominal → maximum values, and explicitly calculate each boundary from 20 Hz / 20 kHz / 48 kHz / 96 kHz / human dynamic range.

No oversampling/anti-aliasing concern here, for what it's worth — these are first-order corner frequencies well below Nyquist even at 44.1 kHz, not something generating harmonics that could fold back; that consideration matters more once we get to the Rectifier & Fuzz plugin (nonlinear diode stuff genuinely can generate energy above Nyquist)
