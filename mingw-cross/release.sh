#!/bin/bash -x

test -z "${MINGW_TARGET}" && exit 1
test -z "${MINGW_ARCH}" && exit 2

echo "MINGW_TARGET: $MINGW_TARGET"
echo "MINGW_ARCH: $MINGW_ARCH"

function find_dll( ){
	if [ "$1" != "" ]; then
		for i in `/usr/bin/${MINGW_ARCH}-w64-mingw32-objdump -p $1 | grep "DLL Name:" | cut -d\: -f2 | cut -d\  -f2 | sort | uniq`; do
			file=`find /usr/${MINGW_ARCH}-w64-mingw32 2>/dev/null | grep  $i | grep -v .dll.a`
			echo $file
			find_dll $file
		done
	fi
}

cp `find_dll /tmp/quake2world-${MINGW_TARGET}/bin/quake2world.exe | sort | uniq | grep "\n"` /tmp/quake2world-${MINGW_TARGET}/bin

find /tmp/quake2world-${MINGW_TARGET} -name "*.la" -delete
find /tmp/quake2world-${MINGW_TARGET} -name "*.dll.a" -delete

cp -r ${MINGW_ARCH}/bin ${MINGW_ARCH}/*.bat /tmp/quake2world-${MINGW_TARGET}

cd /tmp/quake2world-${MINGW_TARGET}
rsync -avzP *.bat bin lib maci@quake2world.net:/opt/rsync/quake2world-win32/${MINGW_ARCH}

mkdir quake2world
mv *.bat bin lib quake2world
zip -9 -r quake2world-BETA-${MINGW_ARCH}.zip quake2world
rsync -avzP quake2world-BETA-${MINGW_ARCH}.zip maci@quake2world.net:/var/www/quake2world/files/quake2world-BETA-${MINGW_ARCH}.zip
