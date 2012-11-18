
all:
	ctags -R --sort=yes --c++-kinds=+p --fields=+iaS --extra=+q .
	cd vec ; make -j `cat /proc/cpuinfo | grep processor | wc -l`

clean:
	cd vec ; make clean
	rm -f vc

dot:
	dot -Tjpg test2.vc.1.dot -o test2.1.jpg
	dot -Tjpg test2.vc.2.dot -o test2.2.jpg
	dot -Tjpg test2.vc.3.dot -o test2.3.jpg
	dot -Tjpg test2.vc.4.dot -o test2.4.jpg
