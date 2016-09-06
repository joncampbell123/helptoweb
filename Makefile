all:
	make -C chm
	make -C mshc

clean:
	make -C chm clean
	make -C mshc clean

distclean:
	make -C chm distclean
	make -C mshc distclean

