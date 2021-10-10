#!/bin/sh
#
# Copyright (c) 2002-2009
#         The Xfce development team. All rights reserved.
#
# Written for Xfce by Benedikt Meurer <benny@xfce.org>.

export XDT_AUTOGEN_REQUIRED_VERSION="4.14.0"

(type xdt-autogen) >/dev/null 2>&1 || {
  cat >&2 <<EOF
autogen.sh: You don't seem to have the Xfce development tools installed on
            your system, which are required to build this software.
            Please install the xfce4-dev-tools package first, available from
            https://xfce.org/.
EOF
  exit 1
}

test -d m4 || mkdir m4

xdt-autogen $@
