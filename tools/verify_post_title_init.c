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
    x86_start = 0x26372,
    x86_stop = 0x263e6,
    x86_data_segment = 0x3cbf,
    x86_mode = 0x3fc12,
    x86_seed = 0x3fc1a,
    x86_flags = 0x3dc5f,
    x86_seeds = 0x417e6,
    x86_grid = 0x4366a,
    x86_stack = 0x50000,
    m68k_code = 0x1000,
    m68k_data = 0x10000,
    m68k_stack = 0x20000,
};

static void check(const char *operation, uc_err error)
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

static uint32_t random_state = 0x534c4943u;

static uint32_t next_random(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
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

    uc_engine *x86 = NULL;
    uc_engine *m68k = NULL;
    check("open x86", uc_open(UC_ARCH_X86, UC_MODE_16, &x86));
    check("map x86 runtime", uc_mem_map(x86, runtime_base, 0x40000,
                                          UC_PROT_ALL));
    check("write x86 runtime", uc_mem_write(x86, runtime_base + 0x100,
                                              runtime, runtime_bytes));
    check("map x86 stack", uc_mem_map(x86, x86_stack, 0x10000, UC_PROT_ALL));
    check("open m68k", uc_open(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, &m68k));
    check("select 68020", uc_ctl_set_cpu_model(m68k, UC_CPU_M68K_M68020));
    check("map m68k code", uc_mem_map(m68k, m68k_code, 0x1000, UC_PROT_ALL));
    check("write m68k code", uc_mem_write(m68k, m68k_code, native,
                                            native_bytes));
    check("map m68k data", uc_mem_map(m68k, m68k_data, 0x10000,
                                        UC_PROT_ALL));
    check("map m68k stack", uc_mem_map(m68k, m68k_stack, 0x10000,
                                         UC_PROT_ALL));

    for (unsigned test = 0; test < 256; ++test) {
        uint8_t flags[13];
        uint8_t x86_grid_bytes[104];
        uint8_t native_grid_bytes[104];
        uint8_t x86_seed_bytes[8];
        uint8_t native_seed_bytes[8];
        for (size_t i = 0; i < sizeof(flags); ++i)
            flags[i] = (uint8_t)next_random();
        for (size_t i = 0; i < sizeof(x86_grid_bytes); ++i)
            x86_grid_bytes[i] = native_grid_bytes[i] = (uint8_t)next_random();
        for (size_t i = 0; i < sizeof(x86_seed_bytes); ++i)
            x86_seed_bytes[i] = native_seed_bytes[i] = (uint8_t)next_random();
        const uint16_t mode = test < 4 ? (uint16_t)test :
                                               (uint16_t)next_random();
        const uint16_t seed = (uint16_t)next_random();
        const uint8_t mode_le[2] = {(uint8_t)mode, (uint8_t)(mode >> 8)};
        const uint8_t seed_le[2] = {(uint8_t)seed, (uint8_t)(seed >> 8)};

        check("write x86 mode", uc_mem_write(x86, x86_mode, mode_le, 2));
        check("write x86 seed", uc_mem_write(x86, x86_seed, seed_le, 2));
        check("write x86 flags", uc_mem_write(x86, x86_flags, flags, 13));
        check("write x86 grid", uc_mem_write(x86, x86_grid,
                                               x86_grid_bytes, 104));
        check("write x86 seeds", uc_mem_write(x86, x86_seeds,
                                                x86_seed_bytes, 8));
        uint16_t word = x86_data_segment;
        check("write x86 DS", uc_reg_write(x86, UC_X86_REG_DS, &word));
        word = 0x5000;
        check("write x86 SS", uc_reg_write(x86, UC_X86_REG_SS, &word));
        word = 0x2000;
        check("write x86 CS", uc_reg_write(x86, UC_X86_REG_CS, &word));
        word = 0x8100;
        check("write x86 BP", uc_reg_write(x86, UC_X86_REG_BP, &word));
        word = 0x8000;
        check("write x86 SP", uc_reg_write(x86, UC_X86_REG_SP, &word));
        check("run x86", uc_emu_start(x86, x86_start, x86_stop, 0, 0));
        check("read x86 grid", uc_mem_read(x86, x86_grid,
                                             x86_grid_bytes, 104));
        check("read x86 seeds", uc_mem_read(x86, x86_seeds,
                                              x86_seed_bytes, 8));
        for (size_t i = 0; i < sizeof(x86_grid_bytes); i += 2) {
            const uint8_t low = x86_grid_bytes[i];
            x86_grid_bytes[i] = x86_grid_bytes[i + 1];
            x86_grid_bytes[i + 1] = low;
        }
        for (size_t i = 0; i < sizeof(x86_seed_bytes); i += 2) {
            const uint8_t low = x86_seed_bytes[i];
            x86_seed_bytes[i] = x86_seed_bytes[i + 1];
            x86_seed_bytes[i + 1] = low;
        }

        const uint32_t grid_address = m68k_data + 0x100;
        const uint32_t flags_address = m68k_data + 0x200;
        const uint32_t seeds_address = m68k_data + 0x300;
        check("write native grid", uc_mem_write(m68k, grid_address,
                                                  native_grid_bytes, 104));
        check("write native flags", uc_mem_write(m68k, flags_address,
                                                   flags, 13));
        check("write native seeds", uc_mem_write(m68k, seeds_address,
                                                   native_seed_bytes, 8));
        const int registers[] = {UC_M68K_REG_A0, UC_M68K_REG_A1,
                                 UC_M68K_REG_A2, UC_M68K_REG_D0,
                                 UC_M68K_REG_D1, UC_M68K_REG_A7};
        const uint32_t values[] = {grid_address, flags_address, seeds_address,
                                   mode, seed, m68k_stack + 0x8000};
        for (size_t i = 0; i < sizeof(registers) / sizeof(registers[0]); ++i)
            check("write native register",
                  uc_reg_write(m68k, registers[i], &values[i]));
        check("run native", uc_emu_start(m68k, m68k_code,
                                           m68k_code + native_bytes - 2,
                                           0, 0));
        check("read native grid", uc_mem_read(m68k, grid_address,
                                                native_grid_bytes, 104));
        check("read native seeds", uc_mem_read(m68k, seeds_address,
                                                 native_seed_bytes, 8));
        if (memcmp(x86_grid_bytes, native_grid_bytes, 104) ||
            memcmp(x86_seed_bytes, native_seed_bytes, 8)) {
            size_t grid_difference = 0;
            while (grid_difference < 104 &&
                   x86_grid_bytes[grid_difference] ==
                       native_grid_bytes[grid_difference])
                ++grid_difference;
            size_t seed_difference = 0;
            while (seed_difference < 8 &&
                   x86_seed_bytes[seed_difference] ==
                       native_seed_bytes[seed_difference])
                ++seed_difference;
            fprintf(stderr, "post-title init case %u failed: mode=%04x "
                            "seed=%04x grid-diff=%zu seed-diff=%zu",
                    test, mode, seed, grid_difference, seed_difference);
            if (grid_difference < 104)
                fprintf(stderr, " grid=%02x/%02x", x86_grid_bytes[grid_difference],
                        native_grid_bytes[grid_difference]);
            if (seed_difference < 8)
                fprintf(stderr, " seeds=%02x/%02x", x86_seed_bytes[seed_difference],
                        native_seed_bytes[seed_difference]);
            fputc('\n', stderr);
            return 1;
        }
    }

    uc_close(x86);
    uc_close(m68k);
    free(runtime);
    free(native);
    puts("native post-title init: 256 x86-versus-68020 cases passed");
    return 0;
}
