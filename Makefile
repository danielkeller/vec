
all:
	ctags -R --sort=yes --c++-kinds=+p --fields=+iaS --extra=+q .
	cd vec ; make


clean:
	cd vec ; make clean
	rm -f vc
