#!/bin/bash

var=1
find . 'chat.pb.cc' > /dev/null 2>/dev/null
if [ "$?" != "0" ]; then
    var=0
fi
find . 'chat.pb.h' > /dev/null 2>/dev/null
if [ "$?" != "0" ]; then
    var=0
fi

if [ "$var" = "0" ]; then
    protoc --cpp_out=.  chat.proto
fi

i=0

mkdir -p ./build    
cd build

make -j7
if [ "$?" != "0" ]; then
    cmake ..
    make -j7
fi

cp C S fS P ..

cd ..
cp config.json ./build
