#!/bin/sh
set -e
TMP1=$(mktemp)
TMP2=$(mktemp)

check_equal() {
    grub-editenv "$TMP1" list >"$TMP1.list"
    ./grubenv "$TMP2" list >"$TMP2.list"
    diff -u "$TMP1.list" "$TMP2.list"
}

# verify version output
./grubenv -V | grep -qx '1.0.0'

# create new env files
./grubenv "$TMP2" create
grub-editenv "$TMP1" create
check_equal

# set multiple variables
./grubenv "$TMP2" set a=1 b=2 c=3
grub-editenv "$TMP1" set a=1 b=2 c=3
check_equal

# set additional variable using opposite tools
grub-editenv "$TMP1" set d=4
./grubenv "$TMP2" set d=4
check_equal

# get and list
./grubenv "$TMP2" get a | grep -qx '1'
./grubenv "$TMP2" list | grep -q '^c=3$'

# unset multiple variables
./grubenv "$TMP2" unset a b
grub-editenv "$TMP1" unset a b
check_equal

# clear using our tool, mimic with grub-editenv
./grubenv "$TMP2" clear
for var in c d; do grub-editenv "$TMP1" unset "$var"; done
check_equal

# set when file missing should create
rm "$TMP1" "$TMP2"
./grubenv "$TMP2" set alpha=beta gamma=delta
grub-editenv "$TMP1" set alpha=beta gamma=delta
check_equal

# list when file missing should create and output empty
rm "$TMP1" "$TMP2"
out1=$(grub-editenv "$TMP1" list)
out2=$(./grubenv "$TMP2" list)
[ -z "$out1" ] && [ -z "$out2" ]
check_equal

# unset when file missing should create
rm "$TMP1" "$TMP2"
./grubenv "$TMP2" unset alpha
grub-editenv "$TMP1" unset alpha
check_equal

# get missing variable must fail
if ./grubenv "$TMP2" get missing >/dev/null 2>&1; then
  echo "get missing should fail" >&2
  exit 1
fi

rm "$TMP1" "$TMP2" "$TMP1.list" "$TMP2.list"
