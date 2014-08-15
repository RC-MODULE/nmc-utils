#!/bin/bash 
#

# Try to determine maintainer and email values
if [ -n "$DEBEMAIL" ]; then
       email=$DEBEMAIL
elif [ -n "$EMAIL" ]; then
       email=$EMAIL
else
       email=$(id -nu)@$(hostname -f)
fi
if [ -n "$DEBFULLNAME" ]; then
       name=$DEBFULLNAME
elif [ -n "$NAME" ]; then
       name=$NAME
else
       name="Anonymous"
fi

maintainer="$name <$email>"

control()
{
    cat <<EOF
Package: nmc-utils-${suffix}
Maintainer: ${maintainer}
Version: ${version}
Depends: ${depends}
Architecture: ${arch}
Description: RC Module's Neuromatrix DSP userspace tools: ${suffix}
EOF
}


arch=$1
suffix=$2
version=$3
shift
shift
shift
depends="$*"
control
