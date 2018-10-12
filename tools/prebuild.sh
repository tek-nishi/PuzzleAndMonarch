#!/bin/sh

cd ../assets
../tools/filedz intro.json intro.data
../tools/filedz params.json params.data
mv intro.json ../warehouse
mv params.json ../warehouse
mv p*.ply ../warehouse/Panels
