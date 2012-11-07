


fg_fuse: clean src/fg_git.o
	gcc -Wall -Iinclude src/fg_fuse.c fg_git.o `pkg-config fuse --cflags --libs` -o fg_fuse
	rm fg_git.o

src/fg_git.o:
	gcc -c -Wall -Iinclude -lgit2 src/fg_git.c

clean:
	$(RM) fg_fuse
