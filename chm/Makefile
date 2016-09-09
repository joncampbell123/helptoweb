all: lclinks xlinks hhctohtml catalog june2010_fix_collapsable_sections

xlinks: xlinks.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

lclinks: lclinks.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

hhctohtml: hhctohtml.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

catalog: catalog.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

june2010_fix_collapsable_sections: june2010_fix_collapsable_sections.cpp
	g++ -o $@ $< `pkg-config libxml-2.0 --cflags` `pkg-config libxml-2.0 --libs`

clean:
	rm -f lclinks hhctohtml catalog xlinks june2010_fix_collapsable_sections

distclean: clean

