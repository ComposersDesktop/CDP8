/***************************************************************\
*   IEEE80.c							*
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
#include <stdio.h>
#include <stdlib.h>
#include 	<math.h>

#include	"ieee80.h"

/* #define MAIN	1	 to compile test routines */

#define ULPOW2TO31	((unsigned long)0x80000000L)
#define DPOW2TO31	((double)2147483648.0)	/* 2^31 */

/* have to deal with ulong's 32nd bit conditionally as double<->ulong casts 
   don't work in some C compilers */

static double   myUlongToDouble (unsigned long ul);
static unsigned long myDoubleToUlong (double val);

static double myUlongToDouble(unsigned long ul)
{
	double val;
	
	/* in THINK_C, ulong -> double apparently goes via long, so can only 
	   apply to 31 bit numbers.  If 32nd bit is set, explicitly add on its
	   value */
	if(ul & ULPOW2TO31)
		val = DPOW2TO31 + (ul & (~ULPOW2TO31));
	else
		val = ul;
	return val;
}

static unsigned long myDoubleToUlong(double	val)
{
	unsigned long ul;
	
	/* cannot cast negative numbers into unsigned longs */
	if(val < 0)	
	{ 
		fprintf(stderr,"IEEE80:DoubleToUlong: val < 0\n"); 
	}
	
	/* in ultrix 4.1's cc, double -> unsigned long loses the top bit, 
	   so we do the conversion only on the bottom 31 bits and set the 
	   last one by hand, if val is truly that big */
	/* should maybe test for val > (double)(unsigned long)0xFFFFFFFF ? */
	if(val < DPOW2TO31)
		ul = (unsigned long)val;
	else
		ul = ULPOW2TO31 | (unsigned long)(val-DPOW2TO31);
	return ul;
}


/*
* Convert IEEE 80 bit floating point to double.
* Should be portable to all C compilers.
*/
double ieee_80_to_double(unsigned char *p)
{
	char sign;
	short exp = 0;
	unsigned long mant1 = 0;
	unsigned long mant0 = 0;
	double val;
	
	exp = *p++;
	exp <<= 8;
	exp |= *p++;
	sign = (exp & 0x8000) ? 1 : 0;
	exp &= 0x7FFF;
	
	mant1 = *p++;
	mant1 <<= 8;
	mant1 |= *p++;
	mant1 <<= 8;
	mant1 |= *p++;
	mant1 <<= 8;
	mant1 |= *p++;
	
	mant0 = *p++;
	mant0 <<= 8;
	mant0 |= *p++;
	mant0 <<= 8;
	mant0 |= *p++;
	mant0 <<= 8;
	mant0 |= *p++;
	
	/* special test for all bits zero meaning zero 
	   - else pow(2,-16383) bombs */
	if(mant1 == 0 && mant0 == 0 && exp == 0 && sign == 0)
		return 0.0;
	else{
		val = myUlongToDouble(mant0) * pow(2.0,-63.0);
		val += myUlongToDouble(mant1) * pow(2.0,-31.0);
		val *= pow(2.0,((double) exp) - 16383.0);
		return sign ? -val : val;
	}
}

/*
* Convert double to IEEE 80 bit floating point
* Should be portable to all C compilers.
* 19aug91 aldel/dpwe  covered for MSB bug in Ultrix 'cc'
*/

void double_to_ieee_80(double val,unsigned char *p)
{
	char sign = 0;
	short exp = 0;
	unsigned long mant1 = 0;
	unsigned long mant0 = 0;
	
	if(val < 0.0)	{  sign = 1;  val = -val; }
	
	if(val != 0.0)	/* val identically zero -> all elements zero */
	{
		exp = (short)(log(val)/log(2.0) + 16383.0);
		val *= pow(2.0, 31.0+16383.0-(double)exp);
		mant1 = myDoubleToUlong(val);
		val -= myUlongToDouble(mant1);
		val *= pow(2.0, 32.0);
		mant0 = myDoubleToUlong(val);
	}
	
	*p++ = ((sign<<7)|(exp>>8));
	*p++ = 0xFF & exp;
	*p++ = (char)(0xFF & (mant1>>24));
	*p++ = (char)(0xFF & (mant1>>16));
	*p++ = (char)(0xFF & (mant1>> 8));
	*p++ = (char)(0xFF & (mant1));
	*p++ = (char)(0xFF & (mant0>>24));
	*p++ = (char)(0xFF & (mant0>>16));
	*p++ = (char)(0xFF & (mant0>> 8));
	*p++ = (char)(0xFF & (mant0));
	
}
