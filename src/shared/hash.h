/* Based off the reference implementation of Tiger, a cryptographically
 * secure 192 bit hash function by Ross Anderson and Eli Biham. More info at:
 * http://www.cs.technion.ac.il/~biham/Reports/Tiger/
 */

#define TIGER_PASSES 3

namespace tiger
{
    extern const int littleendian;
    typedef unsigned long long int chunk;

    union hashval
    {
        uchar bytes[3*8];
        chunk chunks[3];
    };

    extern chunk sboxes[4*256];

    extern void compress(const chunk *str, chunk state[3]);
    extern void gensboxes();
    extern void hash(const uchar *str, int length, hashval &val);
}

