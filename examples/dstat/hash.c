#include "hash.h"

void
dstat_filename_hash(unsigned char* md, const unsigned char* input)
{
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (unsigned char*)input, strlen(input));
    SHA256_Final(md, &ctx);
}

/* EOF */
