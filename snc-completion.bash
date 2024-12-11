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

    # These options expect an extra parameter. If the previous option ('$3') was
    # one of these, don't show completion.
    case "$3" in
        -p | --port | -t | --transmit)
            return
    esac

    # If the current option ('$2') starts with a dash, return (in '$COMPREPLY')
    # the possible completions for the current option using 'compgen'.
    case "$2" in
        -*)
            mapfile -t COMPREPLY < <(compgen -W "${opts[*]}" -- "$2")
            ;;

        *)
            compopt -o bashdefault -o default
            ;;
    esac
}

complete -F _snc_completion snc
