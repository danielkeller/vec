
all:
	ctags -R --sort=yes --c++-kinds=+p --fields=+iaS --extra=+q .
	cd vec ; make


clean:
	cd vec ; make clean
	rm -f vc

dot:
	dot -Tpng test2.vc.1.dot -o test2.1.png
	dot -Tpng test2.vc.2.dot -o test2.2.png
	dot -Tpng test2.vc.3.dot -o test2.3.png
