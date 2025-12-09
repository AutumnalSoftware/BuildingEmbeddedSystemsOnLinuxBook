#!/usr/bin/env bash
#
# rt_run.sh - run a command with CPU affinity + round-robin scheduling
#
# Usage:
#   ./rt_run.sh <cpu-list> <priority> <command> [args...]
#
# Example:
#   ./rt_run.sh 2 20 ./sensor_sim

if [ "$#" -lt 3 ]
then
    echo "Usage: $0 <cpu-list> <priority> <command> [args...]"
    exit 1
fi

CPU_LIST="$1"
PRIO="$2"
shift 2

# taskset pins the process to the given CPUs
# chrt sets SCHED_RR with the given priority
exec taskset -c "$CPU_LIST" chrt -r "$PRIO" "$@"
