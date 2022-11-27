/*
 *	SETHARES: The spectrum of an individual tone determines what notes (forming a scale)
 *	will sound in-tune or consonant, and which not. e.g. in tempered or just intonation,
 *	the octave sounds in tune because of the harmonic structure of the spectra of tones,
 *	but we can construct spectra such that a minor-9th sounds less harsh than an octave.
 *
 *	Naive solution
 *
 *	(1) m scale-tones -> (m-1) interval ratios r[1],r[2],r[3] .....r[m-1]
 *	(2) Choose a set of n partials f[1],f[2]....f[n], and amplitudes a[1],a[2]...a[n]
 *		to minimise the sum of the dissonance over all the (m-1) intervals.
 *
 *	Constraints required
 *
 *	(1) a[k] all set to zero always give minimal dissonance
 *	(2)	r[k] all very large also minimises dissonance
 *
 *	so we want to avoid these special cases.
 *
 *
 *	INTRINSIC OR INHERENT DISSONANCE
 *
 *	Dissonance of a spectrum (the intrinsic or inherent dissonance - within one tone)
 *
 *	Df = 1/2 Z(j=1 to n)Z(k=1 to n)d(f[j],f[k],l[a],l[k])
 *
 *	where d(f[j],f[k],l[a],l[k]) = dissonance between partials f[j],f[k] with loudnesses l[j],l[k].
 *
 *	and "Z" = Sigma (i.e. sum over all values from j = 1 to j = n)
 *
 * (see accompanying text document)
 *
 *	Dissonance is defined in terms of SENSORY DISSONANCE.
 *
 *	If we play two sine tones of the same frequency together we hear 1 tone.
 *	If we then gradually increase the frequency of the 2nd tone ..
 *	(1) When tones are very close, we hear one pitch, with slow beating, relatively pleasant
 *		(resolution of the ear cannot distinguish the two tones)
 *	(2) The beating then becomes rapid and "rough" or "gritty", the peak of sensory dissonance.
 *	(3) The two tones then gradually become distinguishable and dissonance fades away,
 *		until we hear 2 clear tones.
 *
 *	The shape of the dissonance curve varies with the frequency chosen for the FIXED tone.
 * (see accompanying text document)
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */