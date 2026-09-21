#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <unicorn/m68k.h>
#include <unicorn/unicorn.h>

enum {
    code_base = 0x1000,
    stack_base = 0x2000,
    stack_size = 0x1000,
};

static void check(const char *operation, const uc_err error)
{
    if (error != UC_ERR_OK) {
        fprintf(stderr, "%s: %s\n", operation, uc_strerror(error));
        exit(1);
    }
}

static uint8_t expected_action(const uint16_t scan)
{
    switch (scan) {
    case 0x01:
    case 0x44:
        return 1;
    case 0x1c:
    case 0x1d:
    case 0x39:
        return 2;
    case 0x3b:
        return 3;
    case 0x43:
        return 4;
    case 0x58:
        return 5;
    default:
        return 0;
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s sui-title-dispatch.bin\n", argv[0]);
        return 2;
    }
    FILE *input = fopen(argv[1], "rb");
    if (!input || fseek(input, 0, SEEK_END)) {
        perror(argv[1]);
        return 1;
    }
    const long length = ftell(input);
    if (length < 4 || fseek(input, 0, SEEK_SET)) {
        fprintf(stderr, "%s: invalid native routine\n", argv[1]);
        return 1;
    }
    uint8_t *code = malloc((size_t)length);
    if (!code || fread(code, 1, (size_t)length, input) != (size_t)length) {
        fprintf(stderr, "%s: cannot read native routine\n", argv[1]);
        return 1;
    }
    fclose(input);

    uc_engine *uc = NULL;
    check("open m68k", uc_open(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, &uc));
    check("select 68020", uc_ctl_set_cpu_model(uc, UC_CPU_M68K_M68020));
    check("map code", uc_mem_map(uc, code_base, 0x1000, UC_PROT_ALL));
    check("write code", uc_mem_write(uc, code_base, code, (size_t)length));
    check("map stack", uc_mem_map(uc, stack_base, stack_size, UC_PROT_ALL));

    unsigned cases = 0;
    for (uint32_t scan = 0; scan <= 0xffffu; ++scan) {
        uint32_t value = 0xa5a50000u | scan;
        check("write D0", uc_reg_write(uc, UC_M68K_REG_D0, &value));
        value = stack_base + stack_size / 2;
        check("write A7", uc_reg_write(uc, UC_M68K_REG_A7, &value));
        check("run dispatch",
              uc_emu_start(uc, code_base, code_base + (uint32_t)length - 2,
                           0, 0));
        check("read D0", uc_reg_read(uc, UC_M68K_REG_D0, &value));
        if (value != expected_action((uint16_t)scan)) {
            fprintf(stderr, "title dispatch failed: scan=%04x expected=%u "
                            "actual=%08x\n",
                    (unsigned)scan, expected_action((uint16_t)scan), value);
            return 1;
        }
        ++cases;
    }
    uc_close(uc);
    free(code);
    printf("native title dispatch: %u word-valued scan-code cases passed\n",
           cases);
    return 0;
}
