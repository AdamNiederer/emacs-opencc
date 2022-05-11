CFLAGS = -Ofast -D_FORTIFY_SOURCE=2 -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-function
LIBS = -lopencc

emacs-opencc.so: emacs-opencc.o
	ld -shared -o $@ $< $(LIBS)
	strip emacs-opencc.so

emacs-opencc.o: emacs-opencc.c
	gcc $(CFLAGS) -fpic -c -o $@ $<

.PHONY: clean
clean:
	rm -f *.o *.so
