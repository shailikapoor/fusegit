# TODO Make a configure script which will ask the user to determine the 
# directory where all the git repositories will be kept. User just needs to 
# specify the folder. The repositories will be based on a timestamp, and 
# linked to the folder.

all: fg_fuse #docs

fg_fuse: clean src/fg_git.o
	gcc -Wall -Iinclude src/fg_fuse.c fg_git.o `pkg-config fuse --cflags --libs` -o fg_fuse
	rm fg_git.o

src/fg_git.o:
	gcc -c -Wall -Iinclude -lgit2 src/fg_git.c

docs:
	doxygen doxy-config

clean:
	$(RM) fg_fuse
