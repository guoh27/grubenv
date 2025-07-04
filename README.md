# grubenv
Simple Grub Environment Block Edit Tool with support for the same commands as
`grub-editenv`.  The `set` and `unset` commands accept multiple arguments just
like `grub-editenv`.

The provided `Makefile` will honor environment variables such as `CC` and
`CFLAGS` so it can be used easily in cross-compilation environments like Yocto.

Run `make install DESTDIR=/path` to install the binary, which is useful when
packaging with systems like Yocto.

To check the tool version, use `grubenv -V` which prints the constant defined in
`version.h`.
