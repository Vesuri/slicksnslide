/* Execute only the relocation-free Compack loader and capture its handoff.
 *
 * This is deliberately not a general DOS emulator. It reconstructs the MZ
 * load state, executes the observed Compack stub under Unicorn, and stops
 * before the first instruction of the decompressed program.
 */

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unicorn/unicorn.h>

enum {
    MEMORY_SIZE = 2 * 1024 * 1024,
    PSP_SEGMENT = 0x1000,
    LOAD_SEGMENT = PSP_SEGMENT + 0x10,
    MAX_INSTRUCTIONS = 20000000,
};

struct mz_header {
    uint16_t last_page_bytes;
    uint16_t pages;
    uint16_t relocations;
    uint16_t header_paragraphs;
    uint16_t min_alloc;
    uint16_t max_alloc;
    uint16_t ss;
    uint16_t sp;
    uint16_t checksum;
    uint16_t ip;
    uint16_t cs;
    uint16_t reloc_table_offset;
    uint16_t overlay;
};

struct capture {
    uc_engine *uc;
    uint16_t loader_cs;
    uint16_t loader_ss;
    uint16_t final_cs;
    uint16_t final_ip;
    bool entered_copied_decoder;
    bool reached_handoff;
    bool unexpected_interrupt;
    uint32_t interrupt_number;
    uint64_t instructions;
    uint32_t *relocations;
    size_t relocation_count;
    size_t relocation_capacity;
};

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static void fail(const char *message) {
    fprintf(stderr, "%s\n", message);
    exit(EXIT_FAILURE);
}

static void fail_errno(const char *operation, const char *path) {
    fprintf(stderr, "%s %s: %s\n", operation, path, strerror(errno));
    exit(EXIT_FAILURE);
}

static void check_uc(uc_err error, const char *operation) {
    if (error != UC_ERR_OK) {
        fprintf(stderr, "%s: %s\n", operation, uc_strerror(error));
        exit(EXIT_FAILURE);
    }
}

static uint8_t *read_file(const char *path, size_t *size_out) {
    FILE *file = fopen(path, "rb");
    if (!file)
        fail_errno("open", path);
    if (fseek(file, 0, SEEK_END) != 0)
        fail_errno("seek", path);
    long length = ftell(file);
    if (length < 0)
        fail_errno("tell", path);
    rewind(file);
    uint8_t *data = malloc((size_t)length);
    if (!data)
        fail("out of memory reading executable");
    if (fread(data, 1, (size_t)length, file) != (size_t)length)
        fail_errno("read", path);
    fclose(file);
    *size_out = (size_t)length;
    return data;
}

static struct mz_header parse_header(const uint8_t *data, size_t size) {
    if (size < 28 || data[0] != 'M' || data[1] != 'Z')
        fail("input is not a DOS MZ executable");
    struct mz_header h = {
        .last_page_bytes = read_u16(data + 2),
        .pages = read_u16(data + 4),
        .relocations = read_u16(data + 6),
        .header_paragraphs = read_u16(data + 8),
        .min_alloc = read_u16(data + 10),
        .max_alloc = read_u16(data + 12),
        .ss = read_u16(data + 14),
        .sp = read_u16(data + 16),
        .checksum = read_u16(data + 18),
        .ip = read_u16(data + 20),
        .cs = read_u16(data + 22),
        .reloc_table_offset = read_u16(data + 24),
        .overlay = read_u16(data + 26),
    };
    if (h.relocations != 0)
        fail("this capture tool expects the observed relocation-free Compack wrapper");
    return h;
}

static void write_guest_word(uc_engine *uc, uint32_t address, uint16_t value) {
    uint8_t bytes[2] = {(uint8_t)value, (uint8_t)(value >> 8)};
    check_uc(uc_mem_write(uc, address, bytes, sizeof(bytes)), "write guest word");
}

static void record_relocation(struct capture *capture, uint32_t physical) {
    const uint32_t image_base = (uint32_t)LOAD_SEGMENT << 4;
    if (physical < image_base || physical - image_base > UINT16_MAX * 16u + 15u)
        fail("Compack relocation target lies outside representable MZ image");
    if (capture->relocation_count == capture->relocation_capacity) {
        size_t capacity = capture->relocation_capacity
                              ? capture->relocation_capacity * 2
                              : 1024;
        uint32_t *items = realloc(capture->relocations,
                                  capacity * sizeof(*items));
        if (!items)
            fail("out of memory recording Compack relocations");
        capture->relocations = items;
        capture->relocation_capacity = capacity;
    }
    capture->relocations[capture->relocation_count++] = physical - image_base;
}

static void code_hook(uc_engine *uc, uint64_t address, uint32_t size,
                      void *user_data) {
    (void)address;
    (void)size;
    struct capture *capture = user_data;
    uint16_t cs = 0;
    uint16_t ip = 0;
    check_uc(uc_reg_read(uc, UC_X86_REG_CS, &cs), "read CS");
    check_uc(uc_reg_read(uc, UC_X86_REG_IP, &ip), "read IP");
    capture->instructions++;

    if (cs == capture->loader_ss && ip == 0x0642)
        capture->entered_copied_decoder = true;

    /* The copied decoder applies every original MZ segment relocation with
     * `add word ptr es:[bx],di` here. Record the target before it executes. */
    if (capture->entered_copied_decoder && cs == capture->loader_ss &&
        ip == 0x06ef) {
        uint16_t es = 0;
        uint16_t bx = 0;
        check_uc(uc_reg_read(uc, UC_X86_REG_ES, &es), "read relocation ES");
        check_uc(uc_reg_read(uc, UC_X86_REG_BX, &bx), "read relocation BX");
        record_relocation(capture, ((uint32_t)es << 4) + bx);
    }

    uint32_t handoff_address = ((uint32_t)capture->loader_ss << 4) + 0x070b;
    if (capture->entered_copied_decoder && address == handoff_address) {
        uint8_t target[4];
        check_uc(uc_mem_read(uc, handoff_address + 1, target, sizeof(target)),
                 "read Compack far target");
        capture->final_ip = read_u16(target);
        capture->final_cs = read_u16(target + 2);
        capture->reached_handoff = true;
        uc_emu_stop(uc);
    }
}

static void interrupt_hook(uc_engine *uc, uint32_t intno, void *user_data) {
    struct capture *capture = user_data;
    capture->unexpected_interrupt = true;
    capture->interrupt_number = intno;
    uc_emu_stop(uc);
}

static void write_dump(const char *path, uc_engine *uc, uint32_t address,
                       size_t size) {
    uint8_t *data = malloc(size);
    if (!data)
        fail("out of memory creating runtime dump");
    check_uc(uc_mem_read(uc, address, data, size), "read runtime image");
    FILE *file = fopen(path, "wb");
    if (!file)
        fail_errno("open", path);
    if (fwrite(data, 1, size, file) != size)
        fail_errno("write", path);
    fclose(file);
    free(data);
}

static void write_state(const char *path, uc_engine *uc,
                        const struct capture *capture, size_t image_size) {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, ds, es, ss;
    uint32_t eflags;
#define READ_REG(name, value) \
    check_uc(uc_reg_read(uc, UC_X86_REG_##name, &(value)), "read " #name)
    READ_REG(AX, ax); READ_REG(BX, bx); READ_REG(CX, cx); READ_REG(DX, dx);
    READ_REG(SI, si); READ_REG(DI, di); READ_REG(BP, bp); READ_REG(SP, sp);
    READ_REG(DS, ds); READ_REG(ES, es); READ_REG(SS, ss);
    READ_REG(EFLAGS, eflags);
#undef READ_REG
    FILE *file = fopen(path, "w");
    if (!file)
        fail_errno("open", path);
    fprintf(file,
            "{\n"
            "  \"instructions\": %" PRIu64 ",\n"
            "  \"image_size\": %zu,\n"
            "  \"relocation_count\": %zu,\n"
            "  \"registers\": {\n"
            "    \"ax\": %u, \"bx\": %u, \"cx\": %u, \"dx\": %u,\n"
            "    \"si\": %u, \"di\": %u, \"bp\": %u, \"sp\": %u,\n"
            "    \"cs\": %u, \"ip\": %u, \"ds\": %u, \"es\": %u,\n"
            "    \"ss\": %u, \"flags\": %u\n"
            "  }\n"
            "}\n",
            capture->instructions, image_size, capture->relocation_count,
            ax, bx, cx, dx, si, di, bp, sp,
            capture->final_cs, capture->final_ip, ds, es, ss,
            (unsigned)(eflags & 0xffff));
    fclose(file);
}

static void write_relocations(const char *path, const struct capture *capture) {
    FILE *file = fopen(path, "w");
    if (!file)
        fail_errno("open", path);
    fprintf(file, "linear_offset,offset,segment\n");
    for (size_t i = 0; i < capture->relocation_count; i++) {
        uint32_t linear = capture->relocations[i];
        fprintf(file, "0x%05" PRIx32 ",0x%04" PRIx32 ",0x%04" PRIx32 "\n",
                linear, linear & 0x0f, linear >> 4);
    }
    fclose(file);
}

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr,
                "usage: %s INPUT.EXE RUNTIME.BIN STATE.JSON RELOCATIONS.CSV\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    size_t file_size = 0;
    uint8_t *file_data = read_file(argv[1], &file_size);
    struct mz_header h = parse_header(file_data, file_size);
    size_t header_size = (size_t)h.header_paragraphs * 16;
    if (header_size > file_size)
        fail("MZ header extends beyond file");
    size_t load_size = file_size - header_size;

    uc_engine *uc = NULL;
    check_uc(uc_open(UC_ARCH_X86, UC_MODE_16, &uc), "open Unicorn x86-16");
    check_uc(uc_mem_map(uc, 0, MEMORY_SIZE, UC_PROT_ALL), "map guest memory");
    check_uc(uc_mem_write(uc, (uint32_t)LOAD_SEGMENT << 4,
                          file_data + header_size, load_size), "load MZ image");
    free(file_data);

    /* Minimal DOS PSP facts used by common startup code. The Compack loader
     * itself preserves this state and does not invoke DOS before handoff. */
    write_guest_word(uc, ((uint32_t)PSP_SEGMENT << 4) + 2, 0x9fff);
    write_guest_word(uc, ((uint32_t)PSP_SEGMENT << 4) + 0x2c, 0);

    uint16_t ax = 0;
    uint16_t ds = PSP_SEGMENT;
    uint16_t es = PSP_SEGMENT;
    uint16_t cs = (uint16_t)(LOAD_SEGMENT + h.cs);
    uint16_t ip = h.ip;
    uint16_t ss = (uint16_t)(LOAD_SEGMENT + h.ss);
    uint16_t sp = h.sp;
    check_uc(uc_reg_write(uc, UC_X86_REG_AX, &ax), "set AX");
    check_uc(uc_reg_write(uc, UC_X86_REG_DS, &ds), "set DS");
    check_uc(uc_reg_write(uc, UC_X86_REG_ES, &es), "set ES");
    check_uc(uc_reg_write(uc, UC_X86_REG_CS, &cs), "set CS");
    check_uc(uc_reg_write(uc, UC_X86_REG_IP, &ip), "set IP");
    check_uc(uc_reg_write(uc, UC_X86_REG_SS, &ss), "set SS");
    check_uc(uc_reg_write(uc, UC_X86_REG_SP, &sp), "set SP");

    struct capture capture = {
        .uc = uc,
        .loader_cs = cs,
        .loader_ss = ss,
    };
    uc_hook code_handle;
    uc_hook interrupt_handle;
    check_uc(uc_hook_add(uc, &code_handle, UC_HOOK_CODE, code_hook, &capture,
                         1, 0), "add code hook");
    check_uc(uc_hook_add(uc, &interrupt_handle, UC_HOOK_INTR, interrupt_hook,
                         &capture, 1, 0), "add interrupt hook");

    uint32_t start = ((uint32_t)cs << 4) + ip;
    uc_err error = uc_emu_start(uc, start, 0, 0, MAX_INSTRUCTIONS);
    if (error != UC_ERR_OK)
        check_uc(error, "execute Compack loader");
    if (capture.unexpected_interrupt) {
        fprintf(stderr, "unexpected interrupt %02" PRIX32
                        " before Compack handoff\n", capture.interrupt_number);
        return EXIT_FAILURE;
    }
    if (!capture.reached_handoff)
        fail("Compack handoff was not reached before the instruction limit");

    uint16_t final_ss = 0;
    uint16_t final_sp = 0;
    check_uc(uc_reg_read(uc, UC_X86_REG_SS, &final_ss), "read final SS");
    check_uc(uc_reg_read(uc, UC_X86_REG_SP, &final_sp), "read final SP");
    if (final_ss < capture.final_cs)
        fail("captured stack precedes final code segment");
    size_t runtime_size = ((size_t)(final_ss - capture.final_cs) << 4) + final_sp;
    uint32_t runtime_address = (uint32_t)capture.final_cs << 4;
    write_dump(argv[2], uc, runtime_address, runtime_size);
    write_state(argv[3], uc, &capture, runtime_size);
    write_relocations(argv[4], &capture);

    printf("Compack handoff after %" PRIu64 " instructions\n", capture.instructions);
    printf("entry %04X:%04X  stack %04X:%04X  runtime %zu bytes  relocs %zu\n",
           capture.final_cs, capture.final_ip, final_ss, final_sp, runtime_size,
           capture.relocation_count);

    free(capture.relocations);
    uc_close(uc);
    return EXIT_SUCCESS;
}
