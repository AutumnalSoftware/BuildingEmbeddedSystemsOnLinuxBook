#!/usr/bin/env bash
#
# rt_shield_setup.sh - create an ephemeral CPU "shield" using cpusets
#
# Example:
#   sudo ./rt_shield_setup.sh 2

if [ "$#" -ne 1 ]
then
    echo "Usage: $0 <iso-cpu-list>"
    exit 1
fi

ISO_CPUS="$1"

# Directories for cpusets (cpuset v1 style; for v2 you'd adapt the paths)
CGROOT="/sys/fs/cgroup/cpuset"
RT_SET="$CGROOT/pseudo_rt"
SYS_SET="$CGROOT/system"

if [ ! -d "$CGROOT" ]
then
    echo "cpuset cgroup not mounted at $CGROOT"
    exit 1
fi

# Save original CPU layout so teardown can restore it
ORIG_FILE="./rt_shield_original_cpus.txt"

if [ ! -f "$ORIG_FILE" ]
then
    cat "$CGROOT/cpuset.cpus" > "$ORIG_FILE"
fi

ORIG_CPUS=$(cat "$ORIG_FILE")

echo "Original cpus: $ORIG_CPUS"
echo "Isolating cpus: $ISO_CPUS"

# Create sets
mkdir -p "$RT_SET"
mkdir -p "$SYS_SET"

# Memory nodes (most desktops: just 0)
echo 0 > "$CGROOT/cpuset.mems"
echo 0 > "$RT_SET/cpuset.mems"
echo 0 > "$SYS_SET/cpuset.mems"

# Assign CPUs
echo "$ISO_CPUS"      > "$RT_SET/cpuset.cpus"
# everything except ISO_CPUS (user will edit this if needed)
echo "$ORIG_CPUS"     > "$SYS_SET/cpuset.cpus"

echo "Shield created. To run a task in the shield:"
echo "  echo \$PID > $RT_SET/tasks"
