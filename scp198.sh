#!/bin/bash
IP=192.168.1.110
PASSWD=123456
content=$(cat <<!
	spawn scp -r root@$IP:/data/dzpk/.
	send "yes\n"
	expect password:
	send "$PASSWD\n"
	expect eof
	)
echo "$content"|expect
