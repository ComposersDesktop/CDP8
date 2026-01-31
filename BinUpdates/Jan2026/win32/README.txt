CDP WIN32 program updates January 2026

blur:    fix to option "drunk" (bug handling to/fro seeks in a  pvx input file).
fractal: restored correct version of 'spectrum'
repitch: fix to 'getpitch"
pvocex2: reduced low FFT size limit from 64 to 16
pvoc:    updated to support FFT sizes up to 32768 as in usage message (requires pvx format output).
         (the old .ana format cannot go beyond c=8192).

         This required updates to "cdparams" and "cdparams_other" (used by Soundloom)
     Matching changes:
         spec
         specanal
         spectrum
dirsf:   (recognise .pvx files with the new window sizes)

SoundloomE.exe
          fix to handling of MIDI input via program TV.exe
