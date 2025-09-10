/*
 * Copyright (c) 1983-2023 Trevor Wishart and Composers Desktop Project Ltd
 * http://www.trevorwishart.co.uk
 * http://www.composersdesktop.com
 *
 This file is part of the CDP System.

    The CDP System is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    The CDP System is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with the CDP System; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
 *
 */



#include <stdio.h>
#include <stdlib.h>
#include <structures.h>
#include <globcon.h>
#include <pvoc.h>
#include <math.h>           /*RWD*/

// int fft_(),fftmx(),reals_();

/*
 *-----------------------------------------------------------------------
 * subroutine:  fft
 * multivariate complex fourier transform, computed in place
 * using mixed-radix fast fourier transform algorithm.
 *-----------------------------------------------------------------------
 *
 *  this is the call from C:
 *      fft_(anal,banal,&one,&N2,&one,&mtwo);
 *  CHANGED TO:-
 *      fft_(anal,banal,one,N2,one,mtwo);
 */

int fft_(float *a, float *b, int nseg, int n, int nspn, int isn)
        /*  a: pointer to array 'anal'  */
        /*  b: pointer to array 'banal' */
{
    int exit_status;
    int nfac[16];       /*  These are one bigger than needed   */
                        /*  because wish to use Fortran array  */
                        /* index which runs 1 to n, not 0 to n */

    int     m = 0, nf, k, kt, ntot, j, jj, maxf, maxp=0;

/* work space pointers */
    float   *at, *ck, *bt, *sk;
    int     *np;


/* reduce the pointers to input arrays - by doing this, FFT uses FORTRAN
   indexing but retains compatibility with C arrays */
    a--;    b--;

/*  
 * determine the factors of n
 */
    k=nf=abs(n);
    if(nf==1) 
        return(FINISHED);

    nspn=abs(nf*nspn);
    ntot=abs(nspn*nseg);

    if(isn*ntot == 0){
        sprintf(errstr,"zero in FFT parameters %d %d %d %d",nseg, n, nspn, isn);
        return(DATA_ERROR);
    }
    for (m=0; !(k%16); nfac[++m]=4,k/=16);
    for (j=3,jj=9; jj<=k; j+=2,jj=j*j)
        for (; !(k%jj); nfac[++m]=j,k/=jj);

    if (k<=4){
        kt = m;
        nfac[m+1] = k;
        if(k != 1) 
          m++;
    }
    else{
        if(k%4==0){
          nfac[++m]=2;
          k/=4;
        }

        kt = m;
        maxp = max((kt+kt+2),(k-1));
        for(j=2; j<=k; j=1+((j+1)/2)*2)
            if(k%j==0){
                nfac[++m]=j;
                k/=j;
            }
    }
    if(m <= kt+1) 
        maxp = m + kt + 1;
    if(m+kt > 15) {
        sprintf(errstr,"FFT parameter n has more than 15 factors : %d", n);
        return(DATA_ERROR);
    }
    if(kt!=0){
        j = kt;
        while(j)
            nfac[++m]=nfac[j--];
    }
    maxf = nfac[m-kt];
    if(kt > 0) 
        maxf = max(nfac[kt],maxf);

/*  allocate workspace - assume no errors! */
    at = (float *) calloc(maxf,sizeof(float));
    ck = (float *) calloc(maxf,sizeof(float));
    bt = (float *) calloc(maxf,sizeof(float));
    sk = (float *) calloc(maxf,sizeof(float));
    np = (int *) calloc(maxp,sizeof(int));

/* decrement pointers to allow FORTRAN type usage in fftmx */
    at--;   bt--;   ck--;   sk--;   np--;

/* call fft driver */

    if((exit_status = fftmx(a,b,ntot,nf,nspn,isn,m,&kt,at,ck,bt,sk,np,nfac))<0)
        return(exit_status);

/* restore pointers before releasing */
    at++;   bt++;   ck++;   sk++;   np++;

/* release working storage before returning - assume no problems */
    (void) free(at);
    (void) free(sk);
    (void) free(bt);
    (void) free(ck);
    (void) free(np);
    return(FINISHED);
}

/*
 *-----------------------------------------------------------------------
 * subroutine:  fftmx
 * called by subroutine 'fft' to compute mixed-radix fourier transform
 *-----------------------------------------------------------------------
 */
int fftmx(float *a,float *b,int ntot,int n,int nspan,int isn,int m,int *kt,
            float *at,float *ck,float *bt,float *sk,int *np,int nfac[])
{   
    int i,inc,
        j,jc,jf, jj,
        k, k1, k2, k3=0, k4,
        kk,klim,ks,kspan, kspnn,
        lim,
        maxf,mm,
        nn,nt;
    double  aa, aj, ajm, ajp, ak, akm, akp,
        bb, bj, bjm, bjp, bk, bkm, bkp,
        c1, c2=0.0, c3=0.0, c72, cd,
        dr,
        rad, 
        sd, s1, s2=0.0, s3=0.0, s72, s120;

    double  xx; /****** ADDED APRIL 1991 *********/
    inc=abs(isn);
    nt = inc*ntot;
        ks = inc*nspan;
/******************* REPLACED MARCH 29: ***********************
                    rad = atan((double)1.0);
**************************************************************/
    rad = 0.785398163397448278900;
/******************* REPLACED MARCH 29: ***********************
                        s72 = rad/0.625;
                        c72 = cos(s72);
                        s72 = sin(s72);
**************************************************************/
    c72 = 0.309016994374947451270;
    s72 = 0.951056516295153531190;
/******************* REPLACED MARCH 29: ***********************
                        s120 = sqrt((double)0.75);
**************************************************************/
        s120 = 0.866025403784438707600;

/* scale by 1/n for isn > 0 ( reverse transform ) */

        if (isn < 0){
            s72 = -s72;
            s120 = -s120;
            rad = -rad;}
    else{   ak = 1.0/(double)n;
        for(j=1; j<=nt;j += inc){
                a[j] = (float)(a[j] * ak);
                b[j] = (float)(b[j] * ak);
        }
    }
    kspan = ks;
        nn = nt - inc;
        jc = ks/n;

/* sin, cos values are re-initialized each lim steps  */

        lim = 32;
        klim = lim * jc;
        i = 0;
        jf = 0;
        maxf = m - (*kt);
        maxf = nfac[maxf];
        if((*kt) > 0) 
        maxf = max(nfac[*kt],maxf);

/*
 * compute fourier transform
 */

lbl40:
    dr = (8.0 * (double)jc)/((double)kspan);
/*************************** APRIL 1991 POW & POW2 not WORKING.. REPLACE *******
            cd = 2.0 * (pow2 ( sin((double)0.5 * dr * rad)) );
*******************************************************************************/
    xx =  sin((double)0.5 * dr * rad);
        cd = 2.0 * xx * xx;
        sd = sin(dr * rad);
        kk = 1;
        if(nfac[++i]!=2) goto lbl110;
/*
 * transform for factor of 2 (including rotation factor)
 */
        kspan /= 2;
        k1 = kspan + 2;
        do{ do{ k2 = kk + kspan;
                ak = a[k2];
                bk = b[k2];
                a[k2] = (float)((a[kk]) - ak);
                b[k2] = (float)((b[kk]) - bk);
                a[kk] = (float)((a[kk]) + ak);
                b[kk] = (float)((b[kk]) + bk);
                kk = k2 + kspan;
        } while(kk <= nn);
            kk -= nn;
    }while(kk <= jc);
        if(kk > kspan) goto lbl350;
lbl60:  c1 = 1.0 - cd;
        s1 = sd;
        mm = min((k1/2),klim);
        goto lbl80;
lbl70:  ak = c1 - ((cd*c1)+(sd*s1));
        s1 = ((sd*c1)-(cd*s1)) + s1;
        c1 = ak;
lbl80:  do{ do{ k2 = kk + kspan;
                ak = a[kk] - a[k2];
                bk = b[kk] - b[k2];
                a[kk] = a[kk] + a[k2];
                b[kk] = b[kk] + b[k2];
                a[k2] = (float)((c1 * ak) - (s1 * bk));
                b[k2] = (float)((s1 * ak) + (c1 * bk));
                kk = k2 + kspan;
        }while(kk < nt);
            k2 = kk - nt;
            c1 = -c1;
            kk = k1 - k2;
    }while(kk > k2);
        kk += jc;
        if(kk <= mm) goto lbl70;
        if(kk < k2)  goto lbl90;
        k1 += (inc + inc);
        kk = ((k1-kspan)/2) + jc;
        if(kk <= (jc+jc)) goto lbl60;
        goto lbl40;
lbl90:  s1 = ((double)((kk-1)/jc)) * dr * rad;
        c1 = cos(s1);
        s1 = sin(s1);
        mm = min( k1/2, mm+klim);
        goto lbl80;
/*
 * transform for factor of 3 (optional code)
 */


lbl100: k1 = kk + kspan;
    k2 = k1 + kspan;
    ak = a[kk];
    bk = b[kk];
        aj = a[k1] + a[k2];
        bj = b[k1] + b[k2];
        a[kk] = (float)(ak + aj);
        b[kk] = (float)(bk + bj);
        ak += (-0.5 * aj);
        bk += (-0.5 * bj);
        aj = (a[k1] - a[k2]) * s120;
        bj = (b[k1] - b[k2]) * s120;
        a[k1] = (float)(ak - bj);
        b[k1] = (float)(bk + aj);
        a[k2] = (float)(ak + bj);
        b[k2] = (float)(bk - aj);
        kk = k2 + kspan;
        if(kk < nn)     goto lbl100;
        kk -= nn;
        if(kk <= kspan) goto lbl100;
        goto lbl290;

/*
 * transform for factor of 4
 */

lbl110: if(nfac[i] != 4) goto lbl230;
        kspnn = kspan;
        kspan = kspan/4;
lbl120: c1 = 1.0;
        s1 = 0;
        mm = min( kspan, klim);
        goto lbl150;
lbl130: c2 = c1 - ((cd*c1)+(sd*s1));
        s1 = ((sd*c1)-(cd*s1)) + s1;
/*
 * the following three statements compensate for truncation
 * error.  if rounded arithmetic is used, substitute
 * c1=c2
 *
 * c1 = (0.5/(pow2(c2)+pow2(s1))) + 0.5;
 * s1 = c1*s1;
 * c1 = c1*c2;
 */
        c1 = c2;
lbl140: c2 = (c1 * c1) - (s1 * s1);
        s2 = c1 * s1 * 2.0;
        c3 = (c2 * c1) - (s2 * s1);
        s3 = (c2 * s1) + (s2 * c1);
lbl150: k1 = kk + kspan;
        k2 = k1 + kspan;
        k3 = k2 + kspan;
        akp = a[kk] + a[k2];
        akm = a[kk] - a[k2];
        ajp = a[k1] + a[k3];
        ajm = a[k1] - a[k3];
        a[kk] = (float)(akp + ajp);
        ajp = akp - ajp;
        bkp = b[kk] + b[k2];
        bkm = b[kk] - b[k2];
        bjp = b[k1] + b[k3];
        bjm = b[k1] - b[k3];
        b[kk] = (float)(bkp + bjp);
        bjp = (float)(bkp - bjp);
        if(isn < 0) goto lbl180;
        akp = akm - bjm;
        akm = akm + bjm;
        bkp = bkm + ajm;
        bkm = bkm - ajm;
        if(s1 == 0.0) goto lbl190;
lbl160: a[k1] = (float)((akp*c1) - (bkp*s1));
        b[k1] = (float)((akp*s1) + (bkp*c1));
        a[k2] = (float)((ajp*c2) - (bjp*s2));
        b[k2] = (float)((ajp*s2) + (bjp*c2));
        a[k3] = (float)((akm*c3) - (bkm*s3));
        b[k3] = (float)((akm*s3) + (bkm*c3));
        kk = k3 + kspan;
        if(kk <= nt)   goto lbl150;
lbl170: kk -= (nt - jc);
        if(kk <= mm)   goto lbl130;
        if(kk < kspan) goto lbl200;
        kk -= (kspan - inc);
        if(kk <= jc)   goto lbl120;
        if(kspan==jc)  goto lbl350;
        goto lbl40;
lbl180: akp = akm + bjm;
        akm = akm - bjm;
        bkp = bkm - ajm;
        bkm = bkm + ajm;
        if(s1 != 0.0)  goto lbl160;
lbl190: a[k1] = (float)akp;
        b[k1] = (float)bkp;
        a[k2] = (float)ajp;
        b[k2] = (float)bjp;
        a[k3] = (float)akm;
        b[k3] = (float)bkm;
        kk = k3 + kspan;
        if(kk <= nt) goto lbl150;
        goto lbl170;
lbl200: s1 = ((double)((kk-1)/jc)) * dr * rad;
        c1 = cos(s1);
        s1 = sin(s1);
        mm = min( kspan, mm+klim);
        goto lbl140;

/*
 * transform for factor of 5 (optional code)
 */

lbl210: c2 = (c72*c72) - (s72*s72);
        s2 = 2.0 * c72 * s72;
lbl220: k1 = kk + kspan;
        k2 = k1 + kspan;
        k3 = k2 + kspan;
        k4 = k3 + kspan;
        akp = a[k1] + a[k4];
        akm = a[k1] - a[k4];
        bkp = b[k1] + b[k4];
        bkm = b[k1] - b[k4];
        ajp = a[k2] + a[k3];
        ajm = a[k2] - a[k3];
        bjp = b[k2] + b[k3];
        bjm = b[k2] - b[k3];
        aa = a[kk];
        bb = b[kk];
        a[kk] = (float)(aa + akp + ajp);
        b[kk] = (float)(bb + bkp + bjp);
        ak = (akp*c72) + (ajp*c2) + aa;
        bk = (bkp*c72) + (bjp*c2) + bb;
        aj = (akm*s72) + (ajm*s2);
        bj = (bkm*s72) + (bjm*s2);
        a[k1] = (float)(ak - bj);
        a[k4] = (float)(ak + bj);
        b[k1] = (float)(bk + aj);
        b[k4] = (float)(bk - aj);
        ak = (akp*c2) + (ajp*c72) + aa;
        bk = (bkp*c2) + (bjp*c72) + bb;
        aj = (akm*s2) - (ajm*s72);
    bj = (bkm*s2) - (bjm*s72);
    a[k2] = (float)(ak - bj);
        a[k3] = (float)(ak + bj);
        b[k2] = (float)(bk + aj);
        b[k3] = (float)(bk - aj);
        kk = k4 + kspan;
        if(kk < nn)     goto lbl220;
        kk -= nn;
        if(kk <= kspan) goto lbl220;
        goto lbl290;

/*
 * transform for odd factors
 */

lbl230: k = nfac[i];
    kspnn = kspan;
    kspan /= k;
    if(k==3)   goto lbl100;
    if(k==5)   goto lbl210;
    if(k==jf)  goto lbl250;
        jf = k;
        s1 = rad/(((double)(k))/8.0);
        c1 = cos(s1);
        s1 = sin(s1);
        ck[jf] = 1.0f;
    sk[jf] = 0.0f;
    for(j=1; j<k ; j++){
        ck[j] = (float)((ck[k])*c1 + (sk[k])*s1);
            sk[j] = (float)((ck[k])*s1 - (sk[k])*c1);
            k--;
            ck[k] = ck[j];
            sk[k] = -(sk[j]);
    }
lbl250: k1 = kk;
        k2 = kk + kspnn;
    aa = a[kk];
    bb = b[kk];
        ak = aa;
        bk = bb;
        j = 1;
        k1 += kspan;
        do{ k2 -= kspan;
            j++;
            at[j] = a[k1] + a[k2];
            ak = at[j] + ak;    
            bt[j] = b[k1] + b[k2];
            bk = bt[j] + bk;    
            j++;
            at[j] = a[k1] - a[k2];
            bt[j] = b[k1] - b[k2];
            k1 += kspan;
    }while(k1 < k2);
        a[kk] = (float)ak;
        b[kk] = (float)bk;
        k1 = kk;
        k2 = kk + kspnn;
        j = 1;
lbl270: k1 += kspan;
        k2 -= kspan;
        jj = j;
        ak = aa;
        bk = bb;
        aj = 0.0;
        bj = 0.0;
        k = 1;
        do{ k++;
            ak = (at[k] * ck[jj]) + ak;
            bk = (bt[k] * ck[jj]) + bk; 
            k++;
            aj = (at[k] * sk[jj]) + aj;
            bj = (bt[k] * sk[jj]) + bj;
            jj += j;
            if (jj > jf) 
            jj -= jf;
    }while(k < jf);
        k = jf - j;
        a[k1] = (float)(ak - bj);
        b[k1] = (float)(bk + aj);
        a[k2] = (float)(ak + bj);
        b[k2] = (float)(bk - aj);
        j++;
        if(j < k)     goto lbl270;
        kk += kspnn;
        if(kk <= nn)  goto lbl250;
        kk -= nn;
        if(kk<=kspan) goto lbl250;

/*
 * multiply by rotation factor (except for factors of 2 and 4)
 */

lbl290: if(i==m) goto lbl350;
        kk = jc + 1;
lbl300: c2 = 1.0 - cd;
        s1 = sd;
        mm = min( kspan, klim);
        goto lbl320;
lbl310: c2 = c1 - ((cd*c1) + (sd*s1));
        s1 = s1 + ((sd*c1) - (cd*s1));
lbl320: c1 = c2;
        s2 = s1;
        kk += kspan;
lbl330: ak = a[kk];
        a[kk] = (float)((c2*ak) - (s2 * b[kk]));
        b[kk] = (float)((s2*ak) + (c2 * b[kk]));
        kk += kspnn;
        if(kk <= nt) goto lbl330;
        ak = s1*s2;
        s2 = (s1*c2) + (c1*s2);
        c2 = (c1*c2) - ak;
        kk -= (nt - kspan);
        if(kk <= kspnn) goto lbl330;
        kk -= (kspnn - jc);
        if(kk <= mm)   goto lbl310;
        if(kk < kspan) goto lbl340;
        kk -= (kspan - jc - inc);
        if(kk <= (jc+jc)) goto lbl300;
        goto lbl40;
lbl340: s1 = ((double)((kk-1)/jc)) * dr * rad;
        c2 = cos(s1);
        s1 = sin(s1);
        mm = min( kspan, mm+klim);
        goto lbl320;

/*
 * permute the results to normal order---done in two stages
 * permutation for square factors of n
 */

lbl350: np[1] = ks;
        if (!(*kt)) goto lbl440;
        k = *kt + *kt + 1;
        if(m < k) 
        k--;
    np[k+1] = jc;
        for(j=1; j < k; j++,k--){
        np[j+1] = np[j] / nfac[j];
            np[k] = np[k+1] * nfac[j];
    }
        k3 = np[k+1];
        kspan = np[2];
        kk = jc + 1;
        k2 = kspan + 1;
        j = 1;
        if(n != ntot) goto lbl400;
/*
 * permutation for single-variate transform (optional code)
 */
lbl370: do{ ak = a[kk];
            a[kk] = a[k2];
            a[k2] = (float)ak;
            bk = b[kk];
            b[kk] = b[k2];
            b[k2] = (float)bk;
            kk += inc;
            k2 += kspan;
    }while(k2 < ks);
lbl380: do{ k2 -= np[j++];
            k2 += np[j+1];
    }while(k2 > np[j]);
        j = 1;
lbl390: if(kk < k2){
        goto lbl370;
    }
        kk += inc;
        k2 += kspan;
        if(k2 < ks) goto lbl390;
        if(kk < ks) goto lbl380;
        jc = k3;
        goto lbl440;
/*
 * permutation for multivariate transform
 */
lbl400: do{ do{ k = kk + jc;
                do{ ak = a[kk];
                    a[kk] = a[k2];
                    a[k2] = (float)ak;
                    bk = b[kk];
                    b[kk] = b[k2];
                    b[k2] = (float)bk;
                    kk += inc;
                    k2 += inc;
            }while(kk < k);
                kk += (ks - jc);
                k2 += (ks - jc);
        }while(kk < nt);
            k2 -= (nt - kspan);
            kk -= (nt - jc);
    }while(k2 < ks);
lbl420: do{ k2 -= np[j++];
            k2 += np[j+1];
    }while(k2 > np[j]);
        j = 1;
lbl430: if(kk < k2)      goto lbl400;
        kk += jc;
        k2 += kspan;
        if(k2 < ks)      goto lbl430;
        if(kk < ks)      goto lbl420;
        jc = k3;
lbl440: if((2*(*kt))+1 >= m)
        return(FINISHED);

        kspnn = *(np + *(kt) + 1);
        j = m - *kt;        
        nfac[j+1] = 1;
lbl450: nfac[j] = nfac[j] * nfac[j+1];
        j--;
        if(j != *kt) goto lbl450;
        *kt = *(kt) + 1;
        nn = nfac[*kt] - 1;
        jj = 0;
        j = 0;
        goto lbl480;
lbl460: jj -= k2;
        k2 = kk;
        kk = nfac[++k];
lbl470: jj += kk;
        if(jj >= k2) goto lbl460;
        np[j] = jj;
lbl480: k2 = nfac[*kt];
        k = *kt + 1;    
        kk = nfac[k];
        j++;
        if(j <= nn) goto lbl470;
/* Determine permutation cycles of length greater than 1 */
        j = 0;
        goto lbl500;
lbl490: k = kk;
        kk = np[k]; 
        np[k] = -kk;    
        if(kk != j) goto lbl490;
        k3 = kk;
lbl500: kk = np[++j];   
        if(kk < 0)  goto lbl500;
        if(kk != j) goto lbl490;
        np[j] = -j;
        if(j != nn) goto lbl500;
        maxf *= inc;
/* Perform reordering following permutation cycles */
        goto lbl570;
lbl510: j--;
        if (np[j] < 0) goto lbl510;
        jj = jc;
lbl520: kspan = jj;
        if(jj > maxf) 
        kspan = maxf;
        jj -= kspan;
        k = np[j];  
        kk = (jc*k) + i + jj;
        k1 = kk + kspan;
        k2 = 0;
lbl530: k2++;
        at[k2] = a[k1];
        bt[k2] = b[k1];
        k1 -= inc;
        if(k1 != kk) goto lbl530;
lbl540: k1 = kk + kspan;
        k2 = k1 - (jc * (k + np[k]));
        k = -(np[k]);
lbl550: a[k1] = a[k2];
        b[k1] = b[k2];
        k1 -= inc;
        k2 -= inc;
        if(k1 != kk) goto lbl550;
        kk = k2;
        if(k != j)   goto lbl540;
        k1 = kk + kspan;
        k2 = 0;
lbl560: k2++;
        a[k1] = at[k2];
        b[k1] = bt[k2];
        k1 -= inc;
        if(k1 != kk) goto lbl560;
        if(jj)       goto lbl520;
        if(j  != 1)  goto lbl510;
lbl570: j = k3 + 1;
        nt -= kspnn;
        i = nt - inc + 1;
        if(nt >= 0)  goto lbl510;
        return(FINISHED);; 
}


/*
 *-----------------------------------------------------------------------
 * subroutine:  reals
 * used with 'fft' to compute fourier transform or inverse for real data
 *-----------------------------------------------------------------------
 *  this is the call from C:
 *      reals_(anal,banal,N2,mtwo);
 *  which has been changed from CARL call
 *      reals_(anal,banal,&N2,&mtwo);
 */

int reals_(float *a, float *b, int n, int isn)

            /* a refers to an array of floats 'anal'   */
            /* b refers to an array of floats 'banal'  */
/* See IEEE book for a long comment here on usage */

{   int inc,
        j,
        k,
        lim,
        mm,ml,
        nf,nk,nh;
 
    double  aa,ab,
        ba,bb,
        cd,cn,
        dr,
        em,
        rad,re,
        sd,sn;
    double  xx; /******* ADDED APRIL 1991 ******/
/* adjust  input array pointers (called from C) */
    a--;    b--;
    inc=abs(isn);
    nf=abs(n);
        if(nf*isn==0){
            sprintf(errstr,"zero in reals parameters in FFT : %d : %d ",n,isn);
            return(DATA_ERROR);;
        }
        nk = (nf*inc) + 2;
        nh = nk/2;
/*****************************
        rad  = atan((double)1.0);
******************************/
    rad = 0.785398163397448278900;
        dr = -4.0/(double)(nf);
/********************************** POW2 REMOVED APRIL 1991 *****************
                    cd = 2.0 * (pow2(sin((double)0.5 * dr * rad)));
*****************************************************************************/
    xx = sin((double)0.5 * dr * rad);
        cd = 2.0 * xx * xx;
        sd = sin(dr * rad);
/*
 * sin,cos values are re-initialized each lim steps
 */
        lim = 32;
        mm = lim;
        ml = 0;
        sn = 0.0;
    if(isn<0){
        cn = 1.0;
        a[nk-1] = a[1];
        b[nk-1] = b[1]; }
    else {
        cn = -1.0;
        sd = -sd;
    }
        for(j=1;j<=nh;j+=inc)   {
            k = nk - j;
            aa = a[j] + a[k];
            ab = a[j] - a[k];
            ba = b[j] + b[k];
            bb = b[j] - b[k];
            re = (cn*ba) + (sn*ab);
            em = (sn*ba) - (cn*ab);
            b[k] = (float)((em-bb)*0.5);
            b[j] = (float)((em+bb)*0.5);
            a[k] = (float)((aa-re)*0.5);
        a[j] = (float)((aa+re)*0.5);
            ml++;
        if(ml!=mm){
            aa = cn - ((cd*cn)+(sd*sn));
            sn = ((sd*cn) - (cd*sn)) + sn;
            cn = aa;}
        else {
            mm +=lim;
            sn = ((float)ml) * dr * rad;
            cn = cos(sn);
            if(isn>0)
                cn = -cn;
            sn = sin(sn);
        }
    }
    return(FINISHED);
}
