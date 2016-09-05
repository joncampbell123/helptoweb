all: catalog htmlxlate

htmlxlate: htmlxlate.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

catalog: catalog.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

clean:
	rm -f catalog htmlxlate

distclean: clean

