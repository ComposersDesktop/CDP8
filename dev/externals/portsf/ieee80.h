
/***************************************************************\
*   IEEE80.h							*
*   Convert between "double" and IEEE 80 bit format  		*
*   in machine independent manner.				*
*   Assumes array of char is a continuous region of 8 bit frames*
*   Assumes (unsigned long) has 32 useable bits			*
*   billg, dpwe @media.mit.edu					*
*   01aug91							*
*   19aug91  aldel/dpwe  workaround top bit problem in Ultrix   *
*                        cc's double->ulong cast		*
*   05feb92  dpwe/billg  workaround top bit problem in 		*
*                        THINKC4 + 68881 casting		*
\***************************************************************/

double ieee_80_to_double(unsigned char *p);
void   double_to_ieee_80(double val, unsigned char *p);
