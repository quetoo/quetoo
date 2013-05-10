#!/bin/bash
set -e
source ./_common.sh

setup_fedora17

/usr/bin/mock -r ${ENV} --shell "

mkdir -p ~/.quake2world/default/maps
touch ~/.quake2world/default/maps/torn.bsp
echo -e '// generated by Quake2World, do not modify\n' > ~/.quake2world/default/quake2world.cfg

cd /tmp/quake2world
libtoolize --force
autoreconf -i --force
./configure --prefix=/tmp/quake2world-'${ENV}' --with-tests --with-master --without-client
make
make check
make install
" 

archive_workspace

destroy_fedora17
