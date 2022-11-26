#!/bin/bash

lowercase(){
    echo "$1" | sed "y/ABCDEFGHIJKLMNOPQRSTUVWXYZ/abcdefghijklmnopqrstuvwxyz/"
}

OS=`lowercase \`uname\``

if  [ "$OS" == "darwin" ]; then
    MAKE=Makefile.osx

elif  [ "$OS" == "linux" ]; then
    MAKE=Makefile.linux
else
    MAKE=Makefile.mingw
fi

PABUILD="yes"

if [ ! -e ./externals/paprogs/portaudio/lib/.libs/libportaudio.a ]; then
    echo WARNING: portaudio library libportaudio.a not found.
    echo The play and record programs will not be built.
    PABUILD="no"
else
    echo Play and record programs will be built.
fi

targets=(sfsys cdp2k blur cdparams cdparams_other cdparse combine distort editsf env extend filter focus formants \
grain hfperm hilite houskeep misc modify morph new pagrab paview pitch pitchinfo pv pview repitch  \
sfutils sndinfo spec specinfo standalone strange stretch submix synth tabedit texture)

for target in ${targets[@]}
do
    cd ${target}
    echo in folder ${target}
    if [ -e $MAKE ]; then 
        make install -f $MAKE; 
    fi
    cd ..
done

cd externals
# NB portaudio must have been built ("install" not needed) in order to build the record and play programs  
pwd
cd portsf; pwd; make install -f $MAKE; cd ..
cd fastconv; pwd; make install -f $MAKE; cd ..
cd reverb; pwd; make install -f $MAKE; cd ..
if [ "$PABUILD" == "yes" ]; then
    cd paprogs; pwd;
    cd listaudevs; pwd; make install  -f $MAKE; cd ..
    cd paplay; pwd; make install -f $MAKE; cd ..
    cd pvplay; pwd; make install -f $MAKE; cd ..
    cd recsf; pwd; make install -f $MAKE; cd ..
    cd ..
fi

cd mctools; pwd; make install -f $MAKE; cd ..



