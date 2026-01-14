CDP OSX program updates January 2026

blur:    fix to option "drunk" (bug handling to/fro seeks in a  pvx input file).
fractal: restored correct version of 'spectrum'
repitch: fix to 'getpitch"
pvocex2: reduced low FFT size limit from 64 to 16
pvoc:    updated to support FFT sizes up to 32768 as per usage message (requires pvx format output).
         (the old .ana format cannot go beyond c=8192).

         This required updates to "cdparams" and "cdparams_other" (used by Soundloom)
     Matching changes:
         spec
         specanal
         spectrum
dirsf:   (recognise .pvx files with the new window sizes)

SoundloomE.app
 built with updated tclkit version 8.6.x
 - expected to improve GUI handling on newer Mac platforms.

