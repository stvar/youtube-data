#!/bin/bash

# Copyright (C) 2018, 2019  Stefan Vargyas
# 
# This file is part of Youtube-Data.
# 
# Youtube-Data is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# Youtube-Data is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Youtube-Data.  If not, see <http://www.gnu.org/licenses/>.

shopt -s extglob

[ -z "$YOUTUBE_DATA_HOME" ] &&
export YOUTUBE_DATA_HOME="$HOME/youtube-data"

[ -z "$YOUTUBE_DATA_AGENT" ] &&
export YOUTUBE_DATA_AGENT='Youtube-Data (https://github.com/stvar/youtube-data)'

[ -z "$YOUTUBE_DATA_APP_KEY" ] &&
export YOUTUBE_DATA_APP_KEY='???'

[ -z "$YOUTUBE_DATA_SHORTCUTS" ] &&
export YOUTUBE_DATA_SHORTCUTS="$YOUTUBE_DATA_HOME/.shortcuts"

[ -z "$YOUTUBE_DATA_CACHE_THRESHOLD" ] &&
export YOUTUBE_DATA_CACHE_THRESHOLD='16h'

YOUTUBE_WGET_TEE_EXIT_NOPIPE=''
[ "$(tee --help|
        grep --color=none -owe 'exit-nopipe'|sort -u)" == 'exit-nopipe' ] && {
    YOUTUBE_WGET_TEE_EXIT_NOPIPE="$(
        tee --help|
            grep --color=none -owe '--(write|output)-error' -E|sort -u)"
    [[ "$YOUTUBE_WGET_TEE_EXIT_NOPIPE" == --@(write|output)-error ]] &&
    YOUTUBE_WGET_TEE_EXIT_NOPIPE+='=exit-nopipe' ||
    YOUTUBE_WGET_TEE_EXIT_NOPIPE=''
}
export YOUTUBE_WGET_TEE_EXIT_NOPIPE

usage()
{
    echo "usage: $1 [$(sed 's/^://;s/-:$/\x0/;s/[^:]/|-\0/g;s/:/ <arg>/g;s/^|//;s/\x0/-<long>/' <<< "$2")]"
}

quote()
{
    local __n__
    local __v__

    [ -z "$1" -o "$1" == "__n__" -o "$1" == "__v__" ] &&
    return 1

    printf -v __n__ '%q' "$1"
    eval __v__="\"\$$__n__\""
    #!!! echo "!!! 0 __v__='$__v__'"
    test -z "$__v__" && return 0
    printf -v __v__ '%q' "$__v__"
    #!!! echo "!!! 1 __v__='$__v__'"
    printf -v __v__ '%q' "$__v__"  # double quote
    #!!! echo "!!! 2 __v__='$__v__'"
    test -z "$SHELL_BASH_QUOTE_TILDE" &&
    __v__="${__v__//\~/\\~}"
    eval "$__n__=$__v__"
}

quote2()
{
    local __n__
    local __v__

    local __q__='q'
    [ "$1" == '-i' ] && {
        __q__=''
        shift
    }

    [ -z "$1" -o "$1" == "__n__" -o "$1" == "__v__" -o "$1" == "__q__" ] &&
    return 1

    printf -v __n__ '%q' "$1"
    eval __v__="\"\$$__n__\""
    __v__="$(sed -nr '
        H
        $!b
        g
        s/^\n//
        s/\x27/\0\\\0\0/g'${__q__:+'
        s/^/\x27/
        s/$/\x27/'}'
        p
    ' <<< "$__v__")"
    test -n "$__v__" &&
    printf -v __v__ '%q' "$__v__"
    eval "$__n__=$__v__"
}

assign()
{
    local __n__
    local __v__

    [ -z "$1" -o "$1" == "__n__" -o "$1" == "__v__" ] && return 1
    printf -v __n__ '%q' "$1"
    test -n "$2" &&
    printf -v __v__ '%q' "$2"
    test -z "$SHELL_BASH_QUOTE_TILDE" &&
    __v__="${__v__//\~/\\~}"
    eval "$__n__=$__v__"
}

assign2()
{
    local __n__
    local __v__

    [ -z "$1" -o "$1" == "__n__" -o "$1" == "__v__" ] && return 1
    [ -z "$2" -o "$2" == "__n__" -o "$2" == "__v__" ] && return 1
    printf -v __n__ '%q' "$2"
    eval __v__="\"\$$__n__\""
    test -n "$__v__" &&
    printf -v __v__ '%q' "$__v__"
    printf -v __n__ '%q' "$1"
    test -z "$SHELL_BASH_QUOTE_TILDE" &&
    __v__="${__v__//\~/\\~}"
    eval "$__n__=$__v__"
}

optopt()
{
    local __n__="${1:-$opt}"       #!!!NONLOCAL
    local __v__=''
    test -n "$__n__" &&
    printf -v __v__ '%q' "$__n__"  # paranoia
    test -z "$SHELL_BASH_QUOTE_TILDE" &&
    __v__="${__v__//\~/\\~}"
    eval "$__n__=$__v__"
}

optarg()
{
    local __n__="${1:-$opt}"       #!!!NONLOCAL
    local __v__=''
    test -n "$OPTARG" &&
    printf -v __v__ '%q' "$OPTARG" #!!!NONLOCAL
    test -z "$SHELL_BASH_QUOTE_TILDE" &&
    __v__="${__v__//\~/\\~}"
    eval "$__n__=$__v__"
}

optact()
{
    local __v__="${1:-$opt}"       #!!!NONLOCAL
    printf -v __v__ '%q' "$__v__"  # paranoia
    test -z "$SHELL_BASH_QUOTE_TILDE" &&
    __v__="${__v__//\~/\\~}"
    eval "act=$__v__"
}

optactarg()
{
    optact ${1:+"$1"}
    local __v__=''
    test -n "$OPTARG" &&
    printf -v __v__ '%q' "$OPTARG" #!!!NONLOCAL
    test -z "$SHELL_BASH_QUOTE_TILDE" &&
    __v__="${__v__//\~/\\~}"
    eval "arg=$__v__"
}

optlong()
{
    local a="$1"

    if [ "$a" == '-' ]; then
        if [ -z "$OPT" ]; then                                      #!!!NONLOCAL
            local A="${OPTARG%%=*}"                                 #!!!NONLOCAL
            OPT="-$opt$A"                                           #!!!NONLOCAL
            OPTN="${OPTARG:$((${#A})):1}"                           #!!!NONLOCAL
            OPTARG="${OPTARG:$((${#A} + 1))}"                       #!!!NONLOCAL
        else
            OPT="--$OPT"                                            #!!!NONLOCAL
        fi
    elif [ "$opt" == '-' -o \( -n "$a" -a -z "$OPT" \) ]; then      #!!!NONLOCAL
        OPT="${OPTARG%%=*}"                                         #!!!NONLOCAL
        OPTN="${OPTARG:$((${#OPT})):1}"                             #!!!NONLOCAL
        OPTARG="${OPTARG:$((${#OPT} + 1))}"                         #!!!NONLOCAL
        [ -n "$a" ] && OPT="$a-$OPT"                                #!!!NONLOCAL
    elif [ -z "$a" ]; then                                          #!!!NONLOCAL
        OPT=''                                                      #!!!NONLOCAL
        OPTN=''                                                     #!!!NONLOCAL
    fi
}

optlongchkarg()
{
    test -z "$OPT" &&                               #!!!NONLOCAL
    return 0

    [[ "$opt" == [a-zA-Z] ]] || {                   #!!!NONLOCAL
        error "internal: invalid opt name '$opt'"   #!!!NONLOCAL
        return 1
    }

    local r="^:[^$opt]*$opt(.)"
    [[ "$opts" =~ $r ]]
    local m="$?"

    local s
    if [ "$m" -eq 0 ]; then
        s="${BASH_REMATCH[1]}"
    elif [ "$m" -eq 1 ]; then
        error "internal: opt '$opt' not in '$opts'" #!!!NONLOCAL
        return 1
    elif [ "$m" -eq "2" ]; then
        error "internal: invalid regex: $r"
        return 1
    else
        error "internal: unexpected regex match result: $m: ${BASH_REMATCH[@]}"
        return 1
    fi

    if [ "$s" == ':' ]; then
        test -z "$OPTN" && {                        #!!!NONLOCAL
            error --long -a
            return 1
        }
    else
        test -n "$OPTN" && {                        #!!!NONLOCAL
            error --long -d
            return 1
        }
    fi
    return 0
}

error()
{
    local __self__="$self"     #!!!NONLOCAL
    local __help__="$help"     #!!!NONLOCAL
    local __OPTARG__="$OPTARG" #!!!NONLOCAL
    local __opts__="$opts"     #!!!NONLOCAL
    local __opt__="$opt"       #!!!NONLOCAL
    local __OPT__="$OPT"       #!!!NONLOCAL

    local self="error"

    # actions: \
    #  a:argument for option -$OPTARG not found|
    #  o:when $OPTARG != '?': invalid command line option -$OPTARG, or, \
    #    otherwise, usage|
    #  i:invalid argument '$OPTARG' for option -$opt|
    #  d:option '$OPTARG' does not take arguments|
    #  e:error message|
    #  w:warning message|
    #  u:unexpected option -$opt|
    #  g:when $opt == ':': equivalent with 'a', \
    #    when $opt == '?': equivalent with 'o', \
    #    when $opt is anything else: equivalent with 'u'

    local act="e"
    local A="$__OPTARG__" # $OPTARG
    local h="$__help__"   # $help
    local m=""            # error msg
    local O="$__opts__"   # $opts
    local P="$__opt__"    # $opt
    local L="$__OPT__"    # $OPT
    local S="$__self__"   # $self

    local long=''         # short/long opts (default)

    #!!! echo "!!! A='$A'"
    #!!! echo "!!! O='$O'"
    #!!! echo "!!! P='$P'"
    #!!! echo "!!! L='$L'"
    #!!! echo "!!! S='$S'"

    local opt
    local opts=":aA:degh:iL:m:oO:P:S:uw-:"
    local OPTARG
    local OPTERR=0
    local OPTIND=1
    while getopts "$opts" opt; do
        case "$opt" in
            [adeiouwg])
                act="$opt"
                ;;
            #[])
            #	optopt
            #	;;
            [AhLmOPS])
                optarg
                ;;
            \:)	echo "$self: error: argument for option -$OPTARG not found" >&2
                return 1
                ;;
            \?)	if [ "$OPTARG" != "?" ]; then
                    echo "$self: error: invalid command line option -$OPTARG" >&2
                else
                    echo "$self: $(usage $self $opts)"
                fi
                return 1
                ;;
            -)	case "$OPTARG" in
                    long|long-opts)
                        long='l' ;;
                    short|short-opts)
                        long='' ;;
                    *)	echo "$self: error: invalid command line option --$OPTARG" >&2
                        return 1
                        ;;
                esac
                ;;
            *)	echo "$self: error: unexpected option -$OPTARG" >&2
                return 1
                ;;
        esac
    done
    #!!! echo "!!! A='$A'"
    #!!! echo "!!! O='$O'"
    #!!! echo "!!! P='$P'"
    #!!! echo "!!! L='$L'"
    #!!! echo "!!! S='$S'"
    shift $((OPTIND - 1))
    test -n "$1" && m="$1"
    local f="2"
    if [ "$act" == "g" ]; then
        if [ "$P" == ":" ]; then
            act="a"
        elif [ "$P" == "?" ]; then
            act="o"
        else 
            act="u"
        fi
    fi
    local o=''
    if [ -n "$long" -a -n "$L" ]; then
        test "${L:0:1}" != '-' && o+='--'
        o+="$L"
    elif [[ "$act" == [aod] ]]; then
        o="-$A"
    elif [[ "$act" == [iu] ]]; then
        o="-$P"
    fi
    case "$act" in
        a)	m="argument for option $o not found"
            ;;
        o)	if [ "$A" != "?" ]; then
                m="invalid command line option $o"
            else
                act="h"
                m="$(usage $S $O)"
                f="1"
            fi
            ;;
        i)	m="invalid argument for $o: '$A'"
            ;;
        u)	m="unexpected option $o"
            ;;
        d)	m="option $o does not take arguments"
            ;;
        *)	# [ew]
            if [ "$#" -ge "2" ]; then
                S="$1"
                m="$2"
            elif [ "$#" -ge "1" ]; then
                m="$1"
            fi
            ;;
    esac
    if [ "$act" == "w" ]; then
        m="warning${m:+: $m}"
    elif [ "$act" != "h" ]; then
        m="error${m:+: $m}"
    fi
    if [ -z "$S" -o "$S" == "-" ]; then
        printf "%s\n" "$m" >&$f
    else
        printf "%s: %s\n" "$S" "$m" >&$f
    fi
    if [ "$act" == "h" ]; then
        test -n "$1" && h="$1"
        test -n "$h" &&
        printf "%s\n" "$h" >&$f
    fi
    return $f
}

youtube-wget()
{
    local self="youtube-wget"
    local opt0='debug quiet no-check-certif'
    local opt1='header connect-timeout output-document user-agent'
    local optx="@(${opt0// /|}|${opt1// /|})"
    local opx0="@(${opt0// /|})"
    local opx1="@(${opt1// /|})"
    local opta='?(=+(?))'
    local actl='file cache content'
    local actx="@(${actl// /|})"
    local hrex='[0-9a-f]{8}'
    local nrex='[0-9]+'
    local urex="^[-+!]$hrex(:$hrex)?$"
    local lrex="^$nrex"$'\t'"($hrex)("$'\n'"$nrex"$'\t'"($hrex))?$"

    #
    # simple wget options
    #
    local wget_debug=''
    local wget_quiet=''
    local wget_no_check_certif=''

    #
    # wget options with arguments
    #
    local wget_header=''
    local wget_connect_timeout=''
    local wget_output_document=''
    local wget_user_agent=''

    #
    # wget option argument regexes
    #
    local rex_header=''
    local rex_connect_timeout='^[0-9]+$'
    local rex_output_document='^[^[:space:]]+$'
    local rex_user_agent=''

    local x="eval"
    local act="O"       # actions: \
                        #   H: produce cache file name -- 'wget' may be implied (--action=hash)|
                        #   I: produce cache age and file name -- no 'wget' implied (--action=info)|
                        #   O: produce content -- 'wget' may be implied (default) (--action=content)
    local a=""          # pass `--hash-age[=NUM]' to 'cache' when action is `-I' or otherwise do not (default not) (--hash-age[=NUM]|--no-hash-age)
    local c=""          # use only locally cached files when action is not `-I' or otherwise do not (default not) (--[no-]cache-only)
    local e=""          # pass `--save-key' to 'cache' when action is not `-I' or otherwise do not (default not) (--[no-]save-key)
    local h="+"         # home dir (default: '+', i.e. $YOUTUBE_DATA_HOME) (--home=DIR)
    local g=""          # do not pass `-lvv' to 'cache' or otherwise do (default do) (--[no-]log-cache)
    local k=""          # pass `--keep-prev' to 'cache' when action is not `-I' or otherwise do not (default not) (--[no-]keep-prev)
    local l=""          # do not pass `--append-output=$h/.cache/wget.log' to 'wget' or otherwise do (default do) (--[no-]log-wget)
    local t="+"         # cache threshold value ('inf' or of form '[0-9]+[dhms]?'; default: '16h') (--[cache-]threshold=VAL)
    local w=""          # options to be passed as such to 'wget' (--debug|--quiet|--no-check-certif|--connect-timeout=NUM|--header=STR|--output-document=FILE|--user-agent=STR)
    local u=""          # input URL (--url=STR)

    local n
    local v

    local opt
    local OPT
    local OPTN
    local opts=":a:cdegh:HIklOt:u:w:x-:"
    local OPTARG
    local OPTERR=0
    local OPTIND=1
    while getopts "$opts" opt; do
        # discriminate long options
        optlong

        # translate long options to short ones
        test -n "$OPT" &&
        case "$OPT" in
            action)
                [ -n "$OPTN" ] || {
                    error --long -a
                    return 1
                }
                case "$OPTARG" in
                    hash)
                        opt='H' ;;
                    info)
                        opt='I' ;;
                    content)
                        opt='O' ;;
                    *)	error --long -i
                        return 1
                        ;;
                esac
                ;;
            hash-age|no-hash-age)
                opt='a' ;;
            cache-only|no-cache-only)
                opt='c' ;;
            save-key|no-save-key)
                opt='e' ;;
            home)
                opt='h' ;;
            log-cache|no-log-cache)
                opt='g' ;;
            keep-prev|no-keep-prev)
                opt='k' ;;
            log-wget|no-log-wget)
                opt='l' ;;
            threshold|cache-threshold)
                opt='t' ;;
            url)
                opt='u' ;;
            $optx)
                opt='w' ;;
            *)	error --long -o
                return 1
                ;;
        esac

        # check long option argument
        [[ "$opt" == [aHIOw] ]] ||
        optlongchkarg ||
        return 1

        # handle short options
        case "$opt" in
            d)	x="echo"
                ;;
            x)	x="eval"
                ;;
            [HIO])
                optact
                ;;
            [r])
                optopt
                ;;
            [hu])
                optarg
                ;;
            [cegkl])
                if [ "${OPT:0:2}" != 'no' ]; then
                    eval $opt='+'
                else
                    eval $opt=''
                fi
                ;;
            a)	if [ "${OPT:0:2}" == 'no' ]; then
                    [ -z "$OPTN" ] || {
                        error --long -d
                        return 1
                    }
                elif [ -n "$OPT" -a -z "$OPTN" ]; then
                    OPTARG='+'
                elif [[ "$OPTARG" != +([0-9]) ]]; then
                    error --long -i
                    return 1
                fi
                optarg
                ;;
            t)	[[ "$OPTARG" == @([+]|inf) ]] ||
                [[ "$OPTARG" =~ ^[0-9]+[dhms]?$ ]] || {
                    error --long -i
                    return 1
                }
                optarg
                ;;
            w)	#!!! echo >&2 "!!! OPT='$OPT' OPTN='$OPTN' OPTARG='$OPTARG'"

                [[ -n "$OPT" || \
                    "$OPTARG" == $opx0 || \
                    "$OPTARG" == $opx1$opta ]] || {
                    error -i
                    return 1
                }
                optlong -

                #!!! echo >&2 "!!! OPT='$OPT' OPTN='$OPTN' OPTARG='$OPTARG'"

                case "${OPT:2}" in
                    $opx0)
                        [ -z "$OPTN" ] || {
                            error --long -d
                            return 1
                        }
                        n="${OPT:2}"
                        assign "wget_${n//-/_}" '+'
                        ;;
                    $opx1)
                        [ -n "$OPTN" ] || {
                            error --long -a
                            return 1
                        }
                        n="${OPT:2}"
                        assign2 v "rex_${n//-/_}"
                        [[ -z "$v" || "$OPTARG" =~ $v ]] || {
                            error --long -i
                            return 1
                        }
                        assign "wget_${n//-/_}" "$OPTARG"
                        ;;
                    *)	error "internal: unexpected OPT='$OPT'"
                        return 1
                        ;;
                esac
                ;;
            *)	error --long -g
                return 1
                ;;
        esac
    done
    shift $((OPTIND - 1))

    [ -n "$1" ] && u="$1"
    [ -z "$u" ] && {
        error "input url cannot be null"
        return 1
    }

    [ "$h" == '+' ] && h="$YOUTUBE_DATA_HOME"
    [ "$h" == '.' ] && h=''

    [ -n "$h" ] && {
        h="${h%%+(/)}"
        [ -z "$h" ] &&
        h='/'
    }
    [ -n "$h" -a ! -d "$h" ] && {
        error "home dir '$h' not found"
        return 1
    }
    [[ -z "$h" || "$h" =~ /$ ]] || h+='/'

    [ ! -d "$h.cache" ] && {
        error "cache directory '$h.cache' not found"
        return 1
    }

    local C="${h}cache"
    [ -x "$C" ] || {
        error "executable '$C' not found"
        return 1
    }

    quote C
    quote h

    [ "$t" == '+' ] && t='16h'

    local k2="$u"
    local k3="$k2"
    quote2 -i k3
    quote2 -i u

    # stev: reverse $g
    [ -z "$g" ] && g='+' || g=''

    # stev: reverse $l
    [ -z "$l" ] && l='+' || l=''

    # stev: $n is only a shortcut for $YOUTUBE_WGET_TEE_EXIT_NOPIPE
    local n="$YOUTUBE_WGET_TEE_EXIT_NOPIPE"
    [[ -z "$n" || "$n" == --@(write|output)-error=exit-nopipe ]] || {
        error "invalid \$YOUTUBE_WGET_TEE_EXIT_NOPIPE='$n'"
        return 1
    }

    local c0="$C -h $h.cache"
    local c2="$c0"

    local s
    if [[ "$act" == 'I' ]]; then
        if [ "$a" == '+' ]; then
            a='hash-age'
        elif [ -n "$a" ]; then
            a="hash-age=$a"
        fi
        c2+="${g:+ -l}${a:+ --$a} -L '$k3'"

        $x "$c2"
        return $?
    else
        [ -z "$c" ] &&
        c2+=" -t $t${g:+ -lvv}${e:+ --dry-run} --prev-key -U '$k3'" ||
        c2+="${g:+ -l} --no-error --prev-key -L '$k3'"

        if [ "$x" == "eval" -o -n "$c" ]; then
            s="$(eval "$c2")" || {
                [ -n "$c" ] &&
                error "inner command failed: $c2"
                return 1
            }
        else
            echo "$c2"
            s='-ffffffff:fffffffe'
        fi
    fi

    # stev: from now on [[ $act == [HO] ]]

    [[ ( -z "$c" && "$s" =~ $urex ) || \
       ( -n "$c" && "$s" =~ $lrex ) ]] || {
        local s2="$(sed -nr 'H;$!b;g;s/^\n//;l1024' <<< "$s"|
                    sed -nr 's/\$$//;H;$!b;g;s/\n//g;p')"
        error "cache returned unexpected result '${s2:-$s}' on key '$k2'"
        return 1
    }

    # stev: $O is only a shortcut for $wget_output_document
    local O="${wget_output_document:--}"
    quote O

    [ -n "$c" ] && {
        local h0="${BASH_REMATCH[1]}"
        local h1="${BASH_REMATCH[3]}"

        # stev: no need to quote $h0 and $h1 below

        [ "$act" == 'H' ] && c2="\
printf -- '+$h0${h1:+:$h1}\n'"
        [ "$act" == 'O' ] && c2="\
cat $h.cache/$h0"
        [ "$act" == 'O' -a "$O" != '-' ] && c2+=" > \\
$O"

        $x "$c2"
        return $?
    }

    # stev: from now on [ -z "$c" ]

    local p="${s:0:1}"
    [ "$p" != '!' ] || {
        error "cache returned error '$s' on key '$k2'"
        return 1
    }

    local r=''
    [[ "$t" =~ ^0+[dhms]?$ ]] && r='+'

    # stev: do to quote $o: $h is already quoted
    local o="$h.cache/${s:1:8}"

    local o2
    [ -n "$e" ] && o2="${s:1:8}" || o2="$o"

    c2=''
    [[ -n "$r" || "$p" == '-' ]] && {
        [ -n "$e" ] &&
        c0+=" -t $t${g:+ -lvv} -f $o2.1${k:+ --keep-prev} --save-key -U '$k3'" ||
        c0=''

        [ -n "$l" ] && {
            l="wget.log"
            # stev: do to quote $l: $h is already quoted
            [ -z "$e" ] && l="$h.cache/$l"
        }

        local q
        if [ -z "$wget_quiet" ]; then
            [ -n "$l" ] &&
            q="no-verbose" ||
            q="verbose"
        else
            q="quiet"
        fi

        quote2 -i wget_header
        quote2 -i wget_user_agent
        quote wget_output_document

        # stev: no need to quote $wget_debug
        # stev: no need to quote $wget_no_check_certif
        # stev: no need to quote $wget_connect_timeout

        [ -n "$e" ] && c2+="\
(
cd $h.cache &&"$'\n'
        c2+=${c2:+$'\n'}"\
wget \\
--$q \\${wget_debug:+
--debug \\}${wget_no_check_certif:+
--no-check-certificate \\}${wget_user_agent:+
--user-agent='$wget_user_agent' \\}${wget_header:+
--header='$wget_header' \\}${wget_connect_timeout:+
--connect-timeout=$wget_connect_timeout \\}"
        [ ! \( "$act" == 'O' -a "$O" == '-' -a -n "$n" \) ] && c2+="
--output-document=$o2${e:+.1} \\"
        [   \( "$act" == 'O' -a "$O" == '-' -a -n "$n" \) ] && c2+="
--output-document=- \\"
        c2+="${l:+
--append-output=$l \\}
'$u'"
        # stev: when $act is 'H', pass on the output '^[-+!]$hrex(:$hrex)$' of $c0
        [ -n "$e" ] && c2+=" &&

$c0"
        # stev: when $act is 'O', ignore the output of $c0
        [ "$act" == 'O' -a -n "$e" ] && c2+=' >/dev/null'
        [ -n "$e" ] && c2+="

e=\$?
rm -f $o2.1
exit \$e
"")"
    }
    # stev: when $act is 'H' and $c2 doesn't include $c0, print out '^[-+!]$hrex(:$hrex)$' from $s
    [[ "$act" == 'H' && ! ( ( -n "$r" || "$p" == '-' ) && -n "$e" ) ]] && c2+=${c2:+$' &&\n'}"\
printf -- '$s\n'"
    [[ "$act" == 'O' &&   ( "$O" == '-' && -n "$c2" && -n "$n" ) ]] && c2+="|
tee $n $o"
    [[ "$act" == 'O' && ! ( "$O" == '-' && -n "$c2" && -n "$n" ) ]] && c2+=${c2:+$' &&\n'}"\
cat $o"
    [[ "$act" == 'O' && "$O" != '-' ]] && c2+=" > \\
$O"

    $x "$c2"
}

youtube-data()
{
    local self="youtube-data"
    local tmpf="/tmp/$self.XXX"
    local yurl='https://www.googleapis.com/youtube/v3/'
    local yitm='/items'
    local ypth='snippet'
    local ypar=(
        # [0] channel itself:
        'publishedAt title description'
        # [1] playlist itself:
        'channelId publishedAt title description'
        # [2] video itself:
        'channelId publishedAt duration title description'
        # [3] search channel playlists:
        'playlistId publishedAt title description'
        # [4] search channel videos or playlist items:
        'videoId publishedAt title description'
    )
    local yprl='channel-id playlist-id video-id published-at duration title description'
    local yprx="@(${yprl// /|})"
    local ypex=(
        # [0] video itself:
        'contentDetails/duration'
        # [1] search channel playlists:
        'id/playlistId'
        # [2] search channel videos:
        'id/videoId'
        # [3] playlist items:
        'snippet/resourceId/videoId'
    )
    local yprt=(
        # [0] channel itself:
        'brandingSettings,contentDetails,id,snippet,statistics,status,topicDetails'
        # [1] playlist itself:
        'contentDetails,id,player,snippet,status'
        # [2] video itself:
        'contentDetails,id,liveStreamingDetails,player,recordingDetails,snippet,statistics,status,topicDetails'
        # [3] search channel:
        'id,snippet'
        # [4] playlist items:
        'contentDetails,id,snippet,status'
    )
    local yqry=(
        # [0] itself -- ie. channel, video or playlist:
        '%ss?key=%s&id=%s&part=%s'
        # [1] search channel:
        'search?key=%s&channelId=%s&type=%s&part=%s&order=date'
        # [2] playlist items:
        'playlistItems?key=%s&playlistId=%s&part=%s'
    )
    local ymax='&maxResults=%s'
    local ypgt='&pageToken=%s'
    local prex="$yprx*(,$yprx)"
    local drex='^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}\.[0-9]+Z$'
    local trex='^PT([0-9]+H)?([0-9]{1,2}M)?([0-9]{1,2}S)?$'
    local lrex='^PL([0-9a-zA-Z_-]{32}|[0-9A-F]{16})$|^LL[0-9a-zA-Z_-]{22}$'
    local crex='^UC[0-9a-zA-Z_-]{22}$'
    local vrex='^[0-9a-zA-Z_-]{11}$'
    local hrex='[0-9a-f]{8}'
    local lnkx='@(itself|playlists|videos)'
    local resx='@(channel|playlist|video)'
    local resa='?(=+(?))'
    local prtx='@(list|table)'
    local prta="$resa"
    local intx='@(file|id)'
    local inta="$resa"
    local cacx='@([-+~#=!]|previous|local|current|auto|refresh)'
    local colx='@(always|auto)'
    local recx='@(show|diff|wdiff)-recent'
    local reca="$resa"
    # stev: 2 + max of length of each text parameter names:
    #   "description"
    #   "title"
    local minw=13

    local open="\x1b\x5b\x30\x31"
    local close="\x1b\x5b\x30\x6d"
    local green="$open\x3b\x33\x32\x6d"
    local yellow="$open\x3b\x33\x33\x6d"

    local x="eval"
    local act="P"       # actions: \
                        #   U: produce resource's API URL (--url)|
                        #   I: produce resource's cache age and hash file name (--info)|
                        #   H: produce resource's hash file names (previous and current versions) after eventually updating them according to the specifier given by `--cache=SPEC' (--hash)|
                        #   J: produce resource's JSON text (--json)|
                        #   S: produce resource's flattened JSON text (--json2)|
                        #   D: diff two resources (the other one is given by `--file=FILE' or `--id=ID'); the default FILE is '+', i.e. the one generated by action `-J' (--diff[=FILE])|
                        #   P: extract resource's content as list or table; KEYS are comma-separated lists of JSON key names; `-P?', `--list=?' or `--table=?' prints out the valid names corresponding to the pair of resource type and link type given and exits (default) (--list[=KEYS],--table[=KEYS],--channel-id,--playlist-id,--video-id,--published-at,--description,--duration,--title)|
                        #   V: extract resource's content for the most recent NUM video items; the output produced is specified by the print options `-P', `--list', `--table', etc; `--[w]diff-recent' produce a 'diff' or 'wdiff' between previous version of the resource's cached content and the one specified by `--cache=SPEC'; the options `--relative-date', `--wrap-text' and `--color=auto' are implied if not overridden; NUM is at most the argument of `--max-results' and it is 5 by default (--show-recent[=NUM],--diff-recent[=NUM],--wdiff-recent[=NUM])
    local a="+"         # user agent to be passed to 'wget' when action is `-J' or when input type is 'id' (default: '+', i.e. $YOUTUBE_DATA_AGENT) (--[user-]agent=STR)
    local b="+"         # produce relative dates on output or otherwise do not; when given NUM, compute dates relative to NUM as number of seconds since the Epoch (default: do for `-I' or else do not) (--relative-date[=NUM]|--no-relative-date)
    local c="+"         # use resource's locally cached files according to the given specifier or, otherwise, do not use the cache at all, but query the remote party for content; SPEC is any of 'previous', 'local', 'current', 'auto' (default) or 'refresh'; 'previous' means using resource's previous version locally cached file; 'local' means using resource's locally cached file -- in any case do not call the remote party; 'current' means using resource's current version locally cached file if that file exists already -- if not, get it from the remote party; 'auto' means using resource's locally cached file if and only if the respective file has not yet reached its aging threshold; each aged cache file is refreshed the first time its associated resource is requested; 'refresh' makes each cached file to be refreshed prior to using it further regardless of it not exceeding its aging threshold yet; the aging threshold is specified by $YOUTUBE_DATA_CACHE_THRESHOLD (which is of form '[0-9]+[dhms]?'); the short option `-c' accepts shortcut arguments too: '-', '~', '#', '=', '+' and '!' for `--no-cache', `-c previous', `-c local', `-c current', `-c auto' and `-c refresh' respectively (--cache=SPEC|--no-cache)
    local e="+"         # output mode if output file already exists: 'error', 'append' or 'overwrite' with corresponding shortcuts '-', '+' and '=' (default: '-' i.e. 'error') (--output-mode=MODE)
    local g=""          # pass `--debug' to 'wget' (--debug)
    local h="+"         # home dir (default: '+', i.e. $YOUTUBE_DATA_HOME) (--home=DIR)
    local i="+"         # input type -- 'file' or 'id' -- and input name (the default type is 'id') (--file[=FILE]|--id[=ID])
    local k=""          # header cookie to be passed to 'wget'; when '+' take the cookie from $YOUTUBE_DATA_COOKIE (--[header-]cookie=STR)
    local l="itself"    # linked resource's type: 'itself', 'playlists' or 'videos' (default: 'itself') (--itself|--playlists|--videos)
    local m="+"         # max results per API query (value in [1..50], default: 50) (--[max-]results=NUM)
    local n=""          # colorize the output of `--list', `--diff-recent' and `--wdiff-recent', or otherwise do not (default: do not for `-P' and 'auto' for `-V' and `-D', i.e. color only if stdout is a terminal) (--color=always|auto|--no-color)
    local o="+"         # output file name when action is `-J|--json' (default: '+', i.e. generate a name; '+SUFFIX' means appending '.SUFFIX' to the generated name) (--output-file=STR)
    local p="+"         # resource's pages specifier: '-' means the first page only, '+NUM' means first NUM >= 1 pages only, '+' means all pages and '@TOKEN' means the page identified by TOKEN (default: '+') (--page=SPEC)
    local q=""          # be quiet: when action is `-J|--json' do not print out the output file name (--quiet)
    local r="channel"   # resource's type: 'channel', 'playlist' or 'video' (default: 'channel'); INPUT is either a file name or an id (default), determined by `--file' or `--id'; when INPUT terminates with '.json', consider it to be file name; when INPUT ends with '{,-playlists,-videos}.json', set the linked type accordingly (--channel[=INPUT]|--playlist[=INPUT]|--video[=INPUT])
    local s="+"         # channel and playlist shortcuts file name (default: '+', i.e. $YOUTUBE_DATA_SHORTCUTS) (--shortcuts=FILE)
    local t=""          # do not type-check resource's JSON object, i.e. do not pass either of the options `-t $h/*.json' or `-t $h/youtube-data.so', and respectively, neither `-f -- json-litex.so $h/*-litex.json' nor `-f -- json-litex.so $h/youtube-data-litex.so' to 'json', or otherwise do (default do) (--[no-][json-]type)
    local u=""          # pass `-u NUM' to 'diff' when action is `-D|--diff' or `-V|--diff-recent' (--unified=NUM)
    local v=""          # do not validate resource's parameter values or otherwise do (default do) (--[no-]validate)
    local w=""          # wrap 'title' and 'description' texts when output is of type 'list' or otherwise do not (default: do not for `-P' and do for `-V'; NUM must be >= 13; its default value is 72) (--wrap[-text][=NUM]|--no-wrap[-text])
    local y="+"         # Google API application key (default: '+', i.e. $YOUTUBE_DATA_APP_KEY) (--[app-]key=STR)

    local arg='+=+'     # action argument (default for default action '-P')
    local r2='i'        # last option setting $i: 'i' or 'r'
    local V2=''         # argument of `-V|--{show,diff,wdiff}-recent'
    local V3

    local opt
    local OPT
    local OPTN
    local opts=":a:bc:dD:e:gh:Hi:IJk:l:m:n:o:p:P:qr:s:Stu:UvV:wxy:-:"
    local OPTARG
    local OPTERR=0
    local OPTIND=1
    while getopts "$opts" opt; do
        # discriminate long options
        optlong

        # translate long options to short ones
        test -n "$OPT" &&
        case "$OPT" in
            user-agent|agent)
                opt='a' ;;
            relative-date|no-relative-date)
                opt='b' ;;
            cache|no-cache)
                opt='c' ;;
            diff)
                opt='D' ;;
            output-mode)
                opt='e' ;;
            debug)
                opt='g' ;;
            home)
                opt='h' ;;
            hash)
                opt='H' ;;
            $intx)
                opt='i' ;;
            info)
                opt='I' ;;
            json)
                opt='J' ;;
            cookie|header-cookie)
                opt='k' ;;
            $lnkx)
                opt='l' ;;
            results|max-results)
                opt='m' ;;
            color|no-color)
                opt='n' ;;
            output-file)
                opt='o' ;;
            page)
                opt='p' ;;
            $prtx|$yprx)
                opt='P' ;;
            quiet)
                opt='q' ;;
            $resx)
                opt='r' ;;
            json2)
                opt='S' ;;
            type|no-type|json-type|no-json-type)
                opt='t' ;;
            unified)
                opt='u' ;;
            url)
                opt='U' ;;
            validate|no-validate)
                opt='v' ;;
            $recx)
                opt='V' ;;
            wrap|wrap-text|no-wrap|no-wrap-text)
                opt='w' ;;
            key|app-key)
                opt='y' ;;
            *)	error --long -o
                return 1
                ;;
        esac

        # check long option argument
        [[ "$opt" == [bcDilnPrVw] ]] ||
        optlongchkarg ||
        return 1

        # handle short options
        case "$opt" in
            d)	x="echo"
                ;;
            x)	x="eval"
                ;;
            [HIJNSU])
                optactarg
                ;;
            [gqtv])
                optopt
                ;;
            [ahkoy])
                optarg
                ;;
            D)	[[ -n "$OPT" && -z "$OPTN" && -z "$OPTARG" ]] && {
                    OPTARG='+'
                    OPTN='='
                }
                V3="${V2%%=*}"
                [ "$V3" == 'show-recent' ] && {
                    error -w "preceding action \`--$V3' overrides \`-D|--diff'"
                    continue
                }
                optactarg
                ;;
            P)	[[ -n "$OPT" || \
                    "$OPTARG" == @([?+]|++|$prex|$prtx$prta) ]] || {
                    error -i
                    return 1
                }
                optlong -

                #!!! echo >&2 "!!! OPT='$OPT' OPTN='$OPTN' OPTARG='$OPTARG'"

                [[ -z "$OPTN" || -n "$OPTARG" ]] || {
                    error --long -i
                    return 1
                }
                V3="${V2%%=*}"
                [[ "$V3" == @(diff|wdiff)-recent ]] && {
                    [ "${OPT:0:2}" != '--' ] && OPT="${OPT:0:2}"
                    error -w "preceding action \`--$V3' overrides \`$OPT'"
                    continue
                }
                case "${OPT:2}" in
                    \?)	arg='?'
                        ;;
                    +)	arg='+=+'
                        ;;
                    ++)	arg='+=++'
                        ;;
                    $prex)
                        arg="+=${OPT:2}"
                        ;;
                    $prtx)
                        arg="${OPT:2}=${OPTARG:-+}"
                        ;;
                    *)	error --long -o
                        return 1
                        ;;
                esac
                act='P'
                ;;
            V)	#!!! echo >&2 "!!! OPT='$OPT' OPTN='$OPTN' OPTARG='$OPTARG'"

                [[ -n "$OPT" || "$OPTARG" == $recx$reca ]] || {
                    error -i
                    return 1
                }
                optlong -

                #!!! echo >&2 "!!! OPT='$OPT' OPTN='$OPTN' OPTARG='$OPTARG'"

                [[ -z "$OPTN" || -n "$OPTARG" ]] || {
                    error --long -i
                    return 1
                }
                [[ "${OPT:2}" == $recx ]] || {
                    error --long -o
                    return 1
                }

                [ -n "$OPTARG" ] && {
                    [[ "$OPTARG" == @(\+|+([0-9])) ]] && {
                        [[ "$OPTARG" == '+' ]] ||
                        [[ "$OPTARG" -gt 0 ]]; } || {
                        error --long -i
                        return 1
                    }
                }

                if [ "${OPT:2}" == 'show-recent' ]; then
                    [ "$act" != 'P' ] && {
                        act='P'
                        arg='+'
                    }
                else
                    [ "$act" != 'D' ] && {
                        act='D'
                        arg='+'
                    }
                fi
                V2="${OPT:2}=$OPTARG"
                l='videos'
                b='++'
                n='auto'
                w='+'
                p='-'
                ;;
            b)	if [ "${OPT:0:2}" == 'no' ]; then
                    [ -z "$OPTN" ] || {
                        error --long -d
                        return 1
                    }
                elif [ -z "$OPT" -o -z "$OPTN" ]; then
                    OPTARG='++'
                elif [[ "$OPTARG" != +([0-9]) ]]; then
                    error --long -i
                    return 1
                fi
                optarg
                ;;
            c)	[[ -n "$OPT" || "$OPTARG" == $cacx ]] || {
                    error -i
                    return 1
                }
                optlong -

                #!!! echo >&2 "!!! OPT='$OPT' OPTN='$OPTN' OPTARG='$OPTARG'"

                case "${OPT:2}" in
                    no-cache)
                        [ -z "$OPTN" ] || {
                            error --long -d
                            return 1
                        }
                        c='-'
                        ;;
                    cache)
                        [ -n "$OPTN" ] || {
                            error --long -a
                            return 1
                        }
                        case "$OPTARG" in
                            previous)
                                c='~' ;;
                            local)
                                c='#' ;;
                            current)
                                c='=' ;;
                            auto)
                                c='+' ;;
                            refresh)
                                c='!' ;;
                            *)	error --long -i
                                return 1
                                ;;
                        esac
                        ;;
                    $cacx)
                        case "${OPT:2}" in
                            previous)
                                c='~' ;;
                            local)
                                c='#' ;;
                            current)
                                c='=' ;;
                            auto)
                                c='+' ;;
                            refresh)
                                c='!' ;;
                            *)	c="${OPT:2}"
                                ;;
                        esac
                        ;;
                    '')	[ "$OPTN" == '=' -a -z "$OPTARG" ] || {
                            # stev: unexpected $OPTN and $OPTARG
                            error --long -i
                            return 1
                        }
                        c='='
                        ;;
                    *)	error "internal: unexpected OPT='$OPT'"
                        return 1
                        ;;
                esac
                ;;
            e)	case "$OPTARG" in
                    [-+=])
                        ;;
                    error)
                        OPTARG='-'
                        ;;
                    append)
                        OPTARG='+'
                        ;;
                    overwrite)
                        OPTARG='='
                        ;;
                    *)	error --long -i
                        return 1
                        ;;
                esac
                optarg
                ;;
            i)	[[ -n "$OPT" || "$OPTARG" == $intx$inta ]] || {
                    error -i
                    return 1
                }
                optlong -

                [[ -z "$OPTN" || -n "$OPTARG" ]] || {
                    error --long -i
                    return 1
                }
                [[ "${OPT:2}" == $intx ]] || {
                    error --long -o
                    return 1
                }
                if [ -n "$OPTARG" ]; then
                    i="${OPT:2}=$OPTARG"
                elif [ "$r2" == 'r' ]; then
                    i="${OPT:2}=${i#*=}"
                else
                    i="${OPT:2}"
                fi
                [ "${OPT:2}" != 'id' ] ||
                [[ -z "${i:3}" || "${i:3:1}" == '@' || \
                        "${i:3}" =~ $crex|$lrex|$vrex ]] || {
                    error --long -i
                    return 1
                }
                r2='i'
                ;;
            l)	[[ -n "$OPT" || "$OPTARG" == $lnkx ]] || {
                    error -i
                    return 1
                }
                optlong -

                [ -z "$OPTN" ] || {
                    error --long -d
                    return 1
                }
                [[ "${OPT:2}" == $lnkx ]] || {
                    error --long -o
                    return 1
                }
                l="${OPT:2}"
                ;;
            m)	[[ "$OPTARG" == '+' ]] ||
                [[ "$OPTARG" == +([0-9]) && \
                   "$OPTARG" -le 50 && \
                   "$OPTARG" -gt 0 ]] || {
                    error --long -i
                    return 1
                }
                optarg
                ;;
            n)	if [ "${OPT:0:2}" == 'no' ]; then
                    [ -z "$OPTN" ] || {
                        error --long -d
                        return 1
                    }
                elif [[ "$OPTARG" != $colx ]]; then
                    error --long -i
                    return 1
                fi
                optarg
                ;;
            p)	[[ "$OPTARG" == [-+] ]] ||
                [[ "$OPTARG" == \@+([0-9a-zA-Z_-]) ]] ||
                [[ "$OPTARG" == \++([0-9]) && \
                   "${OPTARG:1}" -gt 0 ]] || {
                    error --long -i
                    return 1
                }
                optarg
                ;;
            r)	[[ -n "$OPT" || "$OPTARG" == $resx$resa ]] || {
                    error -i
                    return 1
                }
                optlong -

                [[ -z "$OPTN" || -n "$OPTARG" ]] || {
                    error --long -i
                    return 1
                }
                [[ "${OPT:2}" == $resx ]] || {
                    error --long -o
                    return 1
                }
                [ -n "$OPTARG" ] && {
                    if [[ "$OPTARG" =~ -(playlists|videos)\.json$ ]]; then
                        # stev: consider link type to be 'playlists'
                        # or 'videos' when the given arg terminates
                        # with '-playlists.json' or '-videos.json'
                        # respectively
                        l="${BASH_REMATCH[1]}"
                    elif [[ "$OPTARG" == *.json ]]; then
                        # stev: otherwise, when the arg ends with
                        # '*.json' only, set link type to 'itself'
                        l='itself'
                    fi
                    if [[ "$OPTARG" == *.json || "$OPTARG" =~ ^#$hrex$ ]]; then
                        # stev: take input type as 'file' when
                        # the given arg terminates with '.json'
                        # or when is a hash file reference
                        r2='file'
                    elif [ "$i" == '+' ]; then
                        r2='id'
                    else
                        r2="${i%%=*}"
                    fi
                    [ "$r2" != 'id' ] ||
                    [ "${OPTARG:0:1}" == '@' ] ||
                    [[ "${OPT:2}" == 'channel' && "$OPTARG" =~ $crex ]] ||
                    [[ "${OPT:2}" == 'playlist' && "$OPTARG" =~ $lrex ]] ||
                    [[ "${OPT:2}" == 'video' && "$OPTARG" =~ $vrex ]] || {
                        error --long -i
                        return 1
                    }
                    i="$r2=$OPTARG"
                    r2='r'
                }
                r="${OPT:2}"
                ;;
            u)	[[ "$OPTARG" == +([0-9]) ]] || {
                    error --long -i
                    return 1
                }
                optarg
                ;;
            [vt])
                if [ "${OPT:0:2}" != 'no' ]; then
                    eval $opt="$opt"
                else
                    eval $opt=''
                fi
                ;;
            w)	if [ "${OPT:0:2}" == 'no' ]; then
                    [ -z "$OPTN" ] || {
                        error --long -d
                        return 1
                    }
                elif [ -z "$OPT" -o -z "$OPTN" ]; then
                    OPTARG='+'
                elif [[ -n "$OPT" && "$OPTARG" != @(\+|+([0-9])) ]]; then
                    error --long -i
                    return 1
                elif [ -z "$OPT" -a "$OPTARG" != '+' ] && [[ "$OPTARG" -lt "$minw" ]]; then
                    error "the argument of \`-w|--text-wrap' must be at least $minw"
                    return 1
                fi
                optarg
                ;;
            *)	error --long -g
                return 1
                ;;
        esac
    done
    shift $((OPTIND - 1))

    # stev: need pipelines to succeed only when
    # each of the component commands succeeded!
    set -o pipefail

    case "$r:$l" in
        @(channel|playlist|video):itself)
            ;;
        channel:@(playlists|videos))
            ;;
        playlist:videos)
            ;;
        *)	error "'$r' resources cannot be linked to '${l%s}' ones"
            return 1
            ;;
    esac

    [ "$i" == '+' ] && i='id'

    local i2="${i#*=}"
    if [ "$i" != "$i2" ]; then
        i="${i%%=*}"
    else
        i2="$1"
        [ "$i" != 'id' ] ||
        [[ -z "$i2" || "${i2:0:1}" == '@' || \
            "$i2" =~ $crex|$lrex|$vrex ]] || {
            error "invalid input arg '$i2'"
            return 1
        }
    fi

    [[ -z "$i2" && ! ( "$act" == 'P' && "${arg#*=}" == '?' ) ]] && {
        error "input arg not given"
        return 1
    }

    [ "$YOUTUBE_DATA_DEBUG_ARG" == 'yes' ] &&
    echo >&2 "!!! r='$r' l='$l' i='$i' i2='$i2' act='$act' arg='$arg' V2='$V2'"

    # stev: [ -z "$i2" ] => [[ "$act" == 'P' && "$arg" == *=? ]]

    [[ "$act" == [HIJU] && "$i" == 'file' ]] && {
        case "$act" in
            H) arg='hash' ;;
            I) arg='info' ;;
            J) arg='json' ;;
            U) arg='url' ;;
        esac
        error "cannot have input type be 'file' when action is \`-$act|--$arg'"
        return 1
    }

    [[ "$act" == 'H' && "$c" == [-~] ]] && {
        local c2
        [ "$c" == '-' ] && c2='no-cache' || c2='cache=previous'
        error "cannot have options \`-c$c|--$c2' when action is \`-H|--hash'"
        return 1
    }

    [ -n "$V2" -a "$l" != 'videos' ] && {
        error "cannot have link type be '$l' when action is \`--${V2%%=*}'"
        return 1
    }

    # stev: note that hash file references are checked
    # for existence down below, upon computing $h
    [[ "$i" == 'file' && -n "$i2" && "$i2" != '-' && \
        ! ( "$i2" =~ ^#$hrex$ ) && ! -f "$i2" ]] && {
        error "input file '$i2' not found"
        test "$x" == 'eval' && return 1
    }

    [ "$i" == 'id' -a "${i2:0:1}" == '@' ] && {
        [ -z "$s" ] && {
            error "shortcuts file cannot be null"
            return 1
        }
        [ "$s" == '+' ] && s="$YOUTUBE_DATA_SHORTCUTS"
        [ -z "$s" ] && {
            error "\$YOUTUBE_DATA_SHORTCUTS is null"
            return 1
        }
        [ ! -f "$s" ] && {
            error "shortcuts file '$s' not found"
            return 1
        }

        [[ "${i2:1}" == +([[:alnum:]-]) ]] || {
            error "invalid shortcut name '$i2'"
            return 1
        }

        local n2
        local i3
        local i4
        i3=($(sed -nr 's/^('"[^\s]*${i2:1}"'[^\s]*)\s+([a-zA-Z90-9_-]+)\s*$/\1\t\2/Ip' "$s"|
              sort -t $'\t' -k2,2 -u)) && [ "$((${#i3[@]} % 2))" -eq 0 ] || {
            error "inner command failed: sed [0]"
            return 1
        }
        [ "${#i3[@]}" -eq 0 ] && {
            error "shortcut '$i2' not found"
            return 1
        }
        [ "${#i3[@]}" -ne 2 ] && {
            i4=''
            for ((n2=0; n2<${#i3[@]}; n2+=2)); do
                [ "${i3[$n2]}" == "${i2:1}" ] && {
                    i3[1]="${i3[$(($n2 + 1))]}"
                    i4=''
                    break
                }
                i4+="${i4:+, }${i3[$n2]}"
            done
            [ -n "$i4" ] && {
                error "multiple shortcuts '$i2' found: $i4"
                return 1
            }
        }

        i3="${i3[1]}"
        [[ "$i3" =~ $crex|$lrex ]] || {
            error "invalid shortcuts entry '$i3'"
            return 1
        }

        if [ "$r2" == 'r' ]; then
            i4=''
            case "${i3:0:2}" in
                UC) [ "$r" == 'channel' ]  || i4='channel' ;;
                PL) [ "$r" == 'playlist' ] || i4='playlist' ;;
            esac
            [ -n "$i4" ] && {
                error "shortcut '$i2' is not a $r but a $i4"
                return 1
            }
        else
            case "${i3:0:2}" in
                UC) r='channel' ;;
                PL) r='playlist' ;;
            esac
        fi
        i2="$i3"
    }

    local c0=''
    local c2=''

    [ "$act" == 'D' -a -z "$V2" ] && {
        [ "$arg" == '+' ] && {
            [ "$i" != 'id' ] && {
                error "argument of \`-D|--diff' cannot be '$arg' when input file type is '$i'"
                return 1
            }
            # stev: be aware of duplicated code
            # computing file name from $i2 and $l
            if [ "$l" != 'itself' ]; then
                arg="$i2-$l.json"
            elif [ "$r" == 'video' ]; then
                arg="VD$i2.json"
            else
                arg="$i2.json"
            fi
        }

        [ ! -f "$arg" ] && {
            error "diff source file '$arg' not found"
            test "$x" == "eval" && return 1
        }
        quote arg
        quote i2
    }

    [ "$act" == 'D' -o "${p:0:1}" == '+' ] && {
        c0="$self"

        local n2
        local v2
        # stev: valued params defaulted with '+'
        for n2 in a:agent h:home y:key m:results; do
            assign2 v2 ${n2:0:1} # stev: using $a, $h, $y and $m
            [ "$v2" == '+' ] &&
            continue
            [ -z "$v2" ] &&
            v2="''" ||
            quote v2
            c0+=" --${n2:2}=$v2"
        done
        # stev: valued params defaulted with ''
        for n2 in k:cookie; do 
            assign2 v2 ${n2:0:1} # stev: using $k
            [ -z "$v2" ] &&
            continue
            [ "$v2" != '+' ] &&
            quote2 v2
            c0+=" --${n2:2}=$v2"
        done
        # stev: bool params (defaulted with '')
        for n2 in t:no-type v:no-validate; do
            assign2 v2 ${n2:0:1} # stev: using $t and $v
            [ -n "$v2" ] &&
            c0+=" --${n2:2}"
        done
    }

    [ "$h" == '+' ] && h="$YOUTUBE_DATA_HOME"
    [ "$h" == '.' ] && h=''

    [ -n "$h" ] && {
        h="${h%%+(/)}"
        [ -z "$h" ] &&
        h='/'
    }
    [ -n "$h" -a ! -d "$h" ] && {
        error "home dir '$h' not found"
        return 1
    }
    [[ -z "$h" || "$h" =~ /$ ]] || h+='/'

    [[ "$c" != '-' && ! -d "$h.cache" ]] && {
        error "cache directory '$h.cache' not found"
        return 1
    }

    [[ "$c" != '-' && ! -x "${h}cache" ]] && {
        error "cache executable not found"
        return 1
    }

    [ "$act" == 'D' ] && {
        [ -z "$V2" ] && {
            c0+=" --json2 --$r --$l -p"
            [ "${#p}" -gt 1 ] && c0+=' '
            c0+="$p"

            c2="\
diff -u$u \\
-Lold <($c0 --file=$arg) \\
-Lnew <($c0 --$i=$i2)"
        }

        [ -n "$V2" ] && {
            [[ "${V2%%=*}" == @(diff|wdiff)-recent ]] || {
                error "internal: unexpected V2='$V2' [0]"
                return 1
            }

            local o3
            case "$c" in
                -)  o3='no-cache'
                    ;;
                \~) o3='previous'
                    ;;
                \#) o3='local'
                    ;;
                =)  o3='current'
                    ;;
                +)  o3='auto'
                    ;;
                !)  o3='refresh'
                    ;;
                *)  error "internal: unexpected c='$c'"
                    return 1
                    ;;
            esac
            [ "$c" != '-' ] && o3="cache=$o3"

            [[ "$c" == [-~] ]] && {
                error "action \`--${V2%%=*}' isn't working with \`-c$c|--$o3'"
                return 1
            }
            [ "$i" == 'id' ] || {
                error "internal: unexpected i='$i'"
                return 1
            }

            c2="\
$c0 --$r=$i2 --videos --hash -p$p -c$c"

            local h2
            [ "$x" == 'echo' ] && echo "$c2"
            h2=($(eval "$c2")) && [ "${#h2[@]}" -gt 0 ] || {
                error "inner command failed: $c2"
                return 1
            }

            local b2="$b"
            [ "$b2" != '++' ] ||
            { b2="$(date +%s)" && [[ "$b2" == +([0-9]) ]]; } || {
                error "inner command failed: date"
                return 1
            }
            local V3="${V2##*=}"

            local o2
            [ "$p" == '-' ] &&
            o2=" --show-recent${V3:+=$V3}" ||
            o2=' --videos'

            o2+=' --table'
            [ -n "$b2" ] &&
            o2+=" --relative-date=$b2" ||
            o2+=' --no-relative-date'

            local j
            local c3=''
            local c4=''
            local f2
            local h0
            local h1
            local l0=''
            local l1=''
            local n3=0
            local n4=0
            for ((j=0; j<${#h2[@]}; j++)); do
                [[ "${h2[$j]}" =~ ^[-+!]($hrex)(:($hrex))?$ ]] || {
                    error "internal: unexpected \`-c$c|--$o3' hash result '${h2[$j]}'"
                    return 1
                }
                h0="${BASH_REMATCH[3]}"
                h1="${BASH_REMATCH[1]}"

                l0+="${l0:+:}${h0:--}"
                l1+="${l1:+:}$h1"

                [ -n "$h0" -a -s "$h.cache/$h0" ] && {
                    [ "$n3" -gt 0 ] && c3+=$'\n\t'
                    c3+="\
$c0 --$r=#$h0$o2"
                    ((n3 ++))
                }

                [ -s "$h.cache/$h1" ] && {
                    [ "$n4" -gt 0 ] && c4+=$'\n\t'
                    c4+="\
$c0 --$r=#$h1$o2"
                    ((n4 ++))
                }
            done

            [ "$n3" -gt 1 -o "$n4" -gt 1 ] && {
                [ -n "$c3" ] && c3=$'\n\t'"$c3"
                [ -n "$c4" ] && c4=$'\n\t'"$c4"
            }

            [ -n "$c3" ] && c3="<($c3)" || c3='/dev/null'
            [ -n "$c4" ] && c4="<($c4)" || c4='/dev/null'

            # stev: reset $n when is 'auto' and
            # stdout is not opened on a terminal
            [ "$n" == 'auto' -a ! -t 1 ] && n=''

            local V3="${V2:0:1}"

            [ "$V3" == 'd' ] && c2="\
diff -u$u \\"
            [ "$V3" == 'w' ] && c2="\
wdiff -n \\"
            [ "$V3" == 'w' -a -n "$n" ] && c2+="
-w \$'$yellow' \\
-x \$'$close' \\
-y \$'$green' \\
-z \$'$close' \\"
            [ "$V3" == 'd' ] && c2+="
-L $l0 "
            [ "$V3" == 'w' ] && c2+=$'\n'
            c2+="$c3 \\"
            [ "$V3" == 'd' ] && c2+="
-L $l1 "
            [ "$V3" == 'w' ] && c2+=$'\n'
            c2+="$c4"
            [ "$V3" == 'd' ] && c2+="|
sed -ru '
    1,3d"
            [ "$V3" == 'd' -a -n "$n" ] && c2+="
    s/^ //;t
    s/^\-(.*)$/$yellow\1$close/;t
    s/^\+(.*)$/$green\1$close/;t"
            [ "$V3" == 'd' ] && c2+="
    s/^@.*$/.../'"
        }

        $x "$c2"
        return $?
    }

    local P=''

    [ "$act" == 'P' ] && {
        [ -n "$V2" ] && {
            [ "${V2%%=*}" == 'show-recent' ] || {
                error "internal: unexpected V2='$V2' [1]"
                return 1
            }

            # stev: ensure that action `-V|--show-recent' implies querying
            # the remote service only for the first 'videos' page linked to
            # the specified resource; this is a tolerable restriction that
            # simplifies the implementation below to a substantial extent!
            local V3="${V2##*=}"
            [ -z "$V3" ] && V3=5
            local m2="$m"
            [ "$m2" == '+' ] && m2=50
            [ "$i" == 'id' -a "$V3" -gt "$m2" ] && {
                error "argument of \`--$V2' exceeds argument of \`-m|--max-results=$m2'"
                return 1
            }
            [ "$c" == '-' ] && m="$V3"
            p='-'
        }

        local j
        case "$r:$l" in
            channel:itself)
                j=0 ;;
            playlist:itself)
                j=1 ;;
            video:itself)
                j=2 ;;
            channel:playlists)
                j=3 ;;
            channel:videos|playlist:videos)
                j=4 ;;
            *)	error "internal: unexpected r='$r' and l='$l' [0]"
                return 1
                ;;
        esac
        P="${ypar[$j]}"

        #!!! echo >&2 "!!! arg='$arg'"

        local P2="${arg#*=}"
        arg="${arg%%=*}"

        [ "$arg" == '+' ] &&
        case "$r:$l" in
            @(channel|playlist|video):itself)
                arg='list'
                ;;
            channel:@(playlists|videos)|playlist:videos)
                arg='table'
                ;;
            *)	error "internal: unexpected r='$r' and l='$l' [1]"
                return 1
                ;;
        esac

        #!!! echo >&2 "!!! arg='$arg' P2='$P2'"

        if [ "$P2" == '?' ]; then
            sed -r 's/[A-Z]/-\L\0\E/g;s/ +/\n/g' <<< "$P" || {
                error "internal: inner command failed: sed [1]"
                return 1
            }
            return 0
        elif [ "$P2" == '+' ]; then
            [ "$arg" == 'table' ] &&
            P="${P/ description/}"
        elif [ "$P2" != '++' ]; then
            local e2="invalid $arg spec for --$r --$l"

            [[ "$P2" == $prex ]] || {
                quote P2
                [ "${P2:0:2}" == '$'"'" ] &&
                P2="${P2:2:$((${#P2} - 3))}"

                error "$e2: \"$P2\""
                return 1
            }

            local s2='
                BEGIN {
                    n = split(params, C, " ")
                    for (i = 1; i <= n; i ++)
                        B[C[i]] = 1
                }
                function error(i, w)
                {
                    printf("%s%s\n", w, $i)
                    exit 0
                }
                {
                    for (i = 1; i <= NF; i ++) {
                        if (!($(i) in B))
                            error(i, "-")
                        if (D[$(i)] ++)
                            error(i, "+")
                    }
                }'

            local P3=''
            P3="$(sed -r 's/[A-Z]/-\L\0\E/g' <<< "$P")" && [ -n "$P3" ] || {
                error "internal: inner command failed: sed [2]"
                return 1
            }

            local n2
            n2="$(awk -F ',' -v params="$P3" "$s2" <<< "$P2")" || {
                error "internal: inner command failed: awk"
                return 1
            }
            [ -z "$n2" ] || {
                local e3
                case "${n2:0:1}" in
                    -)	e3='illegal name'
                        ;;
                    +)	e3='duplicate name'
                        ;;
                    *)	error "internal: unexpected n2='$n2'"
                        return 1
                        ;;
                esac
                n2="${n2:1}"
                quote n2
                [ "${n2:0:2}" == '$'"'" ] &&
                n2="${n2:2:$((${#n2} - 3))}"

                error "$e2: $e3: \"$n2\""
                return 1
            }

            P="$(sed -r 's/-([a-z])/\U\1\E/g;s/,/ /g' <<< "$P2")" &&
                [ -n "$P" ] || {
                error "internal: inner command failed: sed [3]"
                return 1
            }
        fi

        #!!! echo >&2 "!!! arg='$arg' P2='$P2'"
    }

    # stev: from now: [ -n "$i2" ]

    [ "$i" == 'id' ] && {
        [ "$y" == '+' ] && {
            y="$YOUTUBE_DATA_APP_KEY"
            [ -z "$y" ] && {
                error "\$YOUTUBE_DATA_APP_KEY is null"
                return 1
            }
        }
        quote2 -i y
    }

    # stev: normalize $p
    [ "${p:0:1}" == '+' -a -n "${p:1}" ] &&
    [ "${p:1}" -eq 1 ] &&
    p='-'

    local u2=''

    # stev: [[ "$act" == [HIJU] ]] => [ "$i" == 'id' ]

    [[ "$act" == [HIJPSU] && "$i" == 'id' && "${p:0:1}" != '+' ]] && {
        [[ ( "$act" == 'I' && -n "$V2" || "$act" == 'U' ) && \
                "${p:0:1}" == '+' || "$p" == '-' ]] && p=''
        [ "$m" == '+' ] && m=50

        [[ "$act" == [HIJU] || "$i" == 'id' ]] &&
        case "$r:$l" in
            @(channel|playlist|video):itself)
                local j
                case "$r" in
                    channel)
                        j=0 ;;
                    playlist)
                        j=1 ;;
                    video)
                        j=2 ;;
                    *)	error "internal: unexpected r='$r'"
                        return 1
                        ;;
                esac
                printf -v u2 "$yurl${yqry[0]}$ymax${p:+$ypgt}" \
                    "$r" "$y" "$i2" "${yprt[$j]}" "$m" ${p:+"${p:1}"}
                ;;
            channel:@(playlists|videos))
                printf -v u2 "$yurl${yqry[1]}$ymax${p:+$ypgt}" \
                    "$y" "$i2" "${l%s}" "${yprt[3]}" "$m" ${p:+"${p:1}"}
                ;;
            playlist:videos)
                printf -v u2 "$yurl${yqry[2]}$ymax${p:+$ypgt}" \
                    "$y" "$i2" "${yprt[4]}" "$m" ${p:+"${p:1}"}
                ;;
            *)	error "internal: unexpected r='$r' and l='$l' [2]"
                return 1
                ;;
        esac

        [[ -n "$u2" && "$c" == '~' ]] && u2+='$previous'

        [ "$act" == 'U' ] && {
            echo "$u2"
            return 0
        }
    }

    # stev: from now on $act is '[HIJPSU]'

    # stev: complete a hash file name reference
    [[ "$i" == 'file' && "$i2" =~ ^#$hrex$ ]] && {
        local i3="$h.cache/${i2:1}"
        [ -f "$i3" ] || {
            error "input hash file '$i2' not found"
            return 1
        }
        i2="$i3" 
    }

    # reverse $t
    [ -z "$t" ] && t='t' || t=''

    # set $b if default
    [ "$b" == '+' ] && { 
        [ "$act" == 'I' ] &&
        b='++' ||
        b=''
    }

    local T=''
    [ -n "$t" ] && {
        local t2
        local T2=''
        case "$r:$l" in
            @(channel|playlist|video):itself)
                t2="${r}s"
                ;;
            channel:@(playlists|videos))
                t2="search-$l"
                T2='search'
                ;;
            playlist:videos)
                t2="$r-items"
                ;;
            *)	error "internal: unexpected r='$r' and l='$l' [3]"
                return 1
                ;;
        esac
        local t3="${h}$t2.json"
        local t4="${h:-./}$self.so"
        [ "$t3" -nt "$t4" ] &&
        t="$t3" ||
        t="$t4"
        [ ! -f "$t" ] && {
            error "json type-lib '$t' not found"
            return  1
        }
        [ "$t" == "$t4" ] &&
        t+=":$t2"

        test -z "$T2" && T2="$t2"
        local T3="${h}$T2-litex.json"
        local T4="${h:-./}$self-litex.so"
        [ "$T3" -nt "$T4" ] &&
        T="$T3" ||
        T="$T4"
        [ ! -f "$T" ] && {
            error "json path-lib '$T' not found"
            return  1
        }
        [ "$T" == "$T4" ] &&
        T+=":$T2"
    }

    [ "$act" == 'J' -a "$o" != '-' ] && {
        local o2=''
        [ "${o:0:1}" == '+' ] &&
        o2="${o:1}"

        # stev: be aware of duplicated code
        # computing file name from $i2 and $l
        [ "${o:0:1}" == '+' ] &&
        if [ "$l" != 'itself' ]; then
            o="$i2-$l.json"
        elif [ "$r" == 'video' ]; then
            o="VD$i2.json"
        else
            o="$i2.json"
        fi
        [ -n "$o2" ] && o+=".$o2"

        [ -z "$o" ] && {
            error "output file cannot be null"
            return 1
        }
        [ "$e" == '-' -a -e "$o" ] && {
            error "output file '$o' already exists"
            test "$x" == "eval" && return 1
        }
    }
    quote o

    # stev: [[ "$act" == [HIJ] ]] => [ "$i" == 'id' ]

    [ "$i" == 'id' ] && {
        [ -z "$a" ] && {
            error "user-agent cannot be null"
            return 1
        }
        [ "$a" == '+' ] && a="$YOUTUBE_DATA_AGENT"
        [ -z "$a" ] && {
            error "\$YOUTUBE_DATA_AGENT is null"
            return 1
        }
        quote2 a

        [ "$k" == '+' ] && {
            k="$YOUTUBE_DATA_COOKIE"
            [ -z "$k" ] && {
                error "\$YOUTUBE_DATA_COOKIE is null"
                return 1
            }
        }
        quote2 -i k
    }

    local d0=''
    local d1=''

    [[ "$act" == [IP] ]] && {
        d0='
                    UNIT[0] = "sec"
                    UNIT[1] = "min"
                    UNIT[2] = "hour"
                    UNIT[3] = "day"
                    UNIT[4] = "week"
                    UNIT[5] = "month"
                    UNIT[6] = "year"

                    DIVS[0] = 60
                    DIVS[1] = 60
                    DIVS[2] = 24
                    DIVS[3] = 7
                    DIVS[4] = 4
                    DIVS[5] = 12'
        [ "$act" != 'P' -o "$arg" != 'list' ] && d0+='

                    # max of prec + space + max of len + 1 +
                    # space +
                    # max of prec + space + max of len + 1 +
                    # space + max suffix len
                    WIDTH = 2 + 1 + 6 + \
                            1 + \
                            2 + 1 + 6 + \
                            1 + 5'
        [ "$b" == '++' ] && d0+='

                    time = systime()'
        [ "$b" != '++' ] && d0+='

                    time = '"$b"

        d1='
                function make_relative(t,	s, b, n, u, i, j, d, v)
                {
                    t = (s = t > time) ? t - time : time - t

                    if (t == 0)'
        [ "$act" != 'P' -o "$arg" != 'list' ] && d1+='
                        return sprintf("%*s", WIDTH, "now")'
        [ "$act" == 'P' -a "$arg" == 'list' ] && d1+='
                        return "now"'
        d1+='

                    n = 7
                    for (i = 0; t && i < n - 1; i ++) {
                        d = DIVS[i]
                        u[i] = t % d
                        t = int(t / d)
                    }
                    u[i] = t

                    if (u[6] >= 100) {
                        b = "+100 years"
                        # stev: step over the "for" loop
                        n = 0
                    }

                    for (i = n - 1; i >= 0; i --) {
                        if (!(v = u[i]))
                            continue'
        [ "$act" != 'P' -o "$arg" != 'list' ] && d1+='

                        b = b sprintf( \
                            "%s%2d %*s%s", \
                            j == 0 ? "" : " ", v, \
                            6 - (v != 1), UNIT[i], \
                            v != 1 ? "s" : "")'
        [ "$act" == 'P' -a "$arg" == 'list' ] && d1+='

                        b = b sprintf( \
                            "%s%d %s%s", \
                            j == 0 ? "" : " ", v, \
                            UNIT[i], v != 1 ? "s" : "")'
        d1+='

                        if (++ j >= 2)
                            break
                    }'
        [ "$act" != 'P' -o "$arg" != 'list' ] && d1+='

                    b = b sprintf(" %-5s", s ? "ahead" : "ago")
                    return sprintf("%*s", WIDTH, b)'
        [ "$act" == 'P' -a "$arg" == 'list' ] && d1+='

                    return b " " (s ? "ahead" : "ago")'
        d1+='
                }'
    }

    if [[ "$act" == [HIJPS] && ! ( "$i" == 'id' && "${p:0:1}" == '+' ) ]]; then
        quote2 -i u2
        quote i2

        local a2
        case "$act" in
            H) a2='hash' ;;
            I) a2='info' ;;
            *) a2='content' ;;
        esac

        local e2="$e"
        [ "$e2" != '+' ] && e2=''

        local o2=''
        [ "$act" != 'J' -o -n "$e2" -o -n "$t" ] && {
            o2="$o"
            o='-'
        }

        local j
        [ "$act" != 'J' ] &&
        j='json2' ||
        j='echo'

        local j2=''
        [ "$i" == 'file' -a "$l" != 'itself' ] &&
        j2='m'

        local s1=''
        [ "$act" == 'S' ] && {
            s1='
                BEGIN { 
                    g = 1
                }
                function starts_with(s, t)
                { return substr(s, 1, length(t)) == t }
                {
                    n = length($0)
                    if (n && !starts_with($0, "/items"))
                        printf("0\t%s\n", $0)
                    else {
                        if (n == 0 || n == 6)
                            g ++
                        if (g == 1 && !p ++)
                            printf("1\t/items\n")
                        printf("%d\t%s\n", g, n \
                            ? $0 : "/items")
                    }
                }'
        }

        local s2=''
        [ "$act" == 'S' ] && {
            s2='
                function starts_with(s, t)
                { return substr(s, 1, length(t)) == t }
                function print_group()
                {
                    if (D && G) {
                        printf("%s\t%s\n", D, G)
                        D = G = ""
                    }
                }
                {
                    if (!starts_with($0, "/items")) {
                        printf("-\t%s\n", $0)
                        next
                    }
                    if (length($0) == 6)
                        print_group()
                    if (starts_with($0, "/items/snippet/publishedAt="))
                        D = substr($0, 28)
                    G = G ? G "\001" $0 : $0
                }
                END {
                    print_group()
                }'
        }

        local k2=0
        local s3=''
        local video_id=''
        local playlist_id=''
        local published_at=''
        local description=''
        local duration=''
        local title=''

        [ "$act" == 'P' ] && {
            local V3="${V2##*=}"
            [ -z "$V3" ] && V3=5
            [ "$w" == '+' ] && w=72

            local n2

            k2=1
            for n2 in $P; do # stev: do not quote $P
                case "$n2" in
                    videoId)
                        video_id="$k2" ;;
                    playlistId)
                        playlist_id="$k2" ;;
                    publishedAt)
                        published_at="$k2" ;;
                    description)
                        description="$k2" ;;
                    duration)
                        duration="$k2" ;;
                    title)
                        title="$k2" ;;
                esac
                ((k2 ++))
            done

            s3='
                BEGIN {
                    TRIM_LEFT = 1
                    TRIM_RIGHT = 2
                    TRIM_EDGES = 3
                    TRIM_INSIDE = 4'
            [ -n "$b" -a -n "$published_at" ] && s3+=$'\n'"$d0"
            s3+='

                    gsep = "'"$yitm"'"
                    path = "'"$yitm/"'"
                    poff = '"$((${#yitm} + 2))"'
                    n = '"$(($k2 - 1))"'

                    # parameter table'

            local n3
            local p2
            local k3=''
            case "$r:$l" in
                video:itself)
                    k3=0 ;;
                channel:playlists)
                    k3=1 ;;
                channel:videos)
                    k3=2 ;;
                playlist:videos)
                    k3=3 ;;
            esac

            k2=0
            for n2 in $P; do # stev: do not quote $P
                p2="$ypth/"
                [ -n "$k3" ] &&
                for n3 in ${ypex[$k3]}; do # stev: do not quote $ypex
                    [ "$n2" == "${n3##*/}" ] && {
                        p2="${n3%/*}/"
                        break
                    }
                done
                s3+='
                    P["'"$p2$n2"'"] = '"$((++ k2))"
            done

            s3+='

                    # parameter names'

            k2=0
            for n2 in $P; do # stev: do not quote $P
                s3+='
                    N['"$((++ k2))"'] = "'"$n2"'"'
            done

            [ -z "$v" -a \( \
              -n "$video_id" -o \
              -n "$playlist_id" -o \
              -n "$published_at" -o \
              -n "$duration" \) ] && s3+='

                    # validating regexes'
            [ -z "$v" -a -n "$video_id" ] && s3+='
                    R['"$video_id"'] = "'"${vrex//\\/\\\\}"'"'
            [ -z "$v" -a -n "$playlist_id" ] && s3+='
                    R['"$playlist_id"'] = "'"${lrex//\\/\\\\}"'"'
            [ -z "$v" -a -n "$published_at" ] && s3+='
                    R['"$published_at"'] = "'"${drex//\\/\\\\}"'"'
            [ -z "$v" -a -n "$duration" ] && s3+='
                    R['"$duration"'] = "'"${trex//\\/\\\\}"'"'
            [ -n "$title" -o \
              -n "$description" ] && s3+='

                    # trimming table'
            [ -n "$title" ] && s3+='
                    T['"$title"'] = TRIM_INSIDE'
            [ -n "$description" ] && s3+='
                    T['"$description"'] = TRIM_INSIDE'
            [ -n "$title" -o \
              -n "$description" ] && s3+='

                    # multi-line params'
            [ -n "$title" ] && s3+='
                    M['"$title"'] = 1'
            [ -n "$description" ] && s3+='
                    M['"$description"'] = 1'
            s3+='
                }

                function error(s)
                {
                    printf("'"$self"': error: #%d: %s\n", FNR, s) \
                        > "/dev/stderr"
                    input_error = 1
                    exit 1
                }'
            [ -z "$v" -o \( -n "$b" -a -n "$published_at" \) ] && s3+='

                function invalid_value(p, v)
                { error(sprintf("invalid parameter value \x22%s=%s\x22", N[p], v)) }'
            s3+='

                function duplicate_param(p)
                { error(sprintf("duplicate parameter \x22%s\x22", N[p])) }

                function missing_param(p)
                { error(sprintf("missing parameter \x22%s\x22", N[p])) }

                function starts_with(s, t)
                { return substr(s, 1, length(t)) == t }

                function trim_spaces(s, w)
                {
                    if (and(w, TRIM_EDGES) == TRIM_EDGES)
                        s = gensub(/^[[:space:]]+|[[:space:]]+$/, "", "g", s)
                    else
                    if (and(w, TRIM_LEFT))
                        s = gensub(/^[[:space:]]+/, "", "", s)
                    else
                    if (and(w, TRIM_RIGHT))
                        s = gensub(/[[:space:]]+$/, "", "", s)
                    if (and(w, TRIM_INSIDE))
                        s = gensub(/[[:space:]]+/, " ", "g", s)
                    return s
                }'
            [ -n "$b" -a -n "$published_at" ] && s3+=$'\n'"$d1"'

                function parse_date(p, v,	r)
                {
                    # 1234567890123456789
                    # YYYY-MM-DDTHH:MM:SS.SSSZ
                    r = mktime( \
                        substr(v,  1, 4) " " \
                        substr(v,  6, 2) " " \
                        substr(v,  9, 2) " " \
                        substr(v, 12, 2) " " \
                        substr(v, 15, 2) " " \
                        substr(v, 18, 2) " 0")

                    if (r == -1)
                        invalid_value(p, v)

                    return r
                }

                function format_date(p, v)
                { return make_relative(parse_date(p, v)) }'
            [ "$arg" == 'list' -a -n "$w" ] && s3+='

                function format_text(p, v,	s, w, l, r, k)
                {
                    # stev: assuming v is left-trimmed

                    s = ""
                    w = '"$(($w - 1))"' - length(N[p])
                    r = v

                    # stev: invariant at the start of
                    # each iteration of the following
                    # loop: r is left-trimmed!

                    while (length(r) > w) {
                        l = substr(r, 1, w)  # lhs
                        r = substr(r, w + 1) # rhs'
            [ "$arg" == 'list' -a -n "$w" -a "$YOUTUBE_DATA_DEBUG_TEXT_WRAP" == 'yes' ] && s3+='

                        printf("!!! 0 !!! \"%s\"\"%s\"\n", l, r) > "/dev/stderr"'
            [ "$arg" == 'list' -a -n "$w" ] && s3+='

                        # notation:
                        # [rhs] rhs starts with space
                        # [lhs] lhs ends with space

                        # cases at the cut-point:
                        # (1) [rhs] || [lhs] =>
                        #     trim_left(rhs) and trim_right(lhs)
                        # (2) ![rhs] && ![lhs] =>
                        #     move the cut-point leftward at the last
                        #     non-space position, if there is such a
                        #     position; otherwise leave the cut-point
                        #     where it is and only append "\\" to the
                        #     lhs; if the cut-point gets moved to the
                        #     left, trim rhs and lhs as did at (1)

                        if (match(r, /^[[:space:]]/) || \
                            !match(l, /[^[:space:]]+$/)) {'
            [ "$arg" == 'list' -a -n "$w" -a "$YOUTUBE_DATA_DEBUG_TEXT_WRAP" == 'yes' ] && s3+='
                            printf("!!! 1 !!! \"%s\"\"%s\"\n", l, r) > "/dev/stderr"'$'\n'
            [ "$arg" == 'list' -a -n "$w" ] && s3+='
                            l = trim_spaces(l, TRIM_RIGHT)
                            r = trim_spaces(r, TRIM_LEFT)
                        }
                        else
                        if (RLENGTH < length(l)) {
                            k = length(l) - RLENGTH
                            r = substr(l, k + 1) r
                            l = substr(l, 1, k)'
            [ "$arg" == 'list' -a -n "$w" -a "$YOUTUBE_DATA_DEBUG_TEXT_WRAP" == 'yes' ] && s3+='

                            printf("!!! 2 !!! \"%s\"\"%s\" k=%d\n", l, r, k) > "/dev/stderr"'
            [ "$arg" == 'list' -a -n "$w" ] && s3+='

                            l = trim_spaces(l, TRIM_RIGHT)
                            r = trim_spaces(r, TRIM_LEFT)
                        }
                        else
                            l = l "\\"

                        s = s ? s "\n" l : l
                    }

                    return s && r ? s "\n" r : s ? s : r
                }'
            s3+='

                function check_params(  i)
                {
                    for (i = 1; i <= n; i ++) {
                        if (!A[i])
                            missing_param(i)
                    }
                }'
            [ "$arg" == 'list' ] && s3+='

                function print_named_value(n, v)'
            [ "$arg" == 'list' -a "$k2" -le 1 ] && s3+='
                { printf("%s\n", v) }'
            [ "$arg" == 'list' -a "$k2" -gt 1 ] && s3+='
                { printf("%s=%s\n", n, v) }'
            [ "$arg" == 'list' ] && s3+='

                function print_multi_value(n, v,	l, a, i)
                {
                    l = split(v, a, "\n")
                    for (i = 1; i <= l; i ++)
                        print_named_value(n, a[i])
                }

                function print_param(i)
                {
                    if (i == 1 && n_items > 1)
                        printf("\n")
                    if (M[i])
                        print_multi_value(N[i], V[i])
                    else
                        print_named_value(N[i], V[i])
                }'
            [ "$arg" == 'table' ] && s3+='

                function print_param(i)
                {
                    printf("%s%s", V[i], i < n ? "\t" : "\n")
                }'
            s3+='

                function print_params(	i)
                {'
            [ "$arg" == 'list' -o -n "$V2" ] && s3+='
                    n_items ++'$'\n'
            s3+='
                    check_params()
                    for (i = 1; i <= n; i ++)
                        print_param(i)
                    delete V
                    delete A'
            [ -n "$V2" ] && s3+=$'\n''
                    if (n_items >= '"$V3"') {
                        params_printed = 1
                        exit 0
                    }'
            s3+='
                }

                function process_line(l,	k, i, p, v)
                {
                    k = index(l, "=")
                    if (!k)
                        next

                    p = substr(l, 1, k - 1)
                    if (!(i = P[p]))
                        next

                    if (A[i] && !M[i])
                        duplicate_param(i)

                    v = trim_spaces(substr(l, k + 1), \
                            or(TRIM_EDGES, T[i]))'
            [ -z "$v" ] && s3+='
                    if (R[i] && !match(v, R[i]))
                        invalid_value(i, v)'
            [ -n "$b" -a -n "$published_at" ] && s3+='

                    # stev: format "publishedAt"
                    if (i == '"$published_at"')
                        v = format_date(i, v)'
            [ "$arg" == 'list' -a -n "$w" ] && s3+='

                    # stev: format multi-line trimmable params
                    if (M[i] && T[i])
                        v = format_text(i, v)'
            [ "$arg" == 'list' ] && s3+='

                    V[i] = A[i] ? V[i] "\n" v : v'
            [ "$arg" == 'table' ] && s3+='

                    if (v)
                        V[i] = V[i] ? V[i] " | " v : v'
            s3+='
                    A[i] = 1
                }

                {
                    if (!length($0) || $0 == gsep)
                        print_params()
                    else
                    if (starts_with($0, path))
                        process_line(substr($0, poff))
                }

                END {
                    if (FNR > 0 && !input_error'"${V2:+ && !params_printed}"')
                        print_params()
                }'

            # stev: reset $published_at when producing lists
            [ "$arg" != 'table' ] && published_at=''
        }

        local s4=''
        [ "$act" == 'P' -a "$arg" == 'table' -a "$k2" -gt 1 ] && {
            s4='
                BEGIN {
                    OFS = FS
                    N = 0
                }
                function record_line(	i, l)
                {
                    for (i = 1; i < NF; i ++) {
                        l = length($i)
                        if (W[i] < l)
                            W[i] = l
                    }
                    L[N ++] = $0
                }
                function print_table(	i, j, n, T)
                {
                    for (i = 0; i < N; i ++) {
                        n = split(L[i], T)
                        for (j = 1; j < n; j ++)
                            printf("%-*s  ", W[j], T[j])
                        if (n > 0)
                            printf("%s\n", T[n])
                    }
                }
                {
                    record_line()
                }
                END {
                    print_table()
                }'
        }

        [ -n "$u2" ] && {
            local t2
            case "$c" in
                !)	t2=0
                    ;;
                [~#=])
                    t2='inf'
                    ;;
                +)	t2="$YOUTUBE_DATA_CACHE_THRESHOLD"
                    [ -n "$t2" ] || {
                        error "\$YOUTUBE_DATA_CACHE_THRESHOLD is null"
                        return 1
                    }
                    [[ "$t2" =~ ^[0-9]+[dhms]?$ ]] || {
                        error "invalid \$YOUTUBE_DATA_CACHE_THRESHOLD='$t2'"
                        return 1
                    }
                    ;;
            esac
            quote h
            quote t
            quote T

            local b2="$b"
            [ "$b2" == '++' ] && b2=''

            [[ "$act" != 'I' && "$c" == '-' ]] && c2+="\
""{
wget \\"
            [[ "$act" == 'I' || "$c" != '-' ]] && c2+="\
youtube-wget \\"
            [[ "$act" != 'I' && "$c" == '-' ]] && c2+="
--quiet \\"
            [[ "$act" != 'I' ]] && c2+="${g:+
--debug \\}
--no-check-certif \\"
            [[ "$act" != [HI] ]] && c2+="
--output-document=$o \\"
            [[ "$act" != 'I' ]] && c2+="
--user-agent=$a \\${k:+
--header='Cookie: $k' \\}"
            [[ "$act" == 'I' && ( -n "$V2" && -n "$b" ) ]] && c2+="
--hash-age${b2:+=$b2} \\"
            [[ "$act" == 'I' || "$c" != '-' ]] && c2+="
--action=$a2 \\
--home=$h \\"
            [[ "$act" != 'I' && "$c" == '#' ]] && c2+="
--cache-only \\"
            [[ "$act" != 'I' && "$c" != [-#] ]] && c2+="
--threshold=$t2 \\
--keep-prev \\
--save-key \\"
            [[ "$act" == 'I' || "$c" != '-' ]] && c2+="
--url="
            [[ "$act" != 'I' && "$c" == '-' ]] && c2+=$'\n'
            c2+="\
'$u2'"
        # stev: ignore 'wget's exit code since is never 141
        [[ "$act" != 'I' && "$c" == '-' ]] && c2+=" ||
true
""}"
        }
        # [ -n "$u2" ] <=> [[ "$act" == [IJ] ]] || [ "$i" == 'id' ]
        [ -n "$u2" ] &&
        [[ "$act" != [HI] ]] && [ "$act" != 'J' -o -n "$t" ] && c2+=$'|\n'
        [[ "$act" != [HI] ]] && [ "$act" != 'J' -o -n "$t" ] && c2+="\
json --$j -Vusyp$j2${t:+ -t $t}${T:+ -f}"
        # [ -z "$u2" ] <=> [[ "$act" == [SP] ]] && [ "$i" == 'file' ]
        [ -z "$u2" -a "$i2" != '-' ] && c2+=" \\
$i2"
        [[ "$act" != [HI] ]] && [ "$act" != 'J' -o -n "$t" ] && [ -n "$T" ] && c2+="\
 -- \\
json-litex.so -p $T -l"
        [ "$act" == 'J' -a "$o" == '-' -a -n "$o2" -a "$o2" != '-' ] && {
            c2+=" >${e2:+>} \\
$o2"
            [ -z "$q" ] && c2+=" &&
printf '%s\n' $o2"
        }
        [ -n "$s1" ] && c2+="|
awk -F '\n' '$s1'|
sort -t $'\t' -k1n,1 -k2,2 -s|
cut -d $'\t' -f2-"
        [ -n "$s2" ] && c2+="|
awk -F '\n' '$s2'|
sort -t $'\t' -k1,1 -s|
cut -d $'\t' -f2-|
tr '\001' '\n'|
uniq"
        [ -n "$s3" ] && c2+="|
awk -F '\n' --re-interval '$s3'"
        [ -n "$s3" -a -z "$b" -a -n "$published_at" ] && c2+="|
sort -t $'\t' -k${published_at}r,$published_at -s"
        [ -n "$s4" ] && c2+="|
awk -F '\t' '$s4'"
    elif [[ "$act" == [HIJU] && "$i" == 'id' && "${p:0:1}" == '+' ]]; then
        quote i2
        quote h

        local a2
        case "$act" in
            H) a2='hash' ;;
            J) a2='json' ;;
            *) a2='info' ;;
        esac

        local o3
        [[ "$act" == [HIU] ]] && o3='> "$t"' || o3='-o "$t" -e= -q'

        local t3
        [[ "$act" == [HIU] ]] && t3="$h"'.cache/"$h"' || t3='"$t"'

        local p2
        [ "${p:0:1}" == '+' ] && p2="${p:1}" || p2=''

        c2=\
'(
    t="$(mktemp '"$tmpf"')" && [ -n "$t" ] || {
        error "inner command failed: mktemp"
        exit 1
    }
    trap "rm -f $t" EXIT'
        [ "$act" == 'J' -a "$o" != '-' -a "$e" != '+' ] && c2+='

    [ ! -f '"$o"' ] ||
    truncate -s0 '"$o"' || {
        error "inner command failed: truncate"
        exit 1
    }'
        c2+='
    '"${p2:+
    k=$p2}"'
    p='\''-'\''
    while [[ -n "$p"'"${p2:+ && \$((k --)) -gt 0}"' ]]; do'
        [ "$act" == 'U' ] && c2+='
        '"$c0"' --'"$r"' --'"$l"' --id='"$i2"' --url -c'"$c"' -p "$p" || {
            error "inner command failed: '"$self"' --'"$r"' --'"$l"' --id='"$i2"' --url -c'"$c"' -p $p"
            exit 1
        }'
        c2+='
        '"$c0"' --'"$r"' --'"$l"' --id='"$i2"' --'"$a2"' -c'"$c"' -p "$p" '"$o3"' || {
            error "inner command failed: '"$self"' --'"$r"' --'"$l"' --id='"$i2"' --'"$a2"' -c'"$c"' -p $p"'
        [ "$act" == 'J' -a "$o" != '-' ] && c2+='
            rm -f -- '"$o"
        c2+='
            exit 1
        }
        [ ! -s "$t" ] && break'$'\n'
        [[ "$act" == [HIJ] ]] && c2+='
        cat "$t"'
        [ "$act" == 'J' -a "$o" != '-' ] && c2+=" >> $o"
        [[ "$act" == [HIJ] ]] && c2+=' || {
            error "inner command failed: cat"'
        [ "$act" == 'J' -a "$o" != '-' ] && c2+='
            rm -f -- '"$o"
        [[ "$act" == [HIJ] ]] && c2+='
            exit 1
        }'
        [[ "$act" == [H] ]] && c2+='
        h="$(sed -nr '\''1{/^[-+!]('"$hrex"')(:'"$hrex"')?$/!q;s//\1/;p;q}'\'' "$t")" && [ -n "$h" ] || {
            error "inner command failed: sed"'
        [[ "$act" == [IU] ]] && c2+='
        h="$(cut -f2 "$t")" && [ -n "$h" ] || {
            error "inner command failed: cut"'
        [[ "$act" == [HIU] ]] && c2+='
            exit 1
        }'
        # stev: note that 'json's option `-b256' below makes
        # the encompassing loop work significantly faster
        c2+='
        p="$(json -Jusp -b256 '"$t3"'|sed -nr '\''/^\/nextPageToken=/{s//@/;p;q}'\'')" || {
            error "inner command failed: json|sed"'
        [ "$act" == 'J' -a "$o" != '-' ] &&
        c2+='
            rm -f -- '"$o"
        c2+='
            exit 1
        }
    done'
        [ "$act" == 'J' -a "$o" != '-' ] &&
        [ -z "$q" ] && c2+='
    printf "%s\n" '"$o"
        c2+='
)'
        [ "$act" == 'I' -a -n "$b" ] && c2+="|
awk -F '\t' '
                BEGIN {"$d0"
                }"$d1"
                {
                    printf(\"%s  %s\n\", make_relative(\$1), \$2)
                }'"
    elif [[ "$act" == [SP] && "$i" == 'id' && "${p:0:1}" == '+' ]]; then
        quote i2

        local t2
        [ -n "$t" ] && t2=' --no-type' || t2=''

        c2="\
$c0 --$r --$l --id=$i2$t2 --json -c$c -p$p -o-|
$c0 --$r --$l --file=- --"
        if [ "$act" == 'S' ]; then
            c2+='json2'
        elif [[ "$arg" == $prtx ]]; then
            P="$(sed -r 's/[A-Z]/-\L\0\E/g;s/ /,/g' <<< "$P")" &&
                [ -n "$P" ] || {
                error "internal: inner command failed: sed [4]"
                return 1
            }
            c2+="$arg=$P"

            # stev: by default 'list' and 'table' are non-wrapping,
            # therefore no need to pass `--no-wrap' when $w is null 
            if [ "$w" == '+' ]; then
                c2+=" --wrap"
            elif [ -n "$w" ]; then
                c2+=" --wrap=$w"
            fi

            if [ -z "$b" ]; then
                c2+=' --no-relative-date'
            elif [ "$b" != '++' ]; then
                c2+=' --relative-date='"$b"
            else
                c2+=' --relative-date'
            fi
            # stev: by default 'list' and 'table' are non-coloring,
            # therefore no need to pass `--no-color' when $n is null 
            [ -n "$n" ] && {
                c2+=" --color=$n"
            }
        else
            error "internal: unexpected arg='$arg'"
            return 1
        fi
    else
        error "internal: unexpected act='$act' i='$i' p='$p'"
        return 1
    fi

    $x "$c2"
}

youtube-data-json()
{
    local n
    local k=6

    echo '['

    for n in \
        channels \
        playlists \
        videos \
        search-playlists \
        search-videos \
        playlist-items
    do
        echo -ne '\t{\n'
        echo -ne '\t\t"name": "'$n'",\n'
        echo -ne '\t\t"type": '
        sed -re '2,$s/^/\t\t/' $n.json
        [ $((-- k)) -gt 0 ] &&
        echo -ne '\t},\n' ||
        echo -ne '\t}\n'
    done

    echo ']'
}

youtube-data-litex()
{
    local n
    local k=5

    echo '{'

    for n in \
        channels \
        playlists \
        videos \
        search \
        playlist-items
    do
        [ $((-- k)) -eq 0 ] && k=''
        echo -ne '\t"'$n'": '
        sed -re '2,$s/^/\t/'"${k:+;\$s/$/,/}" \
        $n-litex.json
    done

    echo '}'
}

