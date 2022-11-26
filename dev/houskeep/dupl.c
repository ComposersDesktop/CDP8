/*
 * Copyright (c) 1983-2013 Trevor Wishart and Composers Desktop Project Ltd
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
#include <tkglobals.h>
#include <globcon.h>
#include <cdpmain.h>
#include <house.h>
#include <modeno.h>
#include <pnames.h>
#include <filetype.h>
#include <flags.h>
#include <logic.h>
#include <limits.h>

#include <sfsys.h>
#include <osbind.h>     /*RWD*/

#include <string.h>
#include <math.h>

static int  check_available_space(dataptr dz);
static int  copy_textfile(dataptr dz);

extern unsigned int superzargo;

/************************************ DO_DUPLICATES ********************************/

int do_duplicates(dataptr dz)
{
    int exit_status;
    int n, start, end;
    int old_ofd;
    int namelen, numlen;
    char *outfilename;
//TW REVISION Dec 2002
    char prefix_units[]     = "_00";
    char prefix_tens[]      = "_0";
    char prefix_hundreds[]  = "_";
    char *p, *q, *r;

    numlen  = 4;
    switch(dz->mode) {
    case(COPYSF):
        if(dz->infile->filetype==WORDLIST)
            return copy_textfile(dz);
        dz->tempsize = dz->insams[0];
        switch(dz->infile->filetype) {
        case(PITCHFILE):
            if((exit_status = write_exact_samps(dz->pitches,dz->insams[0],dz))<0)
                return(exit_status);
            break;
        case(TRANSPOSFILE):
            if((exit_status = write_exact_samps(dz->transpos,dz->insams[0],dz))<0)
                return(exit_status);
            break;
        default:
            while(dz->samps_left) {
                if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
                    return(exit_status);
                if(dz->ssampsread > 0) {
                    if((exit_status = write_exact_samps(dz->sampbuf[0],dz->ssampsread,dz))<0)
                        return(exit_status);
                } else if(dz->total_samps_read == 0) {
                    sprintf(errstr,"Failure to read samples from input file.\n");
                    return(SYSTEM_ERROR);
                }
            }
            if(dz->infile->filetype == ENVFILE)
                dz->outfile->window_size = dz->infile->window_size;
            break;
        }
        break;
    case(DUPL):
        if(!sloom) {
            namelen = strlen(dz->wordstor[0]);
            q = dz->wordstor[0];
            r = dz->wordstor[0] + namelen;
            p = r - 1;
            while((*p != '\\') && (*p != '/') && (*p != ':')) {
                p-- ;
                if(p < dz->wordstor[0])
                    break;
            }
            if(p > dz->wordstor[0]) {
                p++;
                while(p <= r)
                    *q++ = *p++;
            }
        }
        namelen = strlen(dz->wordstor[0]);      
        superzargo = 0; /* timer-base */
        old_ofd = dz->ofd;
        if((exit_status = check_available_space(dz))<0)
            return(exit_status);
//TW ????
        if(!sloom)
            dz->ofd = -1;
        else
            dz->ofd = old_ofd;
        if((dz->tempsize = dz->insams[0] * dz->iparam[COPY_CNT])<0)
            dz->tempsize = INT_MAX; /* ERROR was LONG_MAX **** Overflows **** */

        if(sloom) {
            namelen--;                                  /* Drop the 0 at end of name */
            start = 0;
            end = dz->iparam[COPY_CNT];
        } else {
            start = 1;
            end = dz->iparam[COPY_CNT] + 1;
        }
        for(n=start;n<end;n++) {
            if(sndseekEx(dz->ifd[0],0,0)<0) {
                sprintf(errstr,"sndseek() failed.\n");
                return(SYSTEM_ERROR);
            }
            reset_filedata_counters(dz);
            if((outfilename = (char *)malloc((namelen + numlen + 1) * sizeof(char)))==NULL) {
                sprintf(errstr,"INSUFFICIENT MEMORY for outfilename.\n");
                return(MEMORY_ERROR);
            }               
            strcpy(outfilename,dz->wordstor[0]);

            if(!sloom) {
                if(n<10)
//TW REVISION Dec 2002
                    insert_new_chars_at_filename_end(outfilename,prefix_units);
                else if(n<100)
                    insert_new_chars_at_filename_end(outfilename,prefix_tens);
                else if(n<1000)
                    insert_new_chars_at_filename_end(outfilename,prefix_hundreds);
                else {
                    sprintf(errstr,"Too many duplicates.\n");
                    return(PROGRAM_ERROR);
                }
                insert_new_number_at_filename_end(outfilename,n,0);
            } else {
                insert_new_number_at_filename_end(outfilename,n,1);
            }
            dz->process_type = EQUAL_SNDFILE;   /* allow sndfile to be created */
            if(!sloom || n>0) {
                if((exit_status = create_sized_outfile(outfilename,dz))<0) {
                    if(!sloom) {
                        if(dz->vflag[IGNORE_COPIES]) {
                            fprintf(stdout, "INFO: Soundfile %s already exists\n", outfilename);
                            fflush(stdout);
                            free(outfilename);
                            dz->process_type = OTHER_PROCESS;
                            dz->ofd = -1;
                            continue;
                        }
                        else {
                            sprintf(errstr, "Soundfile %s already exists: Made %d duplicates only.\n",outfilename,n-1);
                            free(outfilename);
                            dz->process_type = OTHER_PROCESS;
                            dz->ofd = -1;
                            return(GOAL_FAILED);
                        }
                    } else {
                        dz->process_type = OTHER_PROCESS;
                        dz->ofd = -1;
                        return(SYSTEM_ERROR);
                    }
                }                           
            }
            dz->process_type = OTHER_PROCESS;
            while(dz->samps_left) {
                if((exit_status = read_samps(dz->sampbuf[0],dz))<0)
                    return(exit_status);
                if(dz->ssampsread > 0) {
                    if((exit_status = write_exact_samps(dz->sampbuf[0],dz->ssampsread,dz))<0)
                        return(exit_status);
                }
            }
            dz->process_type = EQUAL_SNDFILE;       /* allows header to be written  */
            dz->outfiletype  = SNDFILE_OUT;         /* allows header to be written  */
            if((exit_status = headwrite(dz->ofd,dz))<0) {
                free(outfilename);
                return(exit_status);
            }
            dz->process_type = OTHER_PROCESS;       /* restore true status */
            dz->outfiletype  = NO_OUTPUTFILE;       /* restore true status */
            if((exit_status = reset_peak_finder(dz))<0)
                return(exit_status);
            if(sndcloseEx(dz->ofd) < 0) {
                fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
                fflush(stdout);
            }
            free(outfilename);
            dz->ofd = -1;
        }
        break;
    default:
        sprintf(errstr,"Unknown case in do_duplicates()\n");
        return(PROGRAM_ERROR);
    }
    return(FINISHED);
}

/************************************ DO_DELETES ********************************/

int do_deletes(dataptr dz)
{
    char *outfilename;
    int n;
    char prefix_units[]     = "_00";
    char prefix_tens[]      = "_0";
    char prefix_hundreds[]  = "_";
    int namelen = strlen(dz->wordstor[0]);
    int numlen  = 4, unopen, removed;
    unopen = 0;
    removed = 0;
    for(n=1;n<=MAXDUPL;n++) {
        if((!dz->vflag[ALL_COPIES]) && unopen >= COPYDEL_OVERMAX)
            break;
        if((outfilename = (char *)malloc((namelen + numlen + 1) * sizeof(char)))==NULL) {
            sprintf(errstr,"INSUFFICIENT MEMORY for outfilename.\n");
            return(MEMORY_ERROR);
        }               
        strcpy(outfilename,dz->wordstor[0]);
//TW COMMENT: program not available on Sound Loom
//TW REVISIONs Dec 2002 : to do with sending filename extension in cmdline
        if(!sloom) {
            if(n<10)
                insert_new_chars_at_filename_end(outfilename,prefix_units);
            else if(n<100)
                insert_new_chars_at_filename_end(outfilename,prefix_tens);
            else if(n<1000)
                insert_new_chars_at_filename_end(outfilename,prefix_hundreds);
            else {
                sprintf(errstr,"Too many duplicates.\n");
                return(PROGRAM_ERROR);
            }
        }
        insert_new_number_at_filename_end(outfilename,n,0);
        if((dz->ofd = sndopenEx(outfilename,0,CDP_OPEN_RDONLY)) < 0) {   /*RWD don't need sndopenex... */
            unopen++;
            dz->ofd = -1;
            continue;
        }
        if(sndunlink(dz->ofd) < 0) {
            fprintf(stdout,"WARNING: Can't set output soundfile %s for deletion.\n",outfilename);
            fflush(stdout);
            dz->ofd = -1;               
            free(outfilename);
        }
        if(sndcloseEx(dz->ofd) < 0) {
            fprintf(stdout,"WARNING: Can't close output soundfile %s\n",outfilename);
            fflush(stdout);
            dz->ofd = -1;               
            free(outfilename);
        }
        removed++;
        free(outfilename);
        dz->ofd = -1;
    }
    fprintf(stdout,"INFO: %d duplicate files removed.\n",removed);
    fflush(stdout);
    return(FINISHED);
}

/************************************ CHECK_AVAILABLE_SPACE ********************************/

int check_available_space(dataptr dz)
{
    unsigned int slots;
    int outfilesize;

    outfilesize = getdrivefreespace("temp") / sizeof(float);
    slots = outfilesize/dz->insams[0];
    if(slots < (unsigned int)dz->iparam[COPY_CNT]) {
        sprintf(errstr,"Insufficient space on disk to create %d copies.\n"
                       "You have space for %d copies.\n",dz->iparam[COPY_CNT],slots);
        return(GOAL_FAILED);
    }
    return(FINISHED);
}

/************************************ CHECK_AVAILABLE_DISKSPACE ********************************/
/* used for HOUSE DISK only */

#define LEAVESPACE  (10*1024)       /* file space that must be left */
#define DEFAULT_SRATE (44100)

int check_available_diskspace(dataptr dz)
{
    double secs, mins, hrs;
    int srate;
    unsigned int outsamps = 0, orig_outsamps;
    int m, k, kk, spacecnt;
    char temp[800];
    unsigned int freespace = getdrivefreespace("temp") - LEAVESPACE;

    switch(dz->infile->filetype) {
    case(SNDFILE):  srate = dz->infile->srate;  break;
    default:        srate = DEFAULT_SRATE;      break;
    }
    sprintf(errstr,"AVAILABLE DISK SPACE (at sample rate %d)\n",srate);
    sprintf(temp,"%d",freespace);
    strcat(errstr,temp);
    spacecnt = 13 - strlen(temp);
    for(k=0;k<spacecnt;k++)        /* did have ; at end */
        strcat(errstr," ");
    strcat(errstr,"bytes\n");
    splice_multiline_string(errstr,"INFO:");
    fflush(stdout);
    kk = 0;
    while (kk < 4) {
        errstr[0] = ENDOFSTR;
        switch(kk) {
        case(0): outsamps = freespace/2;    break;
        case(1): outsamps = freespace/3;    break;
        case(2): outsamps = freespace/4;    break;
        case(3): outsamps = freespace/sizeof(float);    break;
        }
        sprintf(temp,"%d",outsamps);
        strcat(errstr,temp);
        spacecnt = 13 - strlen(temp);
        for(k=0;k<spacecnt;k++)     /* did have ; at end */
            strcat(errstr," ");
        switch(kk) {
        case(0): strcat(errstr,"16-bit samples\n"); break;
        case(1): strcat(errstr,"24-bit samples\n"); break;
        case(2): strcat(errstr,"32-bit samples\n"); break;
        case(3): strcat(errstr,"float  samples\n"); break;
        }
        orig_outsamps = outsamps;
        for(m=MONO;m<=STEREO;m++) {
            switch(m) {
            case(MONO):
                outsamps = orig_outsamps;
                sprintf(temp,"IN MONO    : ");
                strcat(errstr,temp);
                break;
            case(STEREO):
                outsamps /= 2;
                sprintf(temp,"IN STEREO : ");
                strcat(errstr,temp);
                break;
            }
            secs  = (double)outsamps/srate;
            mins  = floor(secs/60.0);
            secs -= mins * 60.0;
            hrs   = floor(mins/60.0);
            mins -= hrs * 60.0;
            if(hrs > 0.0) {
                sprintf(temp,"%.0lf",hrs);
                strcat(errstr,temp);
                spacecnt = 3 - strlen(temp);
                for(k=0;k<spacecnt;k++)
                    strcat(errstr," ");
                strcat(errstr,"hours ");
            } else {
                for(k=0;k<4 + (int)strlen("hours");k++)
                    strcat(errstr," ");
            }
            if(mins > 0.0) {
                sprintf(temp,"%.0lf",mins);
                strcat(errstr,temp);
                spacecnt = 3 - strlen(temp);
                for(k=0;k<spacecnt;k++)
                    strcat(errstr," ");
                strcat(errstr,"mins ");
            } else {
                for(k=0;k<4 + (int)strlen("mins");k++)
                    strcat(errstr," ");
            }
            sprintf(temp,"%.3lf",secs);
            strcat(errstr,temp);
            spacecnt = 7 - strlen(temp);
            for(k=0;k<spacecnt;k++)
                strcat(errstr," ");
            strcat(errstr,"secs\n");
        }
        splice_multiline_string(errstr,"INFO:");
        fflush(stdout);
        kk++;
    }
    return FINISHED;
}

/************************************ COPY_TEXTFILE ********************************/

int copy_textfile(dataptr dz)
{
    char temp[200];
    int k = 0, n, m, thiswordcnt;
    for(n=0;n<dz->linecnt;n++) {
        thiswordcnt = dz->wordcnt[n];
        for(m=0;m < thiswordcnt;m++) {
            if(m == 0) {
                //sprintf(temp,dz->wordstor[k++]); /* RWD 07 2010: bad use of sprintf */
                strcpy(temp,dz->wordstor[k++]);
            } else {
                strcat(temp," ");
                strcat(temp,dz->wordstor[k++]);
            }
        }
        strcat(temp,"\n");
        if(fputs(temp,dz->fp)<0) {
            sprintf(errstr,"Failed to write line %d to outfile.\n",n);
            return(SYSTEM_ERROR);
        }
    }
    return(FINISHED);
}
