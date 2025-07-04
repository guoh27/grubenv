#!/bin/sh
set -e
TMP1=$(mktemp)
TMP2=$(mktemp)

# create
./grubenv "$TMP2" create
grub-editenv "$TMP1" create
cmp "$TMP1" "$TMP2"

# set foo with both tools
./grubenv "$TMP2" set foo=bar
grub-editenv "$TMP1" set foo=bar
cmp "$TMP1" "$TMP2"

# set baz with opposite tools
grub-editenv "$TMP1" set baz=qux
./grubenv "$TMP2" set baz=qux
cmp "$TMP1" "$TMP2"

# set multiple variables
./grubenv "$TMP2" set a=1 b=2 c=3
grub-editenv "$TMP1" set a=1 b=2 c=3
cmp "$TMP1" "$TMP2"

# unset multiple variables
./grubenv "$TMP2" unset a c
grub-editenv "$TMP1" unset a c
cmp "$TMP1" "$TMP2"

# verify multi-set result
./grubenv "$TMP2" get b | grep -qx '2'

# get and list
./grubenv "$TMP2" get foo | grep -qx 'bar'
./grubenv "$TMP2" list | grep -q '^foo=bar$'
./grubenv "$TMP2" list | grep -q '^b=2$'

# unset foo
./grubenv "$TMP2" unset foo
grub-editenv "$TMP1" unset foo
cmp "$TMP1" "$TMP2"

# clear
./grubenv "$TMP2" clear
grub-editenv "$TMP1" unset b baz
cmp "$TMP1" "$TMP2"

# set when file missing should create
rm "$TMP1" "$TMP2"
./grubenv "$TMP2" set alpha=beta
grub-editenv "$TMP1" set alpha=beta
cmp "$TMP1" "$TMP2"

# list when file missing should create and output empty
rm "$TMP1" "$TMP2"
out1=$(grub-editenv "$TMP1" list)
out2=$(./grubenv "$TMP2" list)
[ -z "$out1" ] && [ -z "$out2" ]
cmp "$TMP1" "$TMP2"

# unset when file missing should create
rm "$TMP1" "$TMP2"
./grubenv "$TMP2" unset alpha
grub-editenv "$TMP1" unset alpha
cmp "$TMP1" "$TMP2"

# get missing variable must fail
if ./grubenv "$TMP2" get missing >/dev/null 2>&1; then
  echo "get missing should fail" >&2
  exit 1
fi

rm "$TMP1" "$TMP2"
