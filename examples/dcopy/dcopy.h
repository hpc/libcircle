#ifndef DCOPY_H
#define DCOPY_H

#include <unistd.h>

#ifndef DCOPY_FILECOPY_BUFFER_SIZE
    #define DCOPY_FILECOPY_BUFFER_SIZE (64 * 1024)
#endif

void DCOPY_block( int fd, int event );

int  DCOPY_copy_data_buffer( int fdin, int fdout, void *buf, size_t bufsize );
int  DCOPY_copy_data       ( int fdin, int fdout );

int  DCOPY_open_infile ( char *infile  );
int  DCOPY_open_outfile( char *outfile, int fdin );

#endif /* DCOPY_H */
