#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unicorn/m68k.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>

enum {
    runtime_base = 0x10000,
    runtime_size = 214048,
    x86_start = 0x10d9f,
    x86_stop = 0x10dc4,
    x86_buffer_base = 0x60000,
    x86_buffer_segment = 0x6000,
    x86_buffer_size = 0x10000,
    x86_stack_base = 0x50000,
    m68k_code_base = 0x1000,
    m68k_buffer_base = 0x100000,
    m68k_stack_base = 0x200000,
};

static void check(const char *operation, const uc_err error)
{
    if (error != UC_ERR_OK) {
        fprintf(stderr, "%s: %s\n", operation, uc_strerror(error));
        exit(1);
    }
}

static uint8_t *read_file(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    if (!file || fseek(file, 0, SEEK_END)) {
        perror(path);
        exit(1);
    }
    const long length = ftell(file);
    if (length <= 0 || fseek(file, 0, SEEK_SET)) {
        fprintf(stderr, "%s: invalid file\n", path);
        exit(1);
    }
    uint8_t *bytes = malloc((size_t)length);
    if (!bytes || fread(bytes, 1, (size_t)length, file) != (size_t)length) {
        fprintf(stderr, "%s: cannot read file\n", path);
        exit(1);
    }
    fclose(file);
    *size = (size_t)length;
    return bytes;
}

static void write_le16(uint8_t bytes[2], const uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static uint32_t random_state = 0x46494c4cu;

static uint32_t next_random(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

static void prepare_x86(uc_engine *uc, const uint16_t offset,
                        const uint16_t count, const uint16_t value)
{
    uint8_t stack[12] = {0};
    write_le16(stack + 4, offset);
    write_le16(stack + 6, x86_buffer_segment);
    write_le16(stack + 8, count);
    write_le16(stack + 10, value);
    check("write x86 stack", uc_mem_write(uc, x86_stack_base + 0x8000,
                                            stack, sizeof(stack)));
    uint16_t word = 0x5000;
    check("write x86 SS", uc_reg_write(uc, UC_X86_REG_SS, &word));
    word = 0x8000;
    check("write x86 SP", uc_reg_write(uc, UC_X86_REG_SP, &word));
    word = 0x1000;
    check("write x86 CS", uc_reg_write(uc, UC_X86_REG_CS, &word));
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s runtime.bin native.bin\n", argv[0]);
        return 2;
    }
    size_t runtime_bytes = 0;
    size_t native_bytes = 0;
    uint8_t *runtime = read_file(argv[1], &runtime_bytes);
    uint8_t *native = read_file(argv[2], &native_bytes);
    if (runtime_bytes != runtime_size || native_bytes < 4) {
        fprintf(stderr, "unexpected input size\n");
        return 1;
    }
    uint8_t *expected = malloc(x86_buffer_size);
    uint8_t *actual = malloc(x86_buffer_size);
    if (!expected || !actual) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    uc_engine *x86 = NULL;
    uc_engine *m68k = NULL;
    check("open x86", uc_open(UC_ARCH_X86, UC_MODE_16, &x86));
    check("map x86 runtime", uc_mem_map(x86, runtime_base, 0x40000,
                                          UC_PROT_ALL));
    check("write x86 runtime", uc_mem_write(x86, runtime_base, runtime,
                                              runtime_bytes));
    check("map x86 buffer", uc_mem_map(x86, x86_buffer_base,
                                         x86_buffer_size, UC_PROT_ALL));
    check("map x86 stack", uc_mem_map(x86, x86_stack_base, 0x10000,
                                        UC_PROT_ALL));

    check("open m68k", uc_open(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, &m68k));
    check("select 68020", uc_ctl_set_cpu_model(m68k, UC_CPU_M68K_M68020));
    check("map m68k code", uc_mem_map(m68k, m68k_code_base, 0x1000,
                                        UC_PROT_ALL));
    check("write m68k code", uc_mem_write(m68k, m68k_code_base, native,
                                            native_bytes));
    check("map m68k buffer", uc_mem_map(m68k, m68k_buffer_base,
                                          x86_buffer_size, UC_PROT_ALL));
    check("map m68k stack", uc_mem_map(m68k, m68k_stack_base, 0x10000,
                                         UC_PROT_ALL));

    static const struct {
        uint16_t offset;
        uint16_t count;
        uint16_t value;
    } edges[] = {
        {0, 0, 0}, {0, 1, 0xff}, {1, 1, 0x1234}, {1, 7, 0xabcd},
        {2, 4, 0x0080}, {0, 0xffff, 0x0100}, {0x7fff, 0x8000, 0x55aa},
    };

    for (unsigned test = 0; test < 256; ++test) {
        uint16_t offset;
        uint16_t count;
        uint16_t value;
        if (test < sizeof(edges) / sizeof(edges[0])) {
            offset = edges[test].offset;
            count = edges[test].count;
            value = edges[test].value;
        } else {
            offset = (uint16_t)next_random();
            uint32_t available = 0x10000u - offset;
            if (available > 0xffffu)
                available = 0xffffu;
            count = (uint16_t)(next_random() % (available + 1u));
            value = (uint16_t)next_random();
        }
        for (size_t i = 0; i < x86_buffer_size; ++i)
            expected[i] = actual[i] =
                (uint8_t)(i * 29u + test * 47u + (i >> 8));
        check("seed x86 buffer", uc_mem_write(x86, x86_buffer_base,
                                                expected, x86_buffer_size));
        check("seed native buffer", uc_mem_write(m68k, m68k_buffer_base,
                                                   actual, x86_buffer_size));
        prepare_x86(x86, offset, count, value);
        check("run x86", uc_emu_start(x86, x86_start, x86_stop, 0, 0));
        check("read x86 buffer", uc_mem_read(x86, x86_buffer_base,
                                               expected, x86_buffer_size));

        const int registers[] = {
            UC_M68K_REG_D0, UC_M68K_REG_D1, UC_M68K_REG_D2,
            UC_M68K_REG_D3, UC_M68K_REG_D4, UC_M68K_REG_D5,
            UC_M68K_REG_D6, UC_M68K_REG_D7, UC_M68K_REG_A0,
            UC_M68K_REG_A1, UC_M68K_REG_A2, UC_M68K_REG_A3,
            UC_M68K_REG_A4, UC_M68K_REG_A5, UC_M68K_REG_A6,
        };
        const uint32_t values[] = {
            0x11110000u | count, 0x22220000u | value,
            0x33333333u, 0x44444444u, 0x55555555u, 0x66666666u,
            0x77777777u, 0x88888888u, m68k_buffer_base + offset,
            0x11112222u, 0x22223333u, 0x33334444u, 0x44445555u,
            0x55556666u, 0x66667777u,
        };
        for (size_t i = 0; i < sizeof(registers) / sizeof(registers[0]); ++i)
            check("write native register",
                  uc_reg_write(m68k, registers[i], &values[i]));
        uint32_t stack = m68k_stack_base + 0x8000;
        check("write native A7", uc_reg_write(m68k, UC_M68K_REG_A7,
                                                &stack));
        check("run native", uc_emu_start(m68k, m68k_code_base,
                                           m68k_code_base + native_bytes - 2,
                                           0, 0));
        check("read native buffer", uc_mem_read(m68k, m68k_buffer_base,
                                                  actual, x86_buffer_size));
        int bad_register = -1;
        for (size_t i = 0; i < sizeof(registers) / sizeof(registers[0]); ++i) {
            uint32_t register_value = 0;
            check("read native register",
                  uc_reg_read(m68k, registers[i], &register_value));
            if (register_value != values[i] && bad_register < 0)
                bad_register = (int)i;
        }
        if (bad_register >= 0 || memcmp(expected, actual, x86_buffer_size)) {
            size_t difference = 0;
            while (difference < x86_buffer_size &&
                   expected[difference] == actual[difference])
                ++difference;
            fprintf(stderr,
                    "fill case %u failed: offset=%04x count=%04x value=%04x "
                    "register=%d difference=%zu\n",
                    test, offset, count, value, bad_register, difference);
            return 1;
        }
    }

    uc_close(x86);
    uc_close(m68k);
    free(runtime);
    free(native);
    free(expected);
    free(actual);
    puts("native byte fill: 256 complete-buffer cases passed");
    return 0;
}
