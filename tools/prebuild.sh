#!/bin/sh

cd ../assets
../tools/filedz intro.json intro.data
mv intro.json ../warehouse
mv p*.ply ../warehouse/Panels
