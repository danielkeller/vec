
all:
	cd vec ; make
	ctags -R --sort=yes --c++-kinds=+p --fields=+iaS --extra=+q .


clean:
	cd vec ; make clean
	rm -f vc
