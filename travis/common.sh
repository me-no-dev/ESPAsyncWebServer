#!/bin/bash

function build_sketches()
{
    #set +e
    local arduino=$1
    local srcpath=$2
    local build_arg=$3
    local build_dir=build.tmp
    mkdir -p $build_dir
    local build_cmd="python travis/build.py -b esp32 -k -p $PWD/$build_dir $build_arg "
    local sketches=$(find $srcpath -name *.ino)
    export ARDUINO_IDE_PATH=$arduino
    for sketch in $sketches; do
        rm -rf $build_dir/*
        local sketchdir=$(dirname $sketch)
        local sketchdirname=$(basename $sketchdir)
        local sketchname=$(basename $sketch)
        if [[ "${sketchdirname}.ino" != "$sketchname" ]]; then
            echo "Skipping $sketch, beacause it is not the main sketch file";
            continue
        fi;
        if [[ -f "$sketchdir/.test.skip" ]]; then
            echo -e "\n ------------ Skipping $sketch ------------ \n";
            continue
        fi
        echo -e "\n ------------ Building $sketch ------------ \n";
        time ($build_cmd $sketch >$build_dir/build.log)
        local result=$?
        if [ $result -ne 0 ]; then
            echo "Build failed ($1)"
            echo "Build log:"
            cat $build_dir/build.log
            return $result
        fi
        rm $build_dir/build.log
    done
    #set -e
}
