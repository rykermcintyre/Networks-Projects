all: pong netpong

netpong: netpong.c
	gcc netpong.c -lncurses -lpthread -o netpong

pong: pong.c
	gcc pong.c -lncurses -lpthread -o pong

clean:
	rm -f pong netpong
