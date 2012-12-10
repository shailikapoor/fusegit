# TODO Make a configure script which will ask the user to determine the 
# directory where all the git repositories will be kept. User just needs to 
# specify the folder. The repositories will be based on a timestamp, and 
# linked to the folder.

TEST_MOUNT := "test_mount1"

.PHONY: clean cleanall docs $(TEST_MOUNT)

all: fusegit #docs	# THE CREATION OF DOCS IS COMMENTED FOR NOW

fusegit: clean src/fg_fuse.o src/fg_util.o src/fg_git.o
	@echo "Compiling the complete library..."
	@gcc -Wall -Iinclude src/fg.c fg_fuse.o fg_git.o fg_util.o `pkg-config fuse --cflags --libs` -lgit2 -o fusegit
	@echo "Removing the object file..."
	@rm *.o

src/fg_fuse.o:
	@echo "Compiling the fuse code..."
	@gcc -c -Wall -Iinclude src/fg_fuse.c `pkg-config fuse --cflags --libs`

src/fg_git.o:
	@echo "Compiling the git code..."
	@gcc -c -Wall -Iinclude -lgit2 src/fg_git.c `pkg-config fuse --cflags --libs`

src/fg_util.o:
	@echo "Compiling the util code..."
	@gcc -c -Wall -Iinclude src/fg_util.c `pkg-config fuse --cflags --libs`


test: src/fg_util.o
	@echo "Compiling the test functions for util..."
	@gcc -Wall -Iinclude test/test_fusegit.c fg_util.o -o test_fusegit
	@echo "Removing the object files..."
	@rm fg_util.o
	@rm -rf $(TEST_MOUNT)*
	@mkdir $(TEST_MOUNT)
	@./fusegit -m $(TEST_MOUNT)
	@echo "Running test..."
	@./test_fusegit $(TEST_MOUNT)
	@echo
	@echo "Successful"

umount: $(TEST_MOUNT)
	@fusermount -u $(TEST_MOUNT)

docs:
	@echo "Making documentation..."
	@doxygen doxy-config

clean:
	@echo "Removing the fusegit executable >> $(RM) fusegit test_fusegit"
	@$(RM) fusegit test_fusegit
	#@echo "Unmounting test_mount..."
	#@fusermount -q -u test_mount

cleanall: clean
	@echo "Removing the tmp repository >> $(RM) -rf *.repo"
	@$(RM) -rf *.repo
