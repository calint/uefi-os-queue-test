#!/bin/sh
set -e

JOBS=50000

echo "Queue SPMC"
for cores in 1 2 4 8; do
    echo "=== Testing with $cores cores ==="
    perf stat -e cache-references,cache-misses,L1-dcache-load-misses,LLC-load-misses \
        ./run-test1.sh $cores $JOBS 2>&1 | grep -E "cache|seconds"
    echo ""
done

echo "Queue MPMC"
for cores in 1 2 4 8; do
    echo "=== Testing with $cores cores ==="
    perf stat -e cache-references,cache-misses,L1-dcache-load-misses,LLC-load-misses \
        ./run-test2.sh 1 $cores $JOBS 2>&1 | grep -E "cache|seconds"
    echo ""
done
