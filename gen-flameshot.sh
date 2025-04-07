# This script assumes that you've already run perf record -F 99 -g -- build/ResourceDragon

perf script | stackcollapse-perf.pl > out.folded
flamegraph.pl out.folded --width 1600 --fontsize 8 > flamegraph.svg
rm out.folded