FuseGit
=======

A FUSE file system with Git as the backend. Git is known to be a file system in 
itself. So what is different in 'FuseGit'.

FuseGit is a file system first, and has the features of Git at its backend. So 
if you modified a file and want to revert back. There is a backup waiting for 
you. If you keep taking snapshots of your file system, then it becomes 
extremely easy for FuseGit to revert you back in time. But, if you didn't name 
it, it will still keep taking snapshots regularly. And you can choose your time 
from the list provided to you.

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
