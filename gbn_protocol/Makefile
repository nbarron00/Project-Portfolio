default: client.c server.c
	gcc client.c -o client
	gcc server.c -o server

dist: client.c server.c README Makefile
	tar -vzcf GBN_rdt_project.tar.gz client.c server.c README Makefile

clean:
	rm -f client server GBN_rdt_project.tar.gz
