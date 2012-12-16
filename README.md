[Home](http://agrawal-varun.com)

FuseGit
=======

A FUSE file system with Git as the backend.

Git is known to be a file system in itself. So what is different in 'FuseGit'?
Git is a version control system which has some features of a file system. But
for a normal user it still looks like a version control system, and has a
learning curve. `FuseGit` is primarily a file system. So, if you have the
required dependencies and you install it, you don't need to know about the inner
workings and can use it like any other file system.

How it started
--------------
The project was just an idea when I first learnt about `Git`. The project became
a reality after one year, as part of a course project in the course **Operating
Systems**.

Dependencies
------------
Fuse: v2.9.2
libgit2: v0.17.0

You need to install them, and have them in the path before installing fusegit.

Installation
------------
make

You can run the following command to get instructions:
./fusegit

Please see the [wiki](http://github.com/varun729/fusegit/wiki) to track the progress.

Incremental Goals
-----------------
1. Reliable when FUSE runs in single thread
2. Fast
3. Run a `fsck` type of utility which identifies the useless commits and removes
them from the commit history.
4. Run `git pack` on the repository, and every thing should still run fine.
5. Reliable when FUSE runs in multi thread

### Information
Found another project with similar goals:
[git-fs](http://github.com/patrickhaller/git-fs)





