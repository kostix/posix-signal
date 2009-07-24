load [file join [file dir [info script]] .. libposixsignal0.1.so]

proc keepalive {msec} {
	puts alive
	after $msec [list keepalive $msec]
}

posix::signal trap SIGHUP {puts SIGHUP}

#keepalive 3000

puts [pid]

vwait forever

