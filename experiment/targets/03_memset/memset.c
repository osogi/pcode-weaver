// gcc -O2 memset.c  -o memset.elf

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static inline void stosq_fill(uint64_t *dest, uint64_t value, uint64_t qwords) {
    __asm__ __volatile__(
        "rep stosq"
        : "+D"(dest), "+c"(qwords)
        : "a"(value)
        : "memory"
    );
}

struct Session {
    uint64_t request_id;
    uint64_t user_id;
    uint64_t flags;
    uint64_t quota;
    uint64_t scratch[8];
};

int main(int argc, char **argv) {
    struct Session session;
    uint64_t cache_tags[8];
    uint64_t packet[10];
    uint64_t audit[6];
    uint64_t seed = argc > 1 ? strtoull(argv[1], 0, 0) : 42;
    uint64_t mode = seed & 3;
    uint64_t score = 0;
    int i;

    stosq_fill((uint64_t *)&session, 0, sizeof(session) / sizeof(uint64_t));
    session.request_id = seed;
    session.user_id = 1000 + (seed % 17);
    session.quota = 4096;

    if ((seed & 1) == 0) {
        stosq_fill(cache_tags, UINT64_MAX, 8);
        session.flags |= 1;
    } else {
        stosq_fill(cache_tags, 0x1111111111111111ULL * (seed & 15), 4);
        stosq_fill(cache_tags + 4, 0xeeeeeeeeeeeeeeeeULL, 4);
        session.flags |= 2;
    }

    for (i = 0; i < 3; i++) {
        uint64_t fill = 0xccccccccccccccccULL ^ (seed + i);

        if (((seed >> i) & 1) != 0)
            stosq_fill(packet + (i * 3), fill, 3);
        else
            stosq_fill(packet + (i * 3), 0, 3);

        packet[i * 3] = 0x5041434b00000000ULL | (uint64_t)i;
        score ^= packet[i * 3 + 1] + cache_tags[i];
    }

    switch (mode) {
    case 0:
        stosq_fill(audit, 0, 6);
        audit[0] = session.request_id;
        break;
    case 1:
        stosq_fill(audit, 0xababababababababULL, 6);
        audit[5] = session.user_id;
        break;
    case 2:
        stosq_fill(audit, score, 3);
        stosq_fill(audit + 3, ~score, 3);
        break;
    default:
        stosq_fill(audit, 0xdead000000000000ULL | seed, 6);
        if (session.quota > 1024)
            stosq_fill(session.scratch, seed, 8);
        break;
    }

    if ((audit[0] ^ audit[5]) == seed)
        stosq_fill(packet + 6, 0x7777777777777777ULL, 4);

    printf("%llu %llu %llx %llx %llx\n",
           (unsigned long long)session.user_id,
           (unsigned long long)session.flags,
           (unsigned long long)cache_tags[0],
           (unsigned long long)packet[6],
           (unsigned long long)audit[5]);
    return (int)(score & 7);
}
