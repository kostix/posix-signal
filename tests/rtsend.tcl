#! /usr/bin/tclsh

load [file join [file dir [info script]] .. libposixsignal0.1.so]

rename posix::signal signal

set sig [signal info sigrtmin 1]
signal trap $sig {puts RTSIG}
signal send $sig [pid]
update

