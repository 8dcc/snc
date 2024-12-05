#!/usr/bin/env bash

_snc_completion() {
    local cur prev opts
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    cur="${COMP_WORDS[COMP_CWORD]}"

    # Clear the completion reply.
    COMPREPLY=()

    # These options expect an extra parameter. If the previous option was one of
    # these, don't show completion.
    for i in "-p" "--port" "-t" "--transmit"; do
        if [ "$prev" == "$i" ]; then
            return 0
        fi
    done

    # Get the full option list.
    #
    # TODO: Remove options (both short and long versions) that are already in
    # `COMP_WORDS'.
    opts=""
    opts+="-h --help "
    opts+="-r --receive "
    opts+="-t --transmit "
    opts+="-p --port "
    opts+="--print-interfaces "
    opts+="--print-peer-info "
    opts+="--print-progress"

    # Get the available options for the current command.
    COMPREPLY+=( $(compgen -W "${opts}" -- ${cur}) )
}

complete -F _snc_completion snc
