#!/usr/bin/env bash
#
# rt_shield_teardown.sh - remove the CPU shield and restore original layout

CGROOT="/sys/fs/cgroup/cpuset"
RT_SET="$CGROOT/pseudo_rt"
SYS_SET="$CGROOT/system"
ORIG_FILE="./rt_shield_original_cpus.txt"

if [ ! -d "$CGROOT" ]
then
    echo "cpuset cgroup not mounted at $CGROOT"
    exit 1
fi

if [ ! -f "$ORIG_FILE" ]
then
    echo "No original cpu record found, nothing to restore."
else
    ORIG_CPUS=$(cat "$ORIG_FILE")
    echo "$ORIG_CPUS" > "$CGROOT/cpuset.cpus"
    rm -f "$ORIG_FILE"
    echo "Restored root cpuset to: $ORIG_CPUS"
fi

# Move any stray tasks back to root
for set in "$RT_SET" "$SYS_SET"
do
    if [ -d "$set" ]
    then
        while read -r pid
        do
            echo "$pid" > "$CGROOT/tasks" 2>/dev/null || true
        done < "$set/tasks" 2>/dev/null || true
        rmdir "$set" 2>/dev/null || true
    fi
done

echo "CPU shield removed."
