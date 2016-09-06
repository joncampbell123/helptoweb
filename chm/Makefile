all: lclinks hhctohtml

lclinks: lclinks.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

hhctohtml: hhctohtml.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

clean:
	rm -f lclinks hhctohtml

distclean: clean

