Usage: lsqecc_slicer [options...]
Options:
    -i, --input            File with input. If not provided will read LS Instructions from stdin
    -q, --qasm             File name of file with QASM. When not provided will read as LLI (not QASM)
    -l, --layout           File name of file with layout spec, otherwise the layout is auto-generated (configure with -L)
    -o, --output           File name of output. When not provided outputs to stdout
    -f, --output-format    Requires -o, STDOUT output format: progress, noprogress, machine
    -t, --timeout          Set a timeout in seconds after which stop producing slices
    -r, --router           Set a router: graph_search (default), graph_search_cached
    -g, --graph-search     Set a graph search provider: custom (default), boost (not always available)
    --graceful             If there is an error when slicing, print the error and terminate
    --printlli             Output LLI instead of JSONs
    --noslices             Do the slicing but don't write the slices out
    --cnotcorrections      Add Xs and Zs to correct the the negative outcomes: never (default), always
    --layoutgenerator, -L  Automatically generates a layout for the given number of qubits. Incompatible with -l. Options:
                            - compact (default): Uses Litinski's Game of Surace Code compact layout (https://arxiv.org/abs/1808.02892)
                            - edpc: Uses a layout specified in the EDPC paper by Beverland et. al. (https://arxiv.org/abs/2110.11493)
    --nostagger            Turns off staggered distillation block timing
    --disttime             Set the distillation time (default 10)
    -h, --help             Shows this page        
