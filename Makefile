all: httpserver

httpserver: httpserver.c
	gcc $< -o $@

clean:
	rm httpserver
