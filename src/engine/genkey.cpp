#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include "pch.h"
#include "tools.h"
#include "crypto.h"
#include "hash.h"
#include <enet/time.h>

void genkey(char *s)
{
    tiger::hashval hash;
    tiger::hash((uchar *)s, (int)strlen(s), hash);
    bigint<8*sizeof(hash.bytes)/BI_DIGIT_BITS> privkey;
    memcpy(privkey.digits, hash.bytes, sizeof(privkey.digits));
    privkey.len = 8*sizeof(hash.bytes)/BI_DIGIT_BITS;
    privkey.shrink();
    printf("private key: "); privkey.print(stdout); putchar('\n');

    ecjacobian c(ecjacobian::base);
    c.mul(privkey);
    c.normalize();
    vector<char> pubkey;
    c.print(pubkey);
    pubkey.add('\0');
    printf("public key: %s\n", pubkey.getbuf());
}

int main(int argc, char **argv)
{
    if(argc < 2) return EXIT_FAILURE;
    genkey(argv[1]);
    return EXIT_SUCCESS;
}

