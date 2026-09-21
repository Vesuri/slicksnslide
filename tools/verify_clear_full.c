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
    x86_start = 0x3ad92,
    x86_stop = 0x3adb6,
    x86_vga_segment_address = 0x3e89f,
    x86_vga_base = 0xa0000,
    x86_vga_size = 0x10000,
    x86_stack_base = 0x50000,
    m68k_code_base = 0x1000,
    m68k_plane_base = 0x100000,
    m68k_plane_size = 0x40000,
    m68k_stack_base = 0x200000,
};

typedef struct {
    uint8_t *planes;
    unsigned in_count;
    unsigned bad_io;
} VgaState;

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

static void hook_out(uc_engine *uc, uint32_t port, int size,
                     uint32_t value, void *user_data)
{
    (void)uc;
    VgaState *state = user_data;
    if (port != 0x3c4 || size != 2 || value != 0x0f02)
        state->bad_io = 1;
}

static uint32_t hook_in(uc_engine *uc, uint32_t port, int size,
                        void *user_data)
{
    (void)uc;
    VgaState *state = user_data;
    if (port != 0x3da || size != 1) {
        state->bad_io = 1;
        return 0;
    }
    return state->in_count++ == 0 ? 8u : 0u;
}

static void hook_vga_write(uc_engine *uc, uc_mem_type type,
                           uint64_t address, int size, int64_t value,
                           void *user_data)
{
    (void)uc;
    (void)type;
    VgaState *state = user_data;
    if (size != 2 || address < x86_vga_base ||
        address + 2 > x86_vga_base + x86_vga_size) {
        state->bad_io = 1;
        return;
    }
    const size_t offset = (size_t)(address - x86_vga_base);
    for (unsigned plane = 0; plane < 4; ++plane) {
        state->planes[plane * x86_vga_size + offset] = (uint8_t)value;
        state->planes[plane * x86_vga_size + offset + 1] =
            (uint8_t)((uint64_t)value >> 8);
    }
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
    uint8_t *expected = malloc(m68k_plane_size);
    uint8_t *actual = malloc(m68k_plane_size);
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
    check("map x86 VGA", uc_mem_map(x86, x86_vga_base, x86_vga_size,
                                      UC_PROT_ALL));
    check("map x86 stack", uc_mem_map(x86, x86_stack_base, 0x10000,
                                        UC_PROT_ALL));
    const uint8_t vga_segment[2] = {0x00, 0xa0};
    check("write VGA segment", uc_mem_write(x86, x86_vga_segment_address,
                                              vga_segment, 2));

    check("open m68k", uc_open(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, &m68k));
    check("select 68020", uc_ctl_set_cpu_model(m68k, UC_CPU_M68K_M68020));
    check("map m68k code", uc_mem_map(m68k, m68k_code_base, 0x1000,
                                        UC_PROT_ALL));
    check("write m68k code", uc_mem_write(m68k, m68k_code_base, native,
                                            native_bytes));
    check("map m68k planes", uc_mem_map(m68k, m68k_plane_base,
                                          m68k_plane_size, UC_PROT_ALL));
    check("map m68k stack", uc_mem_map(m68k, m68k_stack_base, 0x10000,
                                         UC_PROT_ALL));

    for (unsigned test = 0; test < 4; ++test) {
        for (size_t i = 0; i < m68k_plane_size; ++i)
            expected[i] = actual[i] =
                (uint8_t)(i * 37u + (i >> 16) * 53u + test * 71u + 1u);
        check("seed x86 VGA", uc_mem_write(x86, x86_vga_base, expected,
                                             x86_vga_size));
        check("seed m68k planes", uc_mem_write(m68k, m68k_plane_base,
                                                 actual, m68k_plane_size));

        uint16_t word = 0x3caf;
        check("write x86 DS", uc_reg_write(x86, UC_X86_REG_DS, &word));
        word = 0x5000;
        check("write x86 SS", uc_reg_write(x86, UC_X86_REG_SS, &word));
        word = 0x8000;
        check("write x86 SP", uc_reg_write(x86, UC_X86_REG_SP, &word));
        word = 0x3000;
        check("write x86 CS", uc_reg_write(x86, UC_X86_REG_CS, &word));

        VgaState state = {expected, 0, 0};
        uc_hook in_hook = 0;
        uc_hook out_hook = 0;
        uc_hook memory_hook = 0;
        check("hook IN", uc_hook_add(x86, &in_hook, UC_HOOK_INSN,
                                      (void *)hook_in, &state, 1, 0,
                                      UC_X86_INS_IN));
        check("hook OUT", uc_hook_add(x86, &out_hook, UC_HOOK_INSN,
                                       (void *)hook_out, &state, 1, 0,
                                       UC_X86_INS_OUT));
        check("hook VGA writes", uc_hook_add(
                  x86, &memory_hook, UC_HOOK_MEM_WRITE,
                  (void *)hook_vga_write, &state, x86_vga_base,
                  x86_vga_base + x86_vga_size - 1));
        check("run x86", uc_emu_start(x86, x86_start, x86_stop, 0, 0));
        check("delete IN hook", uc_hook_del(x86, in_hook));
        check("delete OUT hook", uc_hook_del(x86, out_hook));
        check("delete VGA hook", uc_hook_del(x86, memory_hook));

        const int registers[] = {
            UC_M68K_REG_D0, UC_M68K_REG_D1, UC_M68K_REG_D2,
            UC_M68K_REG_D3, UC_M68K_REG_D4, UC_M68K_REG_D5,
            UC_M68K_REG_D6, UC_M68K_REG_D7, UC_M68K_REG_A0,
            UC_M68K_REG_A1, UC_M68K_REG_A2, UC_M68K_REG_A3,
            UC_M68K_REG_A4, UC_M68K_REG_A5, UC_M68K_REG_A6,
        };
        const uint32_t values[] = {
            0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u,
            0x55555555u, 0x66666666u, 0x77777777u, 0x88888888u,
            m68k_plane_base, 0x11112222u, 0x22223333u, 0x33334444u,
            0x44445555u, 0x55556666u, 0x66667777u,
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
        check("read native planes", uc_mem_read(m68k, m68k_plane_base,
                                                  actual, m68k_plane_size));

        int bad_register = -1;
        for (size_t i = 0; i < sizeof(registers) / sizeof(registers[0]); ++i) {
            uint32_t value = 0;
            check("read native register",
                  uc_reg_read(m68k, registers[i], &value));
            if (value != values[i] && bad_register < 0)
                bad_register = (int)i;
        }
        if (state.bad_io || state.in_count != 2 || bad_register >= 0 ||
            memcmp(expected, actual, m68k_plane_size) != 0) {
            size_t difference = 0;
            while (difference < m68k_plane_size &&
                   expected[difference] == actual[difference])
                ++difference;
            fprintf(stderr,
                    "clear case %u failed: io=%u in=%u register=%d diff=%zu\n",
                    test, state.bad_io, state.in_count, bad_register,
                    difference);
            return 1;
        }
    }

    uc_close(x86);
    uc_close(m68k);
    free(runtime);
    free(native);
    free(expected);
    free(actual);
    puts("native VGA full clear: 4 whole-framebuffer cases passed");
    return 0;
}
