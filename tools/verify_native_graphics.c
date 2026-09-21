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
    x86_stack_base = 0x50000,
    x86_stack_size = 0x10000,
    x86_stack_segment = 0x5000,
    x86_stack_offset = 0x8000,
    x86_vga_base = 0xa0000,
    x86_vga_size = 0x10000,
    x86_stride_address = 0x3e86b,
    x86_plot_start = 0x3b45e,
    x86_plot_stop = 0x3b48d,
    x86_read_start = 0x3b48e,
    x86_read_stop = 0x3b4ba,
    m68k_code_base = 0x1000,
    m68k_plane_base = 0x100000,
    m68k_plane_size = 0x40000,
};

typedef struct {
    int plane;
    int bad_port_value;
} VgaPortState;

static void fail_uc(const char *operation, const uc_err error)
{
    fprintf(stderr, "%s: %s\n", operation, uc_strerror(error));
    exit(1);
}

static void check_uc(const char *operation, const uc_err error)
{
    if (error != UC_ERR_OK)
        fail_uc(operation, error);
}

static uint8_t *read_file(const char *path, size_t *size)
{
    FILE *input = fopen(path, "rb");
    if (!input) {
        perror(path);
        exit(1);
    }
    if (fseek(input, 0, SEEK_END) || (*size = (size_t)ftell(input)) == 0 ||
        fseek(input, 0, SEEK_SET)) {
        fprintf(stderr, "%s: cannot determine non-empty size\n", path);
        exit(1);
    }
    uint8_t *data = malloc(*size);
    if (!data || fread(data, 1, *size, input) != *size) {
        fprintf(stderr, "%s: cannot read file\n", path);
        exit(1);
    }
    fclose(input);
    return data;
}

static void write_le16(uint8_t bytes[2], const uint16_t value)
{
    bytes[0] = (uint8_t)value;
    bytes[1] = (uint8_t)(value >> 8);
}

static uint32_t plane_from_mask(const uint32_t mask)
{
    switch (mask) {
    case 1: return 0;
    case 2: return 1;
    case 4: return 2;
    case 8: return 3;
    default: return UINT32_MAX;
    }
}

static void hook_x86_out(uc_engine *uc, uint32_t port, int size,
                         uint32_t value, void *user_data)
{
    (void)uc;
    VgaPortState *state = user_data;
    if (size != 2) {
        state->bad_port_value = 1;
        return;
    }
    const uint8_t index = (uint8_t)value;
    const uint8_t data = (uint8_t)(value >> 8);
    if (port == 0x3c4 && index == 2) {
        const uint32_t plane = plane_from_mask(data);
        if (plane == UINT32_MAX)
            state->bad_port_value = 1;
        else
            state->plane = (int)plane;
    } else if (port == 0x3ce && index == 4) {
        if (data > 3)
            state->bad_port_value = 1;
        else
            state->plane = data;
    } else {
        state->bad_port_value = 1;
    }
}

static uc_engine *open_x86(const uint8_t *runtime)
{
    uc_engine *uc = NULL;
    check_uc("open x86", uc_open(UC_ARCH_X86, UC_MODE_16, &uc));
    check_uc("map x86 runtime", uc_mem_map(uc, runtime_base, 0x40000, UC_PROT_ALL));
    check_uc("write x86 runtime",
             uc_mem_write(uc, runtime_base, runtime, runtime_size));
    check_uc("map x86 stack",
             uc_mem_map(uc, x86_stack_base, x86_stack_size, UC_PROT_ALL));
    check_uc("map x86 VGA", uc_mem_map(uc, x86_vga_base, x86_vga_size,
                                        UC_PROT_ALL));
    const uint8_t stride[2] = {100, 0};
    check_uc("write x86 stride",
             uc_mem_write(uc, x86_stride_address, stride, sizeof(stride)));
    return uc;
}

static uc_engine *open_m68k(const uint8_t *code, const size_t code_size)
{
    uc_engine *uc = NULL;
    check_uc("open m68k", uc_open(UC_ARCH_M68K, UC_MODE_BIG_ENDIAN, &uc));
    check_uc("select 68020", uc_ctl_set_cpu_model(uc, UC_CPU_M68K_M68020));
    check_uc("map m68k code", uc_mem_map(uc, m68k_code_base, 0x1000, UC_PROT_ALL));
    check_uc("write m68k code", uc_mem_write(uc, m68k_code_base, code, code_size));
    check_uc("map m68k planes",
             uc_mem_map(uc, m68k_plane_base, m68k_plane_size, UC_PROT_ALL));
    return uc;
}

static uint16_t guest_offset(const uint16_t x, const uint16_t y,
                             const uint16_t screen_base)
{
    return (uint16_t)(screen_base + (uint16_t)(y * 100u) + (x >> 2));
}

static void prepare_x86_stack(uc_engine *uc, const uint16_t *arguments,
                              const size_t argument_count)
{
    uint8_t stack[4 + 8] = {0};
    for (size_t i = 0; i < argument_count; ++i)
        write_le16(&stack[4 + i * 2], arguments[i]);
    const uint64_t address = x86_stack_base + x86_stack_offset;
    check_uc("write x86 stack", uc_mem_write(uc, address, stack,
                                              4 + argument_count * 2));
    uint16_t value = x86_stack_segment;
    check_uc("write SS", uc_reg_write(uc, UC_X86_REG_SS, &value));
    value = x86_stack_offset;
    check_uc("write SP", uc_reg_write(uc, UC_X86_REG_SP, &value));
    value = 0x3caf;
    check_uc("write DS", uc_reg_write(uc, UC_X86_REG_DS, &value));
    value = 0x3b45;
    check_uc("write CS", uc_reg_write(uc, UC_X86_REG_CS, &value));
    value = 0;
    check_uc("clear BP", uc_reg_write(uc, UC_X86_REG_BP, &value));
}

static void prepare_m68k(uc_engine *uc, const uint16_t x, const uint16_t y,
                         const uint8_t pixel, const uint16_t screen_base)
{
    uint32_t value = m68k_plane_base;
    check_uc("write A0", uc_reg_write(uc, UC_M68K_REG_A0, &value));
    value = 0x11110000u | x;
    check_uc("write D0", uc_reg_write(uc, UC_M68K_REG_D0, &value));
    value = 0x22220000u | y;
    check_uc("write D1", uc_reg_write(uc, UC_M68K_REG_D1, &value));
    value = 0x33330000u | pixel;
    check_uc("write D2", uc_reg_write(uc, UC_M68K_REG_D2, &value));
    value = 0x44440000u | screen_base;
    check_uc("write D3", uc_reg_write(uc, UC_M68K_REG_D3, &value));
    value = 0x55550064u;
    check_uc("write D4", uc_reg_write(uc, UC_M68K_REG_D4, &value));
}

static void check_m68k_live_out(uc_engine *uc, const uint16_t x,
                                const uint16_t y, const uint8_t pixel,
                                const uint16_t screen_base, const int is_read,
                                const unsigned case_number)
{
    static const int registers[] = {
        UC_M68K_REG_D0, UC_M68K_REG_D1, UC_M68K_REG_D2,
        UC_M68K_REG_D3, UC_M68K_REG_D4, UC_M68K_REG_A0,
    };
    const uint32_t expected[] = {
        is_read ? pixel : 0x11110000u | x,
        0x22220000u | y,
        0x33330000u | (is_read ? 0u : pixel),
        0x44440000u | screen_base,
        0x55550064u,
        m68k_plane_base,
    };
    for (size_t i = 0; i < sizeof(registers) / sizeof(registers[0]); ++i) {
        uint32_t actual = 0;
        check_uc("read m68k live-out",
                 uc_reg_read(uc, registers[i], &actual));
        if (actual != expected[i]) {
            fprintf(stderr,
                    "%s case %u corrupted live register %zu: %08x != %08x\n",
                    is_read ? "read" : "plot", case_number, i, actual,
                    expected[i]);
            exit(1);
        }
    }
}

static void reset_x86_hook(uc_engine *uc, VgaPortState *state, uc_hook *hook)
{
    state->plane = -1;
    state->bad_port_value = 0;
    if (*hook)
        check_uc("delete OUT hook", uc_hook_del(uc, *hook));
    check_uc("add OUT hook",
             uc_hook_add(uc, hook, UC_HOOK_INSN, (void *)hook_x86_out, state,
                         1, 0, UC_X86_INS_OUT));
}

static void run_case(uc_engine *x86_plot, uc_engine *x86_read,
                     uc_engine *m68k_plot, const size_t m68k_plot_size,
                     uc_engine *m68k_read, const size_t m68k_read_size,
                     const uint16_t x, const uint16_t y, const uint8_t pixel,
                     const uint16_t screen_base, const unsigned case_number)
{
    const uint16_t offset = guest_offset(x, y, screen_base);
    const unsigned expected_plane = x & 3u;
    const uint64_t native_address =
        m68k_plane_base + expected_plane * 0x10000u + offset;
    const uint64_t x86_address = x86_vga_base + offset;
    const uint8_t sentinel = (uint8_t)(pixel ^ 0xa5u);
    VgaPortState ports = {-1, 0};
    uc_hook hook = 0;

    check_uc("seed x86 plot byte", uc_mem_write(x86_plot, x86_address,
                                                 &sentinel, 1));
    check_uc("seed m68k plot byte", uc_mem_write(m68k_plot, native_address,
                                                  &sentinel, 1));
    const uint16_t plot_args[] = {x, y, pixel, screen_base};
    prepare_x86_stack(x86_plot, plot_args, 4);
    reset_x86_hook(x86_plot, &ports, &hook);
    check_uc("run x86 plot",
             uc_emu_start(x86_plot, x86_plot_start, x86_plot_stop, 0, 0));
    uint8_t x86_result = 0;
    uint8_t native_result = 0;
    check_uc("read x86 plot result",
             uc_mem_read(x86_plot, x86_address, &x86_result, 1));
    prepare_m68k(m68k_plot, x, y, pixel, screen_base);
    check_uc("run m68k plot",
             uc_emu_start(m68k_plot, m68k_code_base,
                          m68k_code_base + m68k_plot_size - 2, 0, 0));
    check_uc("read m68k plot result",
             uc_mem_read(m68k_plot, native_address, &native_result, 1));
    check_m68k_live_out(m68k_plot, x, y, pixel, screen_base, 0, case_number);
    if (ports.bad_port_value || ports.plane != (int)expected_plane ||
        x86_result != pixel || native_result != x86_result) {
        fprintf(stderr,
                "plot case %u failed: x=%04x y=%04x base=%04x plane=%d/%u "
                "x86=%02x m68k=%02x\n",
                case_number, x, y, screen_base, ports.plane, expected_plane,
                x86_result, native_result);
        exit(1);
    }
    check_uc("delete plot OUT hook", uc_hook_del(x86_plot, hook));
    hook = 0;

    check_uc("seed x86 read byte", uc_mem_write(x86_read, x86_address,
                                                 &pixel, 1));
    check_uc("seed m68k read byte", uc_mem_write(m68k_read, native_address,
                                                  &pixel, 1));
    const uint16_t read_args[] = {x, y, screen_base};
    prepare_x86_stack(x86_read, read_args, 3);
    reset_x86_hook(x86_read, &ports, &hook);
    check_uc("run x86 read",
             uc_emu_start(x86_read, x86_read_start, x86_read_stop, 0, 0));
    uint16_t x86_ax = 0;
    uint32_t native_d0 = 0;
    check_uc("read x86 AX", uc_reg_read(x86_read, UC_X86_REG_AX, &x86_ax));
    prepare_m68k(m68k_read, x, y, 0, screen_base);
    check_uc("run m68k read",
             uc_emu_start(m68k_read, m68k_code_base,
                          m68k_code_base + m68k_read_size - 2, 0, 0));
    check_uc("read m68k D0",
             uc_reg_read(m68k_read, UC_M68K_REG_D0, &native_d0));
    check_m68k_live_out(m68k_read, x, y, pixel, screen_base, 1, case_number);
    if (ports.bad_port_value || ports.plane != (int)expected_plane ||
        x86_ax != pixel || native_d0 != x86_ax) {
        fprintf(stderr,
                "read case %u failed: x=%04x y=%04x base=%04x plane=%d/%u "
                "x86=%04x m68k=%08x\n",
                case_number, x, y, screen_base, ports.plane, expected_plane,
                x86_ax, native_d0);
        exit(1);
    }
    check_uc("delete read OUT hook", uc_hook_del(x86_read, hook));
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
    if (argc != 4) {
        fprintf(stderr, "usage: %s runtime.bin plot.bin read.bin\n", argv[0]);
        return 2;
    }
    size_t runtime_bytes = 0;
    size_t plot_bytes = 0;
    size_t read_bytes = 0;
    uint8_t *runtime = read_file(argv[1], &runtime_bytes);
    uint8_t *plot = read_file(argv[2], &plot_bytes);
    uint8_t *read = read_file(argv[3], &read_bytes);
    if (runtime_bytes != runtime_size || plot_bytes < 4 || read_bytes < 4) {
        fprintf(stderr, "unexpected runtime or native routine size\n");
        return 1;
    }

    uc_engine *x86_plot = open_x86(runtime);
    uc_engine *x86_read = open_x86(runtime);
    uc_engine *m68k_plot = open_m68k(plot, plot_bytes);
    uc_engine *m68k_read = open_m68k(read, read_bytes);

    static const uint16_t edge_x[] = {0, 1, 2, 3, 4, 319, 320, 0xffff};
    static const uint16_t edge_y[] = {0, 1, 189, 326, 0xffff};
    unsigned cases = 0;
    for (size_t xi = 0; xi < sizeof(edge_x) / sizeof(edge_x[0]); ++xi) {
        for (size_t yi = 0; yi < sizeof(edge_y) / sizeof(edge_y[0]); ++yi) {
            run_case(x86_plot, x86_read, m68k_plot, plot_bytes, m68k_read,
                     read_bytes, edge_x[xi], edge_y[yi],
                     (uint8_t)(xi * 37u + yi * 19u),
                     (yi & 1u) ? 32700 : 0, cases++);
        }
    }
    for (unsigned i = 0; i < 2000; ++i) {
        const uint16_t x = (uint16_t)next_random();
        const uint16_t y = (uint16_t)next_random();
        const uint8_t pixel = (uint8_t)next_random();
        const uint16_t base = (uint16_t)next_random();
        run_case(x86_plot, x86_read, m68k_plot, plot_bytes, m68k_read,
                 read_bytes, x, y, pixel, base, cases++);
    }

    uc_close(x86_plot);
    uc_close(x86_read);
    uc_close(m68k_plot);
    uc_close(m68k_read);
    free(runtime);
    free(plot);
    free(read);
    printf("native graphics differential: %u plot/read cases passed\n", cases);
    return 0;
}
