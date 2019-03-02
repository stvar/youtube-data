#!/bin/bash

# Copyright (C) 2018, 2019  Stefan Vargyas
# 
# This file is part of Cache-DB.
# 
# Cache-DB is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# Cache-DB is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Cache-DB.  If not, see <http://www.gnu.org/licenses/>.

libgdbm-version()
{
    local e=''
    [[ "$1" == '-32' || \
       "$1" == '-64' ]] && {
        e="${1:1}"
        shift
    }

    "${2:-gcc}" -Xlinker --verbose ${1:+ -L"$1"} -lgdbm ${e:+-m$e} 2>/dev/null|
    sed -nr '
        /^-lgdbm\s*\((.*)\)\s*$/!b
        s//\1/
        s/^\s+//
        s/\s+$//
        p
        q'|
    xargs strings|
    sed -nr '
        /^GDBM\s+version\s+([0-9]+\.[0-9]+(\.[0-9]+)?)\..*$/!b
        s//\1/
        p
        q'|
    awk -F. '{
        printf("%d%02d%02d\n", $1, $2, $3)
    }'
}


