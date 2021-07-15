LIB_SRC:=api_fs.c

# this is the simplest compilation, just demonstration
# you can change it to SharedLibrary, for example

util: util.c ${LIB_SRC} error.h Makefile
	gcc -I../include  -Wall -Werror -o $@ util.c ${LIB_SRC}
install: util
	sudo install $< /usr/local/bin/niagara_util
uninstall:
	sudo rm -f /usr/local/bin/niagara_util
clean:
	make -C examples clean
	rm -f util
