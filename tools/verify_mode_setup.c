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
    x86_start = 0x3adb7,
    x86_stop = 0x3aeb4,
    x86_data_segment = 0x3caf,
    x86_state_address = 0x3e84e,
    x86_vga_base = 0xa0000,
    x86_vga_size = 0x10000,
    x86_stack_base = 0x50000,
    x86_stack_segment = 0x5000,
    x86_stack_offset = 0x8000,
    m68k_code_base = 0x1000,
    m68k_plane_base = 0x100000,
    m68k_plane_size = 0x40000,
    m68k_state_base = 0x200000,
    m68k_stack_base = 0x300000,
    native_state_size = 22,
};

typedef struct {
    uint8_t *planes;
    unsigned in_count;
    unsigned int10_count;
    unsigned int33_count;
    unsigned clear_mask_count;
    unsigned bad_hardware;
} HardwareState;

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

static uint16_t read_le16(const uint8_t *bytes)
{
    return (uint16_t)(bytes[0] | (uint16_t)bytes[1] << 8);
}

static void write_be16(uint8_t bytes[2], const uint16_t value)
{
    bytes[0] = (uint8_t)(value >> 8);
    bytes[1] = (uint8_t)value;
}

static void hook_out(uc_engine *uc, uint32_t port, int size,
                     uint32_t value, void *user_data)
{
    (void)uc;
    HardwareState *state = user_data;
    if (port == 0x3c4 && size == 2 && value == 0x0f02)
        ++state->clear_mask_count;
}

static uint32_t hook_in(uc_engine *uc, uint32_t port, int size,
                        void *user_data)
{
    (void)uc;
    HardwareState *state = user_data;
    static const uint8_t retrace_sequence[] = {8, 0, 0, 8};
    if (port == 0x3d5 && size == 1)
        return 0;
    if (port != 0x3da || size != 1) {
        state->bad_hardware = 1;
        return 0;
    }
    const unsigned index = state->in_count++;
    return index < sizeof(retrace_sequence) ? retrace_sequence[index] :
                                              retrace_sequence[index & 3u];
}

static void hook_interrupt(uc_engine *uc, uint32_t number, void *user_data)
{
    HardwareState *state = user_data;
    uint32_t eax = 0;
    uint32_t ebx = 0;
    check("read interrupt AX", uc_reg_read(uc, UC_X86_REG_EAX, &eax));
    if (number == 0x10) {
        if ((eax & 0xffffu) != 0x0013u)
            state->bad_hardware = 1;
        ++state->int10_count;
        return;
    }
    if (number != 0x33) {
        state->bad_hardware = 1;
        return;
    }
    ++state->int33_count;
    switch (eax & 0xffffu) {
    case 0:
        eax = (eax & 0xffff0000u) | 0xffffu;
        ebx = 2;
        check("write mouse AX", uc_reg_write(uc, UC_X86_REG_EAX, &eax));
        check("write mouse BX", uc_reg_write(uc, UC_X86_REG_EBX, &ebx));
        break;
    case 3: {
        uint32_t zero = 0;
        check("write mouse CX", uc_reg_write(uc, UC_X86_REG_ECX, &zero));
        check("write mouse DX", uc_reg_write(uc, UC_X86_REG_EDX, &zero));
        break;
    }
    case 8:
    case 9:
        break;
    default:
        state->bad_hardware = 1;
        break;
    }
}

static void hook_vga_write(uc_engine *uc, uc_mem_type type,
                           uint64_t address, int size, int64_t value,
                           void *user_data)
{
    (void)uc;
    (void)type;
    HardwareState *state = user_data;
    if ((size != 1 && size != 2) || address < x86_vga_base ||
        address + (uint64_t)size > x86_vga_base + x86_vga_size) {
        state->bad_hardware = 1;
        return;
    }
    const size_t offset = (size_t)(address - x86_vga_base);
    for (unsigned plane = 0; plane < 4; ++plane) {
        for (int byte = 0; byte < size; ++byte) {
            state->planes[plane * x86_vga_size + offset + (size_t)byte] =
                (uint8_t)((uint64_t)value >> (byte * 8));
        }
    }
}

static void prepare_x86_stack(uc_engine *uc, const uint16_t mode,
                              const uint16_t virtual_width)
{
    uint8_t stack[8] = {0};
    write_le16(stack + 4, mode);
    write_le16(stack + 6, virtual_width);
    check("write x86 stack", uc_mem_write(
              uc, x86_stack_base + x86_stack_offset, stack, sizeof(stack)));
    uint16_t word = x86_data_segment;
    check("write x86 DS", uc_reg_write(uc, UC_X86_REG_DS, &word));
    word = x86_stack_segment;
    check("write x86 SS", uc_reg_write(uc, UC_X86_REG_SS, &word));
    word = x86_stack_offset;
    check("write x86 SP", uc_reg_write(uc, UC_X86_REG_SP, &word));
    word = 0x3000;
    check("write x86 CS", uc_reg_write(uc, UC_X86_REG_CS, &word));
}

static void extract_expected_state(uc_engine *x86,
                                   uint8_t expected[native_state_size])
{
    uint8_t state[0x58];
    check("read x86 mode state", uc_mem_read(x86, x86_state_address,
                                               state, sizeof(state)));
    write_be16(expected + 0, read_le16(state + 0x00));
    write_be16(expected + 2, read_le16(state + 0x05));
    write_be16(expected + 4, read_le16(state + 0x07));
    write_be16(expected + 6, read_le16(state + 0x1f));
    write_be16(expected + 8, read_le16(state + 0x1d));
    write_be16(expected + 10, read_le16(state + 0x1b));
    write_be16(expected + 12, read_le16(state + 0x21));
    write_be16(expected + 14, read_le16(state + 0x25));
    write_be16(expected + 16, read_le16(state + 0x23));
    write_be16(expected + 18, state[0x02]);
    write_be16(expected + 20, state[0x57]);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr,
                "usage: %s runtime.bin clear.bin mode-setup.bin\n", argv[0]);
        return 2;
    }
    size_t runtime_bytes = 0;
    size_t clear_bytes = 0;
    size_t mode_bytes = 0;
    uint8_t *runtime = read_file(argv[1], &runtime_bytes);
    uint8_t *clear = read_file(argv[2], &clear_bytes);
    uint8_t *mode = read_file(argv[3], &mode_bytes);
    if (runtime_bytes != runtime_size || clear_bytes < 4 ||
        mode_bytes <= clear_bytes || memcmp(mode, clear, clear_bytes)) {
        fprintf(stderr, "unexpected input layout\n");
        return 1;
    }
    uint8_t *expected_planes = malloc(m68k_plane_size);
    uint8_t *actual_planes = malloc(m68k_plane_size);
    if (!expected_planes || !actual_planes) {
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

    check("open m68k", uc_open(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, &m68k));
    check("select 68020", uc_ctl_set_cpu_model(m68k, UC_CPU_M68K_M68020));
    check("map m68k code", uc_mem_map(m68k, m68k_code_base, 0x1000,
                                        UC_PROT_ALL));
    check("write m68k code", uc_mem_write(m68k, m68k_code_base, mode,
                                            mode_bytes));
    check("map m68k planes", uc_mem_map(m68k, m68k_plane_base,
                                          m68k_plane_size, UC_PROT_ALL));
    check("map m68k state", uc_mem_map(m68k, m68k_state_base, 0x10000,
                                         UC_PROT_ALL));
    check("map m68k stack", uc_mem_map(m68k, m68k_stack_base, 0x10000,
                                         UC_PROT_ALL));

    static const uint16_t widths[] = {
        0, 1, 319, 320, 321, 399, 400, 401, 2048,
    };
    for (unsigned test = 0; test < sizeof(widths) / sizeof(widths[0]); ++test) {
        for (size_t i = 0; i < m68k_plane_size; ++i)
            expected_planes[i] = actual_planes[i] =
                (uint8_t)(i * 41u + (i >> 16) * 67u + test * 23u + 1u);
        check("seed x86 VGA", uc_mem_write(x86, x86_vga_base,
                                             expected_planes, x86_vga_size));
        check("seed m68k planes", uc_mem_write(m68k, m68k_plane_base,
                                                 actual_planes,
                                                 m68k_plane_size));
        prepare_x86_stack(x86, 0, widths[test]);

        HardwareState hardware = {expected_planes, 0, 0, 0, 0, 0};
        uc_hook in_hook = 0;
        uc_hook out_hook = 0;
        uc_hook interrupt_hook = 0;
        uc_hook memory_hook = 0;
        check("hook IN", uc_hook_add(x86, &in_hook, UC_HOOK_INSN,
                                      (void *)hook_in, &hardware, 1, 0,
                                      UC_X86_INS_IN));
        check("hook OUT", uc_hook_add(x86, &out_hook, UC_HOOK_INSN,
                                       (void *)hook_out, &hardware, 1, 0,
                                       UC_X86_INS_OUT));
        check("hook interrupts", uc_hook_add(
                  x86, &interrupt_hook, UC_HOOK_INTR,
                  (void *)hook_interrupt, &hardware, 1, 0));
        check("hook VGA writes", uc_hook_add(
                  x86, &memory_hook, UC_HOOK_MEM_WRITE,
                  (void *)hook_vga_write, &hardware, x86_vga_base,
                  x86_vga_base + x86_vga_size - 1));
        check("run x86", uc_emu_start(x86, x86_start, x86_stop, 0, 0));
        check("delete IN", uc_hook_del(x86, in_hook));
        check("delete OUT", uc_hook_del(x86, out_hook));
        check("delete interrupts", uc_hook_del(x86, interrupt_hook));
        check("delete VGA", uc_hook_del(x86, memory_hook));

        uint8_t expected_state[native_state_size];
        uint8_t actual_state[native_state_size];
        extract_expected_state(x86, expected_state);
        memset(actual_state, 0xa5, sizeof(actual_state));
        check("seed native state", uc_mem_write(m68k, m68k_state_base,
                                                  actual_state,
                                                  sizeof(actual_state)));

        const int registers[] = {
            UC_M68K_REG_D0, UC_M68K_REG_D1, UC_M68K_REG_D2,
            UC_M68K_REG_D3, UC_M68K_REG_D4, UC_M68K_REG_D5,
            UC_M68K_REG_D6, UC_M68K_REG_D7, UC_M68K_REG_A0,
            UC_M68K_REG_A1, UC_M68K_REG_A2, UC_M68K_REG_A3,
            UC_M68K_REG_A4, UC_M68K_REG_A5, UC_M68K_REG_A6,
        };
        const uint32_t values[] = {
            0, widths[test], 0x22223333u, 0x33334444u,
            0x44445555u, 0x55556666u, 0x66667777u, 0x77778888u,
            m68k_plane_base, m68k_state_base, 0x22222222u,
            0x33333333u, 0x44444444u, 0x55555555u, 0x66666666u,
        };
        for (size_t i = 0; i < sizeof(registers) / sizeof(registers[0]); ++i)
            check("write native register",
                  uc_reg_write(m68k, registers[i], &values[i]));
        uint32_t stack = m68k_stack_base + 0x8000;
        check("write native A7", uc_reg_write(m68k, UC_M68K_REG_A7,
                                                &stack));
        check("run native mode setup", uc_emu_start(
                  m68k, m68k_code_base + clear_bytes,
                  m68k_code_base + mode_bytes - 2, 0, 0));
        check("read native state", uc_mem_read(m68k, m68k_state_base,
                                                 actual_state,
                                                 sizeof(actual_state)));
        check("read native planes", uc_mem_read(m68k, m68k_plane_base,
                                                  actual_planes,
                                                  m68k_plane_size));

        uint32_t result = 0;
        check("read native result", uc_reg_read(m68k, UC_M68K_REG_D0,
                                                  &result));
        int bad_register = -1;
        for (size_t i = 1; i < sizeof(registers) / sizeof(registers[0]); ++i) {
            uint32_t value = 0;
            check("read preserved register",
                  uc_reg_read(m68k, registers[i], &value));
            if (value != values[i] && bad_register < 0)
                bad_register = (int)i;
        }
        if (hardware.bad_hardware || hardware.int10_count != 1 ||
            hardware.int33_count != 4 || hardware.clear_mask_count < 2 ||
            result != 0 || bad_register >= 0 ||
            memcmp(expected_state, actual_state, native_state_size) ||
            memcmp(expected_planes, actual_planes, m68k_plane_size)) {
            size_t state_difference = 0;
            while (state_difference < native_state_size &&
                   expected_state[state_difference] ==
                       actual_state[state_difference])
                ++state_difference;
            size_t plane_difference = 0;
            while (plane_difference < m68k_plane_size &&
                   expected_planes[plane_difference] ==
                       actual_planes[plane_difference])
                ++plane_difference;
            fprintf(stderr,
                    "mode setup case %u width=%u failed: hw=%u int=%u/%u "
                    "mask=%u result=%08x reg=%d state=%zu plane=%zu\n",
                    test, widths[test], hardware.bad_hardware,
                    hardware.int10_count, hardware.int33_count,
                    hardware.clear_mask_count, result, bad_register,
                    state_difference, plane_difference);
            if (state_difference < native_state_size)
                fprintf(stderr, "state bytes %02x/%02x\n",
                        expected_state[state_difference],
                        actual_state[state_difference]);
            return 1;
        }
    }

    uc_close(x86);
    uc_close(m68k);
    free(runtime);
    free(clear);
    free(mode);
    free(expected_planes);
    free(actual_planes);
    puts("native VGA mode setup: 9 observed-mode geometry cases passed");
    return 0;
}
