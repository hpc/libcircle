dstat (distributed stat)
========================

This is an example for a distributed stat() using libcircle. The purpose is to
walk a large file tree which is mounted on several nodes in a cluster. Then,
as the files are stat()ed, the results are stored in a database.
