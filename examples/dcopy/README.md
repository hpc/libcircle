dcopy (distributed copy)
========================

The purpose of dcopy is to perform a large number of file copy operations (such
as a directory tree with billions of files) across many different nodes in a
cluster. Using a filesystem that benefits from heavy parallel reads and writes
is recommended.
