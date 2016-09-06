all: lclinks xlinks hhctohtml catalog

xlinks: xlinks.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

lclinks: lclinks.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

hhctohtml: hhctohtml.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

catalog: catalog.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

clean:
	rm -f lclinks hhctohtml catalog xlinks

distclean: clean

