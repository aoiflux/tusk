#!/bin/sh
# zig ar -s is the equivalent of ranlib (updates the archive symbol table)
exec zig ar -s "$@"
