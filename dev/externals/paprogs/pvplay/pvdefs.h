/* pvdefs.h */
#ifndef __PVDEFS_H_INCLUDED
#define __PVDEFS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


#define VERY_TINY_VAL (1e-20)

#define ODD(x)	  			((x)&1)
#define EVEN(x)	  			(!ODD(x))
#define CHAN_SRCHRANGE_F	(4)


#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b)) 
#endif
#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b)) 
#endif


/* for future reference: IEEE_DOUBLE not implemented yet for PVOCEX */
enum pvoc_wordformat { PVOC_IEEE_FLOAT, PVOC_IEEE_DOUBLE};
typedef enum pvoc_mode {  PVPP_NOT_SET,PVPP_OFFLINE,PVPP_STREAMING} pvocmode;
/* the old CARL pvoc flags */
typedef enum pvoc_wfac {PVOC_O_W0,PVOC_O_W1,PVOC_O_W2,PVOC_O_W3,PVOC_O_DEFAULT} pvoc_overlapfac;
typedef enum pvoc_scaletype {PVOC_S_TIME,PVOC_S_PITCH,PVOC_S_NONE} pv_scaletype;

typedef enum pvoc_frametype { PVOC_AMP_FREQ,PVOC_AMP_PHASE,PVOC_COMPLEX } pv_frametype;
typedef enum pvoc_windowtype {PVOC_DEFAULT,
								PVOC_HAMMING,
								PVOC_HANN,
								PVOC_KAISER,
								PVOC_RECT,
								PVOC_CUSTOM} pv_wtype;

#ifdef __cplusplus
}
#endif
#endif