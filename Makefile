all: server client
server: server.c comm.c
client: client.c comm.c
clean:
	rm -rf server client *~
