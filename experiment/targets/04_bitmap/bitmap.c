// gcc -O0 bitmap.c -o bitmap.elf

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PBMP_WORDS 20
#define PBMP_BITS (PBMP_WORDS * 64)

typedef uint64_t pbmp_t[PBMP_WORDS];

#define PBMP_WORD(bit) ((bit) >> 6)
#define PBMP_SHIFT(bit) ((bit) & 63)
#define PBMP_MASK(bit) (1ULL << PBMP_SHIFT(bit))
#define PBMP_SET(map, bit) ((map)[PBMP_WORD(bit)] |= PBMP_MASK(bit))
#define PBMP_CLEAR(map, bit) ((map)[PBMP_WORD(bit)] &= ~PBMP_MASK(bit))
#define PBMP_TEST(map, bit) (((map)[PBMP_WORD(bit)] & PBMP_MASK(bit)) != 0)

int main(int argc, char **argv) {
    pbmp_t users = {0};
    pbmp_t locked = {0};
    uint64_t seed = argc > 1 ? strtoull(argv[1], 0, 0) : 41;
    uint64_t flags = argc > 2 ? strtoull(argv[2], 0, 0) : 6;
    uint64_t bit, other;
    uint64_t score = 0;
    int i;

    for (i = 0; i < 24; i++) {
        bit = (seed + (uint64_t)i * 19) % PBMP_BITS;
        other = (bit + flags + 11) % PBMP_BITS;

        PBMP_SET(users, bit);

        if (((bit ^ flags) & 3) == 0)
            PBMP_SET(locked, other);
    }

    bit = seed % PBMP_BITS;
    if (PBMP_TEST(users, bit)) {
        score += bit;
        PBMP_CLEAR(users, bit);
    } else {
        PBMP_SET(users, bit);
        score ^= flags;
    }

    other = (seed * 7 + flags) % PBMP_BITS;
    if (PBMP_TEST(locked, other)) {
        PBMP_CLEAR(locked, other);
        score += other * 3;
    } else if (PBMP_TEST(users, other)) {
        PBMP_SET(locked, other);
        score ^= other << 1;
    }

    switch ((unsigned)(flags & 3)) {
    case 0:
        PBMP_SET(users, score % PBMP_BITS);
        break;
    default:
        if (PBMP_TEST(users, flags % PBMP_BITS))
            score += 100;
        break;
    }

    printf("score=%llu users0=%llx locked3=%llx users19=%llx\n",
           (unsigned long long)score,
           (unsigned long long)users[0],
           (unsigned long long)locked[3],
           (unsigned long long)users[19]);
    return (int)(score & 7);
}
