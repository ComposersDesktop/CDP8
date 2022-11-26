Preliminary documentation  for fastconv.


FASTCONV:  multi-channel fast convolution (using FFTs)

version: 1.0 2010

usage message: 
fastconv [-aX][-f] infile impulsefile outfile [dry]
   -aX        : scale output amplitude by X
   -f         : write output as floats (no clipping)
  infile      : input soundfile to be processed.
  impulsefile : soundfile or text file containing impulse response,
                  e.g. reverb or FIR filter.
                (text file name must have extension .txt)
              Supported channel combinations:
               (a) mono infile, N-channel impulsefile;
               (b) channels are the same;
               (c) mono impulsefile, N-channel infile.
  [dry]       :  set dry/wet mix (e.g. for reverb)
                 Range: 0.0 - 1.0,  default = 0.0
                 (uses sin/cos law for constant power mix)
Note: some recorded reverb impulses effectively include the direct signal.
In such cases  <dry>  need not be used
Where impulsefile is a filter response (FIR), optimum length is power-of-two - 1.


The primary application of fastconv is convolution reverberation using a 
sampled impulse response of a building or other responsive space. The term "fast"  
refers to the use of the Fast Fourier transform (FFT) to perfume the convolution. 
The program can also be used more experimentally, as the impulse response input can 
be any mono or multi-channel file (see details of available channel combinations below); 
a file can also be convolved with itself. 
The program uses double-precision processing throughout, and files of considerable 
length can be processed cleanly.
Note however that the FFT of the impulse response is stored in main memory, 
so very large files may raise memory demands to critical levels.


More on channel options:

  Impulse soundfile can be multi-channel.
  The .amb format is supported, also Logic Pro SDIR files for reading (change the file extension to .aifc).

  When the infile is multi-channel, impulse file must be mono, or the same number of channels.
        Where the impulse file is mono, data is duplicated for all input channels.
		Typical usage: linear-phase filtering. Output should be 100% "wet".
	The optimum length for a filter impulse response is power-of-two-1, 
	e.g. 127,255, 511 etc. Most filter creation tools will output a file of this size.

	(More tools to support linear-phase filtering are in preparation!).


  When infile is mono, impulse soundfile can be multi-channel, outfile has channel count of
      impulse response.
    Typical usage: reverb convolution for spatialization, B-Format convolution
	It will be usual to supply a non-zero value for "dry", e.g. 0.5. Note however that some recorded 
       or synthetic impulse responses  may already include a "direct" component. In such cases, 
	a "dry" value may not be needed.

  The program employs an rms-based gain scaling algorithm which attempts to ensure all outputs are
 approximately at the same level as the input. In normal use (e.g. a naturally decaying reverb impulse response),
 the -a flag should not be needed.
When the -f flag is used, output is forced to floats, with no clipping. The program reports 
the output level together with a suggested corrective gain factor. This will be of particular relevance to more
experimental procedures, such as convolving a soundfile with itself or with some other arbitrary source.


Note for advanced users - use for FIR linear-phase filtering.

Convolution implements a filtering process - equivalent to multiplication of the spectra of the two inputs. 
The most common filter in audio work is recursive - it recycles output samples back into the input. The output 
continues in principle for ever, hence the term "infinite impulse response" (IIR). The advantage of this technique 
(as employed for example in CDP's "filter' package) is that even a low-order filter (i.e. using a small 
number of delayed inputs and outputs) can be very powerful in its effects. A disadvantage in some applications 
can be that an IIR filter changes the phase of components in the input (frequency components are 
delayed by different amounts). This means among the things that the waveform shape of the input is not 
preserved. The timbre of the sound (even in regions  not directly boosted 
or attenuated by the filter) will therefore be changed. In common parlance, such a filter "colours" the signal.

The alternative is a linear phase filter, which preserves all phase relationships in the signal. 
All frequency components are delayed equally. To achieve this the impulse response must have a symmetrical 
shape (see illustration). The response decays identically either side of the central peak.  

This requires that there be no recirculated outputs reinjected into the filter. 
Such a filter has a Finite Impulse Response (FIR). The impulse response data now comprises literally the 
response of the filter to a single input sample (impulse). An impulse response of 31 samples means 
that the filter generates and adds together 31 delayed copies of the input, to create each output sample. 
While IIR filter coefficients may involve only two delayed samples ("a "second-order" filter), FIR responses 
need to employ many more samples to achieve similar effects. It would not be unusual to use a 511-th order FIR filter. 
This also means that the overall delay ("latency") of a FIR filter is much longer than that of an IIR filter. 

A FIR filter cannot resonate as an IIR filter can. By computing only delayed inputs, It is unconditionally 
"stable" - whereas a badly designed IIR filter can "blow up" with output values rapidly exceeding the sample limit.

Fastconv supports the use of FIR coefficient files either in the form of either a short soundfile, or a 
plain text file containing (in a single column)  the list of coefficients as floating-point numbers within 
the "normalised" range -1.0 to 1.0. For orthodox filtering purposes a mono soundfile should be used, to process 
all channels identically. FIR coefficient text files are generated by many engineering-oriented filter design 
applications.  User may be tempted to  write response files by hand; this can be done, but the results will be 
virtually impossible to predict or control. 

For maximum efficiency, such files should ideally have a size that is a power-of-two less one: e.g. 255, 511, 1023, etc.

For more information about FIR filters, see :

http://www.labbookpages.co.uk/audio/firWindowing.html


August 23 2010
