#!/usr/bin/env bash

_snc_completion() {
    local opts
    opts=(
        -h --help
        -r --receive
        -t --transmit
        -p --port
        --print-interfaces
        --print-peer-info
        --print-progress
    )

    # Check the the previous option ('$3') for special values or options.
    case "$3" in
        '2>' | '>' | '<')
            # If it was a redirector, show the default file completion.
            compopt -o bashdefault -o default
            return
            ;;

        '-p' | '--port' | '-t' | '--transmit')
            # These options expect an extra parameter, so don't show completion.
            return
            ;;
    esac

    # If the current option ('$2') starts with a dash, return (in '$COMPREPLY')
    # the possible completions for the current option using 'compgen'.
    if [[ "$2" =~ -* ]]; then
        mapfile -t COMPREPLY < <(compgen -W "${opts[*]}" -- "$2")
    fi
}

complete -F _snc_completion snc
