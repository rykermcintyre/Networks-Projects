File Names:
	/clientdir/client.c
	/clientdir/Makefile
	/serverdir/server.c
	/serverdir/Makefile

Inside both directories run make to get the executables.
To run the server, ./myftpd [Port]
To run the client, ./myftp serverName [Port]

NOTE: If you run LS as your first command, it will segfault upon running the next
command. It is unknown why this happens. Simply run MKDIR or some other utility
before LS to avoid this. Thank you for your time and have a nice week.
