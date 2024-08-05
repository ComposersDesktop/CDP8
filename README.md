# CDP System Software, Release 8.
### Full release as of 24 October 2023


This is a fork to make the commandline utils work for macOS arm64 and raspberry pi
requires portaudio v19.7.0


#### Copyright (c) 2022 Composers Desktop Project Ltd

![The CDP logo]( http://composersdesktop.com/logo.gif) 

	The CDP System is free software; you can redistribute them and/or modify them  
	under the  terms of the GNU Lesser General Public License as published by   
	the Free Software Foundation; either version 2.1 of the License,   
	or (at your option) any later version.

	The CDP System is distributed in the hope that it will be useful, but WITHOUT  
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
	FITNESS FOR A PARTICULAR PURPOSE.  
	See the GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License  
	along with this software (see the top-level file COPYING); if not, write to  
	the Free Software  Foundation, Inc., 51 Franklin St, Fifth Floor,  
	Boston, MA 02110-1301 USA
	

What's New?

* approximately 80 new programs/processes by Trevor Wishart. See details in the [docs](./docs) folder.
*  majority already available for use in Soundloom 17.0.4.
*  includes new programs for multichannel (<= 8 channels) production, waveset distortion, speech/voice processing.
*  ***PVOCEX*** (.pvx) analysis file support for all pvoc-related programs. This is the standard phase vocoder analysis file format used in Csound. See also the downloadable utilities (Mac, Win32), with example files, in the companion PVXTOOLS distribution:
*  play program **pvplay** will play  mono/stereo .pvx files.
*  classic CDP directory utility **dirsf** now recognises .pvx files with format details.
*  See also Tabula Vigilans (TV): some bugs fixed, full MIDI device I/O now working on Linux.

What's needed?

* Developers to get involved (whether CDP8 programs or TV), especially to add new software, create new user interfaces, create libraries, and so many other things we have not thought of. Join the new **cdp-dev** mailing list (see *building.txt* for details); get immediate news of any updates pushed to github.


[updated 28/12/2023]