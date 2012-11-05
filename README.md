FuseGit
=======

A FUSE file system with Git as the backend. Git is known to be a file system 
in itself. So what is different in 'FuseGit'.

FuseGit is a file system first, and has the features of Git at its backend. So 
if you modified a file and want to revert back. There is a backup waiting for 
you. If you keep taking snapshots of your file system, then it becomes 
extremely easy for FuseGit to revert you back in time. But, if you didn't name 
it, it will still keep taking snapshots regularly. And you can choose your 
time from the list provided to you.

All this sounds non-magical if you are familiar with Git or any other version 
control system. But for people who don't want the added complexity of playing 
with a VCS, FuseGit will give a ready to use recipe. Just plug it in. And have 
regular backups of your file system, without worrying about too much memory 
being consumed.

Cleaning up
-----------
Have you ever looked at the Linux repository? If you do a clone of the 
repository, it will be several multiples of the actual filesize. This is 
because they are storing every possible content. This is a common issue with 
any VCS, it grows in size with time more than the actual content. This can be 
handled if you take care to clean up the history markers at regular intervals.

Implementation
--------------
`FUSE` is in the name, so we are obviously using it. This means that the file 
system is in user space. This is a prototype version and we want to keep 
things simple.

`libgit2` will be used to get the Git functionality. This library is still 
under active development. I hope we don't end up requiring a feature which is 
still under development. This is a risk we might have to take right now.

### libgit2
1. Create a repository.
2. If its a new repository, find the global configuration path in the system, 
and apply it to this repository using `git_config_find_global` and `git_config_add_file_ondisk`.
3. To add a new file to the repository:
	1. Create a blob from the file.
	2. Create a signature for the author and committer.
	3. Create a treebuilder, and add the blob to the tree.
	4. Multiple files could be added to this treebuilder.
	5. Once the tree is obtained, commit it to the repository.

### FUSE
TODO
