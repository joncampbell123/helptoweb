all: catalog

catalog: catalog.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

clean: catalog

distclean: clean

