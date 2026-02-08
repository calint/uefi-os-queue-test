#!/bin/sh
# cache_analysis.sh

for cores in 1 2 4 8; do
    echo "=== Testing with $cores cores ==="
    perf stat -e cache-references,cache-misses,L1-dcache-load-misses,LLC-load-misses \
        ./run.sh $cores 10000 2>&1 | grep -E "cache|seconds"
    echo ""
done
