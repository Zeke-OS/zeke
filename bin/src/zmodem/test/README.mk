socat -d -d pty pty
faketty /dev/pts/18 rz
faketty /dev/pts/19 sz --zmodem file
