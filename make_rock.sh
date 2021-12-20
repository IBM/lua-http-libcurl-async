#!/bin/sh
if [ $# != "1" ]
then
    echo "Usage: $0 <tag-name>"
    exit 1
fi

PACKAGE=lua-async-http
TAG=$1
TARNAME="${PACKAGE}-${TAG}.tar"
TARGETDIR="/tmp"
mkdir -p ${TARGETDIR}
echo "Generating ${TARGETDIR}/${TARNAME}"
git archive $TAG --prefix ${PACKAGE}-${TAG}/ > ${TARGETDIR}/$TARNAME || exit 1
echo "Gizipping the archive"
rm -f ${TARGETDIR}/$TARNAME.gz
gzip -9 ${TARGETDIR}/$TARNAME
echo "Package is ready in ${TARGETDIR}/$TARNAME.gz"
MD5=$(md5sum ${TARGETDIR}/${PACKAGE}-${TAG}.tar.gz | cut -d ' ' -f4)

ROCKSPEC=$(cat << EOF
package = "${PACKAGE}"
version = "${TAG}-1"
source = {
  url = "file://${TARGETDIR}/$TARNAME.gz",
  md5 = "${MD5}"
}
description = {
  summary = "Async HTTP/S client, based on libcurl",
  detailed = [[Asynced http/s client based on libcurl, allowing the usage of certification based messages.]]
}
dependencies = {
  "lua >= 5.1"
}
build = {
  type = "make",
  install_target = "",
  variables = {
  },
  build_variables = {
   DRIVER_LIBS = "",
  },
  install = {
    lib = {
      ["lua_async_http"] = "bin/lua_async_http.so"
    }
  }
}
EOF
)

ROCKSPEC_FILE=${TARGETDIR}/${PACKAGE}-${TAG}-1.rockspec

rm -rf ${ROCKSPEC_FILE}

echo "${ROCKSPEC}" > ${ROCKSPEC_FILE}

chmod 444 ${ROCKSPEC_FILE}

echo "rockspec file is ready in ${ROCKSPEC_FILE}"

luarocks pack ${ROCKSPEC_FILE}

echo "Rock is ready in your current path."