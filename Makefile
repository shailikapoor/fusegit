# TODO Make a configure script which will ask the user to determine the 
# directory where all the git repositories will be kept. User just needs to 
# specify the folder. The repositories will be based on a timestamp, and 
# linked to the folder.

all: fusegit #docs	# THE CREATION OF DOCS IS COMMENTED FOR NOW

fusegit: clean src/fg_git.o src/fg_util.o
	@echo "Compiling the complete library..."
	@gcc -Wall -Iinclude src/fg_fuse.c fg_git.o fg_util.o `pkg-config fuse --cflags --libs` -lgit2 -o fusegit
	@echo "Removing the object file..."
	@rm fg_git.o fg_util.o

src/fg_git.o:
	@echo "Compiling the git code..."
	@gcc -c -Wall -Iinclude -lgit2 src/fg_git.c

src/fg_util.o:
	@echo "Compiling the util code..."
	@gcc -c -Wall -Iinclude src/fg_util.c

test: src/fg_util.o
	@echo "Compiling the test functions for util..."
	@gcc -Wall -Iinclude test/fg_test.c fg_util.o -o test_fusegit
	@echo "Removing the object files..."
	@rm fg_util.o
	@echo "Running test..."
	@./test_fusegit
	@echo
	@echo "Successful"

docs:
	@echo "Making documentation..."
	@doxygen doxy-config

clean:
	@echo "Removing the fusegit executable >> $(RM) fusegit"
	@$(RM) fusegit test_fusegit

cleanall: clean
	@echo "Removing the tmp repository >> $(RM) -rf *.repo"
	@$(RM) -rf *.repo
