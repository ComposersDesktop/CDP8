/*
 * Copyright (c) 1983-2020 Richard Dobson and Composers Desktop Project Ltd
 * http://www.rwdobson.com
 * http://www.composersdesktop.com
 * This file is part of the CDP System.
 * The CDP System is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public 
 * License as published by the Free Software Foundation; either 
 * version 2.1 of the License, or (at your option) any later version. 
 *
 * The CDP System is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 * See the GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public
 * License along with the CDP System; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/* devs.c : display list of installed audio devices */  
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
# include <windows.h>
#endif
#include <portaudio.h>

int show_devices(void);

int main(void)
{
    return show_devices();
}

int show_devices(void)
{
        PaDeviceIndex numDevices,p;
        const    PaDeviceInfo *pdi;
        PaError  err;
        int nOutputDevices = 0;

        Pa_Initialize();
        numDevices =  Pa_GetDeviceCount();
        if( numDevices < 0 )
        {
            printf("ERROR: Pa_CountDevices returned 0x%x\n", numDevices );
            err = numDevices;
            return err;
        }
        //printf("Number of devices = %d\n", numDevices );
        printf("Device\tInput\tOutput\tName\n");
        
        for( p=0; p<numDevices; p++ )
        {
            pdi = Pa_GetDeviceInfo( p );    
            nOutputDevices++;
            if( p == Pa_GetDefaultOutputDevice() ) 
                printf("*");
            else
                printf(" ");
            printf("%d\t%d\t%d\t%s\n",p,
                pdi->maxInputChannels,
                pdi->maxOutputChannels,
                pdi->name);         
        }
        Pa_Terminate();
        return 0;
}
