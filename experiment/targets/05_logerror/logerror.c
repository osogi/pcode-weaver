// gcc -O2 logerror.c -o logerror.elf

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static int g_log_level = 2;

#if defined(__GNUC__) || defined(__clang__)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif

#define RETURN_IF_ERROR(call, level, text)                                      \
    do {                                                                        \
        int _rc = (call);                                                       \
        if (_rc < 0) {                                                          \
            if (g_log_level < (level))                                          \
                fprintf(stderr, "error: %s: %d\n", (text), _rc);               \
            return _rc;                                                         \
        }                                                                       \
    } while (0)

static NOINLINE int open_store(uint64_t id, uint64_t salt) {
    if (((id ^ salt) & 7) == 0)
        return -10;
    return 0;
}

static NOINLINE int read_record(uint64_t id, uint64_t salt, uint64_t *value) {
    if (((id + salt) & 3) == 3)
        return -20;
    *value = (id * 17 + 5) ^ salt;
    return 0;
}

static NOINLINE int write_audit(uint64_t id, uint64_t salt, uint64_t value) {
    if (((id ^ value ^ salt) & 15) == 9)
        return -30;
    return 0;
}

static NOINLINE int flush_store(uint64_t flags, uint64_t salt) {
    if (((flags ^ salt) & 0x40) != 0)
        return -40;
    return 0;
}

static int handle_request(uint64_t id, uint64_t flags, uint64_t salt) {
    uint64_t value = 0;
    int i;

    RETURN_IF_ERROR(open_store(id, salt), 3, "open_store");

    if ((flags & 1) != 0) {
        RETURN_IF_ERROR(read_record(id, salt, &value), 2, "read_record");
    } else {
        RETURN_IF_ERROR(read_record(id + 1, salt, &value), 4, "read_record_fallback");
        value ^= flags;
    }

    for (i = 0; i < 3; i++) {
        uint64_t next = value + (uint64_t)i;

        if ((next & 1) == 0)
            RETURN_IF_ERROR(write_audit(id + i, salt, next), 4, "write_audit");
        else if ((flags & 2) != 0)
            RETURN_IF_ERROR(write_audit(id + i, salt, next ^ 0xff), 5, "write_audit_masked");
    }

    switch ((unsigned)(flags & 3)) {
    case 0:
        RETURN_IF_ERROR(flush_store(flags, salt), 5, "flush_store");
        break;
    case 1:
        RETURN_IF_ERROR(flush_store(flags ^ 0x10, salt), 3, "flush_store_alt");
        break;
    case 2:
        if (value > 100)
            RETURN_IF_ERROR(write_audit(id, salt, value - 1), 3, "final_audit");
        break;
    default:
        RETURN_IF_ERROR(read_record(id ^ flags, salt, &value), 2, "reload_record");
        RETURN_IF_ERROR(flush_store(flags | (value & 0x40), salt), 5, "flush_reload");
        break;
    }

    return (int)(value & 31);
}

int main(int argc, char **argv) {
    uint64_t id = argc > 1 ? strtoull(argv[1], 0, 0) : 19;
    uint64_t flags = argc > 2 ? strtoull(argv[2], 0, 0) : 3;
    uint64_t salt = argc > 3 ? strtoull(argv[3], 0, 0) : 11;
    int rc;

    if (argc > 4)
        g_log_level = atoi(argv[4]);

    rc = handle_request(id, flags, salt);
    if (rc < 0) {
        if (g_log_level < 4)
            fprintf(stderr, "request failed: %d\n", rc);
        return 1;
    }

    printf("request=%llu result=%d\n", (unsigned long long)id, rc);
    return 0;
}
