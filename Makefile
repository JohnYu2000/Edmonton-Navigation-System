# Name: John Yu
# ID: 1615007
# CMPUT 275, Winter 2021

cc = g++
gt = gnome-terminal --
serverObjs = server.o dijkstra.o digraph.o
pipes = inpipe outpipe
serverExec = ./server/server
clientExec = ./client/client
port = 8888
ip = 127.0.0.1

all: server client
	${gt} ${serverExec} ${port}
	${gt} ${clientExec} ${port} ${ip}
	${gt} ./plotter

server: ${serverObjs}
	${cc} -o ${serverExec} ${serverObjs}

server.o: ./server/server.cpp
	${cc} -c ./server/server.cpp

dijkstra.o: ./server/dijkstra.cpp
	${cc} -c ./server/dijkstra.cpp

digraph.o: ./server/digraph.cpp
	${cc} -c ./server/digraph.cpp

client: client.o
	${cc} -o ${clientExec} client.o

client.o: ./client/client.cpp
	${cc} -c ./client/client.cpp

clean:
	rm -f ${serverExec} ${serverObjs} ${clientExec} client.o ${pipes}