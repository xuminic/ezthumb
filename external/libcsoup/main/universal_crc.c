/*
 * universal_crc - CRC C code generator utility
 *
 * Copyright (C) 2011 Danjel McGougan
 * Contact me at <danjel.mcgougan@gmail.com>
 *
 * Ideas and inspiration from at least these sources:
 *
 *   A PAINLESS GUIDE TO CRC ERROR DETECTION ALGORITHMS by Ross N. Williams
 *   (http://www.ross.net/crc/crcpaper.html)
 *
 *   PYCRC by Thomas Pircher
 *   (http://www.tty1.net/pycrc)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char bit_t;
typedef unsigned char bool_t;

#define VERSION "1.3a"
#define FALSE 0
#define TRUE 1

/*
 * Some portability cruft
 */

#ifndef WIN32
/*
 * If your env does not support inttypes.h replace with this:
 * typedef unsigned long long crc_reg_t;
 * #define REGFMT "llx"
 */
#include <inttypes.h>
typedef uint64_t crc_reg_t;
#define REGFMT PRIx64
#else
/* Windows */
typedef unsigned __int64 crc_reg_t;
typedef unsigned char uint8_t;
#define REGFMT "I64x"
#endif

static crc_reg_t strtoreg(const char *str)
{
    crc_reg_t v;
    if (str[0] == '0' && str[1] == 'x') {
        sscanf(&str[2], "%" REGFMT, &v);
    } else {
        sscanf(str, "%" REGFMT, &v);
    }
    return v;
}

/*
 * Implementation of a shift register with parameterized length
 */

struct bit_reg {
    unsigned int bits;
    crc_reg_t top_bit;
    crc_reg_t mask;
    crc_reg_t reg;
};

static void bit_reg_init(struct bit_reg *reg, unsigned int bits)
{
    reg->bits = bits;
    reg->top_bit = (crc_reg_t)1 << (bits - 1);
    reg->mask = reg->top_bit | (reg->top_bit - 1);
    reg->reg = 0;
}

static bit_t bit_reg_shift_left(struct bit_reg *reg, bit_t in)
{
    bit_t out = !!(reg->reg & reg->top_bit);
    reg->reg <<= 1;
    reg->reg &= reg->mask;
    reg->reg |= !!in;
    return out;
}

static bit_t bit_reg_shift_right(struct bit_reg *reg, bit_t in)
{
    bit_t out = (bit_t)(reg->reg & 1);
    reg->reg >>= 1;
    if (in) {
        reg->reg |= reg->top_bit;
    }
    return out;
}

static void bit_reg_reverse(struct bit_reg *reg)
{
    struct bit_reg tmp_reg = *reg;
    unsigned int i;
    for (i = 0; i < reg->bits; i++) {
        bit_reg_shift_right(reg, bit_reg_shift_left(&tmp_reg, 0));
    }
}

static void bit_reg_set(struct bit_reg *reg, crc_reg_t value)
{
    reg->reg = value & reg->mask;
}

static crc_reg_t bit_reg_get(struct bit_reg *reg)
{
    return reg->reg;
}

static void bit_reg_xor(struct bit_reg *reg, crc_reg_t value)
{
    reg->reg = (reg->reg ^ value) & reg->mask;
}

/*
 * Code to help generate tables
 */

static void tab_post_value(unsigned int i,
                           unsigned int size,
                           unsigned int per_line)
{
    if (i < size - 1) {
        if (i % per_line < per_line - 1) {
            printf(", ");
        } else {
            printf(",\n");
        }
    }
}

struct crc_param {
    unsigned int bits;
    crc_reg_t poly;
    crc_reg_t init;
    crc_reg_t eor;
    bool_t reverse;
    bool_t non_direct;
};

/*
 * General CRC calculation, really slow but handles all variants
 * Note: len is given in bits
 */
static crc_reg_t crc_calc(const unsigned char *data,
                          size_t len,
                          const struct crc_param *param)
{
    struct bit_reg reg;
    struct bit_reg poly;
    struct bit_reg msg;
    unsigned int i;

    bit_reg_init(&reg, param->bits);
    bit_reg_init(&poly, param->bits);
    bit_reg_init(&msg, 8);

    bit_reg_set(&reg, param->init);
    bit_reg_set(&poly, param->poly);
    if (param->reverse) {
        bit_reg_reverse(&poly);
    }

    if (len > 0) {
        bit_reg_set(&msg, *data++);
    }

    while (len > 0) {
        if (param->non_direct) {
            if (param->reverse) {
                if (bit_reg_shift_right(&reg, bit_reg_shift_right(&msg, 0))) {
                    bit_reg_xor(&reg, bit_reg_get(&poly));
                }
            } else {
                if (bit_reg_shift_left(&reg, bit_reg_shift_left(&msg, 0))) {
                    bit_reg_xor(&reg, bit_reg_get(&poly));
                }
            }
        } else {
            if (param->reverse) {
                if (bit_reg_shift_right(&reg, 0) ^ bit_reg_shift_right(&msg, 0)) {
                    bit_reg_xor(&reg, bit_reg_get(&poly));
                }
            } else {
                if (bit_reg_shift_left(&reg, 0) ^ bit_reg_shift_left(&msg, 0)) {
                    bit_reg_xor(&reg, bit_reg_get(&poly));
                }
            }
        }

        len--;

        if (len % 8 == 0 && len > 0) {
            bit_reg_set(&msg, *data++);
        }
    }

    if (param->non_direct) {
        for (i = 0; i < param->bits; i++) {
            if (param->reverse) {
                if (bit_reg_shift_right(&reg, 0)) {
                    bit_reg_xor(&reg, bit_reg_get(&poly));
                }
            } else {
                if (bit_reg_shift_left(&reg, 0)) {
                    bit_reg_xor(&reg, bit_reg_get(&poly));
                }
            }
        }
    }

    bit_reg_xor(&reg, param->eor);

    return bit_reg_get(&reg);
}

/*
 * Generate optimized ANSI C code for calculating a CRC
 */

typedef enum {
    ALGO_BIT,
    ALGO_TAB,
    ALGO_TAB16,
    ALGO_TAB16I,
    ALGO_TABW,
    ALGO_TABI,
    ALGO_TABIW
} algo_t;

struct code_param {
    const char *crc_type;
    const char *tab_type;
    algo_t algo;
    bool_t test_code;
};

static crc_reg_t init_to_direct(const struct crc_param* param)
{
    crc_reg_t init = param->init;
    unsigned int i;

    /* Convert from non-direct to direct if needed */
    if (param->non_direct && init) {
        struct bit_reg reg;
        struct bit_reg poly;
        bit_reg_init(&reg, param->bits);
        bit_reg_init(&poly, param->bits);
        bit_reg_set(&reg, param->init);
        bit_reg_set(&poly, param->poly);
        if (param->reverse) {
            bit_reg_reverse(&reg);
        }
        for (i = 0; i < param->bits; i++) {
            if (bit_reg_shift_left(&reg, 0)) {
                bit_reg_xor(&reg, bit_reg_get(&poly));
            }
        }
        if (param->reverse) {
            bit_reg_reverse(&reg);
        }
        init = bit_reg_get(&reg);
    }

    return init;
}

static void get_types(const struct crc_param *param,
                      const struct code_param *code_param,
                      const char **crc_type,
                      const char **tab_type,
                      unsigned int *crc_type_bits,
                      unsigned int *crc_shift)
{
    unsigned int shift;

    /* Data types of the CRC register and the table entries */
    if (param->bits <= 8) {
        *crc_type = "uint8_t";
        *crc_type_bits = 8;
        shift = 8 - param->bits;
    } else if (param->bits <= 16) {
        *crc_type = "uint16_t";
        *crc_type_bits = 16;
        shift = 16 - param->bits;
    } else if (param->bits <= 32) {
        *crc_type = "uint32_t";
        *crc_type_bits = 32;
        shift = 32 - param->bits;
    } else {
        *crc_type = "uint64_t";
        *crc_type_bits = 64;
        shift = 64 - param->bits;
    }
    *tab_type = *crc_type;

    /* Types can be overridden */
    if (code_param->crc_type) {
        *crc_type = code_param->crc_type;
        *crc_type_bits = 0;
    }
    if (code_param->tab_type) {
        *tab_type = code_param->tab_type;
    }

    if (crc_shift) *crc_shift = shift;
}

static void gen_header_comment(const struct crc_param *param)
{
    unsigned int print_width = (param->bits + 7) / 8 * 2;

    printf("/*\n * crc.h\n *\n"
           " * Code generated by universal_crc by Danjel McGougan\n *\n"
           " * CRC parameters used:\n"
           " *   bits:       %u\n"
           " *   poly:       0x%0*" REGFMT "\n"
           " *   init:       0x%0*" REGFMT "\n"
           " *   xor:        0x%0*" REGFMT "\n"
           " *   reverse:    %s\n"
           " *   non-direct: %s\n *\n"
           " * CRC of the string \"123456789\" is 0x%0*" REGFMT "\n */\n\n",
           param->bits,
           print_width,
           param->poly,
           print_width,
           param->init,
           print_width,
           param->eor,
           param->reverse ? "true" : "false",
           param->non_direct ? "true" : "false",
           print_width,
           crc_calc((const unsigned char *)"123456789", 9 * 8, param));
}

static void gen_test_code(const struct crc_param *param,
                          const char *crc_type)
{
    unsigned char test_data[64];
    unsigned int per_line;
    unsigned int print_width = (param->bits + 7) / 8 * 2;
    unsigned int i;

    srand((unsigned int)(param->bits ^
                         param->poly ^
                         param->init ^
                         param->eor ^
                         param->reverse ^
                         param->non_direct));
    for (i = 0; i < sizeof(test_data); i++) {
        test_data[i] = rand() & 0xff;
    }

    printf("\nint main(void)\n{\n");
    printf("\tstatic const uint8_t test_data[%u] = {\n",
           (unsigned)sizeof(test_data));
    for (i = 0; i < sizeof(test_data); i++) {
        if (i % 8 == 0) printf("\t\t");
        printf("0x%02x", test_data[i]);
        tab_post_value(i, sizeof(test_data), 8);
    }
    printf("\n\t};\n");

    per_line = 1;
    while (4 + (2 + print_width + 2) * (per_line * 2) <= 84) per_line *= 2;
    printf("\tstatic const %s crc_result[%u] = {\n",
           crc_type,
           ((unsigned)sizeof(test_data) - 2) * 4);
    for (i = 0; i < (sizeof(test_data) - 2) * 4; i++) {
        if (i % per_line == 0) printf("\t\t");
        printf("0x%0*" REGFMT,
               print_width,
               crc_calc(&test_data[sizeof(test_data) - 3 - i / 4], (i / 4 + i % 4) * 8, param));
        tab_post_value(i, (sizeof(test_data) - 2) * 4, per_line);
    }
    printf("\n\t};\n");

    printf("\tsize_t i;\n\n"
           "\tfor (i = 0; i < %u; i++) {\n"
           "\t\tif (crc_calc(&test_data[%u - i / 4], i / 4 + i %% 4) != crc_result[i]) {\n"
           "\t\t\tfprintf(stderr, \"FAIL\\n\");\n"
           "\t\t\texit(1);\n"
           "\t\t}\n"
           "\t}\n\n"
           "\tprintf(\"OK\\n\");\n"
           "\treturn 0;\n"
           "}\n",
           ((unsigned)sizeof(test_data) - 2) * 4,
           (unsigned)sizeof(test_data) - 3);
}

static crc_reg_t byte_swap(crc_reg_t crc, unsigned int bits)
{
    if (bits <= 8) {
        /* do nothing */
    } else if (bits <= 16) {
        crc = (crc >> 8) | ((crc & 0xff) << 8);
    } else if (bits <= 32) {
        crc = (crc >> 16) | ((crc & 0xffff) << 16);
        crc = ((crc >> 8) & 0xff00ff) | ((crc & 0xff00ff) << 8);
    } else {
        crc = (crc >> 32) | ((crc & 0xffffffff) << 32);
        crc = ((crc >> 16) & 0xffff0000ffff) | ((crc & 0xffff0000ffff) << 16);
        crc = ((crc >> 8) & 0xff00ff00ff00ff) | ((crc & 0xff00ff00ff00ff) << 8);
    }
    return crc;
}

static void gen_code_standard(const struct crc_param *param,
                              const struct code_param *code_param)
{
    const char *crc_type;
    const char *tab_type;
    unsigned int crc_type_bits;
    crc_reg_t init = init_to_direct(param);
    crc_reg_t top_bit = (crc_reg_t)1 << (param->bits - 1);
    crc_reg_t mask = top_bit | (top_bit - 1);
    unsigned int print_width = (param->bits + 7) / 8 * 2;
    unsigned int i;
    unsigned int per_line;

    get_types(param, code_param, &crc_type, &tab_type, &crc_type_bits, 0);

    /*
     * Generate the header file
     */

    gen_header_comment(param);
    printf("#ifndef CRC_H_\n"
           "#define CRC_H_\n\n"
           "#include <stddef.h>\n");
    printf("#include <stdint.h>\n");
    printf("\n%s crc_calc(const uint8_t *data, size_t len);\n\n", crc_type);
    if (code_param->algo == ALGO_TAB) {
        printf("extern const %s crc_table[256];\n\n", tab_type);
    }

    /* crc_init */
    printf("static inline %s crc_init(void)\n"
           "{\n"
           "\treturn %s%" REGFMT ";\n"
           "}\n\n",
           crc_type,
           init ? "0x" : "",
           param->bits >= 8 || param->reverse ? init : (init << (8 - param->bits)));

    /* crc_next */
    printf("static inline %s crc_next(%s crc, uint8_t data)\n",
           crc_type, crc_type);
    if (code_param->algo == ALGO_TAB) {
        if (param->bits > 8) {
            if (param->reverse) {
                printf("{\n"
                       "\treturn (crc >> 8) ^ crc_table[(crc & 0xff) ^ data];\n"
                       "}\n\n");
            } else {
                printf("{\n"
                       "\treturn (crc << 8) ^ crc_table[((crc >> %u) & 0xff) ^ data];\n"
                       "}\n\n",
                       param->bits - 8);
            }
        } else {
            printf("{\n"
                   "\treturn crc_table[crc ^ data];\n"
                   "}\n\n");
        }
    } else {
        if (param->reverse) {
            struct bit_reg poly;
            bit_reg_init(&poly, param->bits);
            bit_reg_set(&poly, param->poly);
            bit_reg_reverse(&poly);
            printf("{\n"
                   "\t%s eor;\n"
                   "\tunsigned int i = 8;\n\n"
                   "\tcrc ^= data;\n"
                   "\tdo {\n"
                   "\t\t/* This might produce branchless code */\n"
                   "\t\teor = crc & 1 ? 0x%" REGFMT " : 0;\n"
                   "\t\tcrc >>= 1;\n"
                   "\t\tcrc ^= eor;\n"
                   "\t} while (--i);\n\n"
                   "\treturn crc;\n"
                   "}\n\n",
                   crc_type,
                   bit_reg_get(&poly));
        } else {
            printf("{\n"
                   "\t%s eor;\n"
                   "\tunsigned int i = 8;\n\n",
                   crc_type);

            if (param->bits > 8) {
                printf("\tcrc ^= (%s)data << %u;\n",
                       crc_type,
                       param->bits - 8);
            } else {
                printf("\tcrc ^= data;\n");
            }

            printf("\tdo {\n"
                   "\t\t/* This might produce branchless code */\n"
                   "\t\teor = crc & 0x%" REGFMT " ? 0x%" REGFMT " : 0;\n"
                   "\t\tcrc <<= 1;\n"
                   "\t\tcrc ^= eor;\n"
                   "\t} while (--i);\n\n"
                   "\treturn crc;\n"
                   "}\n\n",
                   param->bits >= 8 ? top_bit : 0x80,
                   param->bits >= 8 ? param->poly : param->poly << (8 - param->bits));
        }
    }

    /* crc_final */
    {
        char post_mask[24] = "";
        const char *pre_xor = "";
        char post_xor[24] = "";
        const char *pre_shift = "";
        char post_shift[8] = "";
        if (!param->reverse &&
            crc_type_bits != param->bits &&
            (param->bits > 8 || code_param->algo == ALGO_BIT))
        {
            /* Masking */
            sprintf(post_mask, " & 0x%" REGFMT, mask);
        }
        if (param->eor) {
            /* XORing */
            if (param->eor == mask &&
                (post_mask[0] || crc_type_bits == param->bits))
            {
                pre_xor = "~";
            } else {
                pre_xor = "(";
                sprintf(post_xor, " ^ 0x%" REGFMT ")", param->eor);
            }
        }
        if (param->bits < 8 && !param->reverse) {
            /* Shifting */
            pre_shift = "(";
            sprintf(post_shift, " >> %u)", 8 - param->bits);
        }
        printf("static inline %s crc_final(%s crc)\n"
               "{\n"
               "\treturn %s%scrc%s%s%s;\n"
               "}\n\n",
               crc_type,
               crc_type,
               pre_xor,
               pre_shift,
               post_shift,
               post_xor,
               post_mask);
    }

    printf("#endif\n\n");

    /*
     * Generate the C file
     */

    printf("/*\n * crc.c\n */\n\n");
    if (code_param->test_code) {
        printf("#include <stdio.h>\n\n");
        printf("#include <stdlib.h>\n\n");
    } else {
        printf("#include \"crc.h\"\n\n");
    }

    /* Generate the CRC table */
    if (code_param->algo == ALGO_TAB) {
        per_line = 1;
        while (4 + (2 + print_width + 2) * (per_line * 2) <= 84) per_line *= 2;
        printf("const %s crc_table[256] = {\n", tab_type);
        for (i = 0; i < 256; i++) {
            struct crc_param p = *param;
            unsigned char data = (unsigned char)i;
            crc_reg_t crc;
            p.init = 0;
            p.eor = 0;
            crc = crc_calc(&data, 8, &p);
            if (param->bits < 8 && !param->reverse) {
                /* Avoids the need for shifts in crc_next */
                crc <<= (8 - param->bits);
            }
            if (i % per_line == 0) printf("\t");
            printf("0x%0*" REGFMT, print_width, crc);
            tab_post_value(i, 256, per_line);
        }
        printf("\n};\n\n");
    }

    /*
     * Generate the crc_calc function
     */
    printf("%s crc_calc(const uint8_t *data, size_t len)\n"
           "{\n\t%s crc = crc_init();\n\n"
           "\tif (len) do {\n"
           "\t\tcrc = crc_next(crc, *data++);\n"
           "\t} while (--len);\n\n"
           "\treturn crc_final(crc);\n"
           "}\n",
           crc_type, crc_type);

    /* Generate test code */
    if (code_param->test_code) {
        gen_test_code(param, crc_type);
    }
}

static void gen_code_tab16(const struct crc_param *param,
                           const struct code_param *code_param)
{
    const char *crc_type;
    const char *tab_type;
    unsigned int crc_type_bits;
    crc_reg_t init = init_to_direct(param);
    crc_reg_t top_bit = (crc_reg_t)1 << (param->bits - 1);
    crc_reg_t mask = top_bit | (top_bit - 1);
    unsigned int print_width = (param->bits + 7) / 8 * 2;
    unsigned int i;
    unsigned int per_line;

    get_types(param, code_param, &crc_type, &tab_type, &crc_type_bits, 0);

    /*
     * Generate the header file
     */

    gen_header_comment(param);
    printf("#ifndef CRC_H_\n"
           "#define CRC_H_\n\n"
           "#include <stddef.h>\n");
    printf("#include <stdint.h>\n");
    printf("\n%s crc_calc(const uint8_t *data, size_t len);\n\n", crc_type);
    printf("extern const %s crc_table[16];\n\n", tab_type);

    /* crc_init */
    if (!param->reverse) {
        if (param->bits < 4) {
            init <<= 4 - param->bits;
        }
        if (param->bits > 4 && param->bits < 8) {
            init <<= 8 - param->bits;
        }
    }
    printf("static inline %s crc_init(void)\n"
           "{\n"
           "\treturn %s%" REGFMT ";\n"
           "}\n\n",
           crc_type,
           init ? "0x" : "",
           init);

    /* crc_next */
    printf("static inline %s crc_next(%s crc, uint8_t data)\n",
           crc_type, crc_type);
    if (param->bits > 8) {
        if (param->reverse) {
            printf("{\n"
                   "\tcrc ^= data;\n"
                   "\tcrc = (crc >> 4) ^ crc_table[crc & 15];\n"
                   "\tcrc = (crc >> 4) ^ crc_table[crc & 15];\n"
                   "\treturn crc;\n"
                   "}\n\n");
        } else {
            printf("{\n"
                   "\tcrc ^= (%s)data << %u;\n"
                   "\tcrc = (crc << 4) ^ crc_table[(crc >> %u) & 15];\n"
                   "\tcrc = (crc << 4) ^ crc_table[(crc >> %u) & 15];\n"
                   "\treturn crc;\n"
                   "}\n\n",
                   crc_type,
                   param->bits - 8,
                   param->bits - 4,
                   param->bits - 4);
        }
    } else if (param->bits > 4) {
        if (param->reverse) {
            printf("{\n"
                   "\tcrc ^= data;\n"
                   "\tcrc = (crc >> 4) ^ crc_table[crc & 15];\n"
                   "\tcrc = (crc >> 4) ^ crc_table[crc & 15];\n"
                   "\treturn crc;\n"
                   "}\n\n");
        } else {
            printf("{\n"
                   "\tcrc ^= data;\n"
                   "\tcrc = ((crc << 4) & 0xff) ^ crc_table[crc >> 4];\n"
                   "\tcrc = ((crc << 4) & 0xff) ^ crc_table[crc >> 4];\n"
                   "\treturn crc;\n"
                   "}\n\n");
        }
    } else {
        if (param->reverse) {
            printf("{\n"
                   "\tcrc = crc_table[crc ^ (data & 15)];\n"
                   "\tcrc = crc_table[crc ^ (data >> 4)];\n"
                   "\treturn crc;\n"
                   "}\n\n");
        } else {
            printf("{\n"
                   "\tcrc = crc_table[crc ^ (data >> 4)];\n"
                   "\tcrc = crc_table[crc ^ (data & 15)];\n"
                   "\treturn crc;\n"
                   "}\n\n");
        }
    }

    /* crc_final */
    {
        char post_mask[24] = "";
        const char *pre_xor = "";
        char post_xor[24] = "";
        const char *pre_shift = "";
        char post_shift[8] = "";
        if (!param->reverse &&
            crc_type_bits != param->bits &&
            param->bits > 8)
        {
            /* Masking */
            sprintf(post_mask, " & 0x%" REGFMT, mask);
        }
        if (param->eor) {
            /* XORing */
            if (param->eor == mask &&
                (post_mask[0] || crc_type_bits == param->bits))
            {
                pre_xor = "~";
            } else {
                pre_xor = "(";
                sprintf(post_xor, " ^ 0x%" REGFMT ")", param->eor);
            }
        }
        /* Shifting */
        if (!param->reverse) {
            if (param->bits < 4) {
                sprintf(post_shift, " >> %u", 4 - param->bits);
            }
            if (param->bits > 4 && param->bits < 8) {
                sprintf(post_shift, " >> %u", 8 - param->bits);
            }
        }
        printf("static inline %s crc_final(%s crc)\n"
               "{\n"
               "\treturn %s%scrc%s%s%s;\n"
               "}\n\n",
               crc_type,
               crc_type,
               pre_xor,
               pre_shift,
               post_shift,
               post_xor,
               post_mask);
    }

    printf("#endif\n\n");

    /*
     * Generate the C file
     */

    printf("/*\n * crc.c\n */\n\n");
    if (code_param->test_code) {
        printf("#include <stdio.h>\n\n");
        printf("#include <stdlib.h>\n\n");
    } else {
        printf("#include \"crc.h\"\n\n");
    }

    /* Generate the CRC table */
    per_line = 1;
    while (4 + (2 + print_width + 2) * (per_line * 2) <= 84) per_line *= 2;
    printf("const %s crc_table[16] = {\n", tab_type);
    for (i = 0; i < 16; i++) {
        struct crc_param p = *param;
        unsigned char data = (unsigned char)i;
        crc_reg_t crc;
        p.init = 0;
        p.eor = 0;
        if (!param->reverse) {
            data <<= 4;
        }
        crc = crc_calc(&data, 4, &p);
        /* Pre-shift for efficiency in crc_next */
        if (!param->reverse) {
            if (param->bits < 4) {
                crc <<= (4 - param->bits);
            }
            if (param->bits > 4 && param->bits < 8) {
                crc <<= (8 - param->bits);
            }
        }
        if (i % per_line == 0) printf("\t");
        printf("0x%0*" REGFMT, print_width, crc);
        tab_post_value(i, 16, per_line);
    }
    printf("\n};\n\n");

    /*
     * Generate the crc_calc function
     */
    printf("%s crc_calc(const uint8_t *data, size_t len)\n"
           "{\n\t%s crc = crc_init();\n\n"
           "\tif (len) do {\n"
           "\t\tcrc = crc_next(crc, *data++);\n"
           "\t} while (--len);\n\n"
           "\treturn crc_final(crc);\n"
           "}\n",
           crc_type, crc_type);

    /* Generate test code */
    if (code_param->test_code) {
        gen_test_code(param, crc_type);
    }
}

static void gen_code_tab16i(const struct crc_param *param,
                            const struct code_param *code_param)
{
    const char *crc_type;
    const char *tab_type;
    unsigned int crc_type_bits;
    crc_reg_t init = init_to_direct(param);
    crc_reg_t top_bit = (crc_reg_t)1 << (param->bits - 1);
    crc_reg_t mask = top_bit | (top_bit - 1);
    unsigned int print_width = (param->bits + 7) / 8 * 2;
    unsigned int i;
    unsigned int per_line;

    get_types(param, code_param, &crc_type, &tab_type, &crc_type_bits, 0);

    /*
     * Generate the header file
     */

    gen_header_comment(param);
    printf("#ifndef CRC_H_\n"
           "#define CRC_H_\n\n"
           "#include <stddef.h>\n");
    printf("#include <stdint.h>\n");
    printf("\n%s crc_calc(const uint8_t *data, size_t len);\n\n", crc_type);
    printf("extern const %s crc_table[32];\n\n", tab_type);

    /* crc_init */
    if (!param->reverse) {
        if (param->bits < 8) {
            init <<= 8 - param->bits;
        }
    }
    printf("static inline %s crc_init(void)\n"
           "{\n"
           "\treturn %s%" REGFMT ";\n"
           "}\n\n",
           crc_type,
           init ? "0x" : "",
           init);

    /* crc_next */
    printf("static inline %s crc_next(%s crc, uint8_t data)\n",
           crc_type, crc_type);
    if (param->bits > 8) {
        if (param->reverse) {
            printf("{\n"
                   "\tcrc ^= data;\n"
                   "\tcrc =\n"
                   "\t\t(crc >> 8) ^\n"
                   "\t\tcrc_table[(crc >> 4) & 15] ^\n"
                   "\t\tcrc_table[(crc & 15) + 16];\n"
                   "\treturn crc;\n"
                   "}\n\n");
        } else {
            printf("{\n"
                   "\tcrc ^= (%s)data << %u;\n"
                   "\tcrc =\n"
                   "\t\t(crc << 8) ^\n"
                   "\t\tcrc_table[(crc >> %u) & 15] ^\n"
                   "\t\tcrc_table[((crc >> %u) & 15) + 16];\n"
                   "\treturn crc;\n"
                   "}\n\n",
                   crc_type,
                   param->bits - 8,
                   param->bits - 8,
                   param->bits - 4);
        }
    } else {
        if (param->reverse) {
            printf("{\n"
                   "\tcrc ^= data;\n"
                   "\tcrc =\n"
                   "\t\tcrc_table[crc >> 4] ^\n"
                   "\t\tcrc_table[(crc & 15) + 16];\n"
                   "\treturn crc;\n"
                   "}\n\n");
        } else {
            printf("{\n"
                   "\tcrc ^= data;\n"
                   "\tcrc =\n"
                   "\t\tcrc_table[crc & 15] ^\n"
                   "\t\tcrc_table[(crc >> 4) + 16];\n"
                   "\treturn crc;\n"
                   "}\n\n");
        }
    }

    /* crc_final */
    {
        char post_mask[24] = "";
        const char *pre_xor = "";
        char post_xor[24] = "";
        const char *pre_shift = "";
        char post_shift[8] = "";
        if (!param->reverse &&
            crc_type_bits != param->bits &&
            param->bits > 8)
        {
            /* Masking */
            sprintf(post_mask, " & 0x%" REGFMT, mask);
        }
        if (param->eor) {
            /* XORing */
            if (param->eor == mask &&
                (post_mask[0] || crc_type_bits == param->bits))
            {
                pre_xor = "~";
            } else {
                pre_xor = "(";
                sprintf(post_xor, " ^ 0x%" REGFMT ")", param->eor);
            }
        }
        /* Shifting */
        if (!param->reverse) {
            if (param->bits < 8) {
                sprintf(post_shift, " >> %u", 8 - param->bits);
            }
        }
        printf("static inline %s crc_final(%s crc)\n"
               "{\n"
               "\treturn %s%scrc%s%s%s;\n"
               "}\n\n",
               crc_type,
               crc_type,
               pre_xor,
               pre_shift,
               post_shift,
               post_xor,
               post_mask);
    }

    printf("#endif\n\n");

    /*
     * Generate the C file
     */

    printf("/*\n * crc.c\n */\n\n");
    if (code_param->test_code) {
        printf("#include <stdio.h>\n\n");
        printf("#include <stdlib.h>\n\n");
    } else {
        printf("#include \"crc.h\"\n\n");
    }

    /* Generate the CRC table */
    per_line = 1;
    while (4 + (2 + print_width + 2) * (per_line * 2) <= 84) per_line *= 2;
    printf("const %s crc_table[32] = {\n", tab_type);
    for (i = 0; i < 32; i++) {
        struct crc_param p = *param;
        unsigned char data = (i & 15) << (!param->reverse * 4);
        crc_reg_t crc;
        p.init = 0;
        p.eor = 0;
        crc = crc_calc(&data, (i / 16 + 1) * 4, &p);
        /* Pre-shift for efficiency in crc_next */
        if (!param->reverse) {
            if (param->bits < 8) {
                crc <<= (8 - param->bits);
            }
        }
        if (i % per_line == 0) printf("\t");
        printf("0x%0*" REGFMT, print_width, crc);
        tab_post_value(i, 32, per_line);
    }
    printf("\n};\n\n");

    /*
     * Generate the crc_calc function
     */
    printf("%s crc_calc(const uint8_t *data, size_t len)\n"
           "{\n\t%s crc = crc_init();\n\n"
           "\tif (len) do {\n"
           "\t\tcrc = crc_next(crc, *data++);\n"
           "\t} while (--len);\n\n"
           "\treturn crc_final(crc);\n"
           "}\n",
           crc_type, crc_type);

    /* Generate test code */
    if (code_param->test_code) {
        gen_test_code(param, crc_type);
    }
}

static void gen_code_tabw(const struct crc_param *param,
                          const struct code_param *code_param)
{
    const char *crc_type;
    const char *tab_type;
    unsigned int crc_type_bits;
    unsigned int crc_shift;
    crc_reg_t init = init_to_direct(param);
    crc_reg_t top_bit = (crc_reg_t)1 << (param->bits - 1);
    crc_reg_t mask = top_bit | (top_bit - 1);
    unsigned int print_width;
    unsigned int i;
    unsigned int per_line;

    print_width = 2;
    while (4 * print_width < param->bits) print_width *= 2;

    get_types(param, code_param, &crc_type, &tab_type, &crc_type_bits, &crc_shift);

    /*
     * Generate the header file
     */

    gen_header_comment(param);
    printf("#ifndef CRC_H_\n"
           "#define CRC_H_\n\n"
#if defined(TEST_BIG_ENDIAN)
           "#undef ARCH_LITTLE_ENDIAN\n\n"
#elif defined(TEST_LITTLE_ENDIAN)
           "#define ARCH_LITTLE_ENDIAN\n\n"
#endif
           "#include <stddef.h>\n");
    printf("#include <stdint.h>\n");
    printf("\n%s crc_calc(const uint8_t *data, size_t len);\n\n", crc_type);
    printf("extern const %s crc_table[256];\n\n", tab_type);

    /* crc_init */
    {
        crc_reg_t init_shift = param->reverse ? init : init << crc_shift;
        crc_reg_t init_bsw = byte_swap(init_shift, param->bits);
        crc_reg_t init_le = param->reverse ? init_shift : init_bsw;
        crc_reg_t init_be = param->reverse ? init_bsw : init_shift;
        if (init_le != init_be) {
            printf("static inline %s crc_init(void)\n"
                   "{\n"
                   "#ifdef ARCH_LITTLE_ENDIAN\n"
                   "\treturn 0x%" REGFMT ";\n"
                   "#else\n"
                   "\treturn 0x%" REGFMT ";\n"
                   "#endif\n"
                   "}\n\n",
                   crc_type,
                   init_le,
                   init_be);
        } else {
            printf("static inline %s crc_init(void)\n"
                   "{\n"
                   "\treturn %s%" REGFMT ";\n"
                   "}\n\n",
                   crc_type,
                   init_le ? "0x" : "",
                   init_le);
        }
    }

    /* crc_next */
    printf("static inline %s crc_next(%s crc, uint8_t data)\n",
           crc_type, crc_type);
    if (param->bits > 8) {
        printf("{\n"
               "#ifdef ARCH_LITTLE_ENDIAN\n"
               "\treturn (crc >> 8) ^ crc_table[(crc & 0xff) ^ data];\n"
               "#else\n"
               "\treturn (crc << 8) ^ crc_table[((crc >> %u) & 0xff) ^ data];\n"
               "#endif\n"
               "}\n\n",
               param->bits + crc_shift - 8);
    } else {
        printf("{\n"
               "\treturn crc_table[crc ^ data];\n"
               "}\n\n");
    }

    /* crc_next4 */
    printf("/*\n"
           " * Process 4 bytes in one go\n"
           " * The data parameter must contain all 4 bytes;\n"
           " *   On big-endian machines: first message byte is the most significant byte\n"
           " *   On little-endian machines: first message byte is the least significant byte\n"
           " */\n"
           "static inline %s crc_next4(%s crc, uint32_t data)\n"
           "{\n",
           crc_type, crc_type);
    if (param->bits <= 8) {
        printf("#ifdef ARCH_LITTLE_ENDIAN\n"
               "\tcrc = crc_table[(crc ^ data) & 0xff];\n"
               "\tcrc = crc_table[(crc ^ (data >> 8)) & 0xff];\n"
               "\tcrc = crc_table[(crc ^ (data >> 16)) & 0xff];\n"
               "\tcrc = crc_table[(crc ^ (data >> 24)) & 0xff];\n"
               "#else\n"
               "\tcrc = crc_table[(crc ^ (data >> 24)) & 0xff];\n"
               "\tcrc = crc_table[(crc ^ (data >> 16)) & 0xff];\n"
               "\tcrc = crc_table[(crc ^ (data >> 8)) & 0xff];\n"
               "\tcrc = crc_table[(crc ^ data) & 0xff];\n"
               "#endif\n");
    } else if (param->bits <= 16) {
        printf("#ifdef ARCH_LITTLE_ENDIAN\n"
               "\tuint32_t crc_w = crc ^ data;\n"
               "\tcrc_w = (crc_w >> 8) ^ crc_table[crc_w & 0xff];\n"
               "\tcrc_w = (crc_w >> 8) ^ crc_table[crc_w & 0xff];\n"
               "\tcrc_w = (crc_w >> 8) ^ crc_table[crc_w & 0xff];\n"
               "\tcrc = (%s)((crc_w >> 8) ^ crc_table[crc_w & 0xff]);\n"
               "#else\n"
               "\tcrc ^= (%s)(data >> 16);\n"
               "\tcrc = (crc << 8) ^ crc_table[(crc >> 8) & 0xff];\n"
               "\tcrc = (crc << 8) ^ crc_table[(crc >> 8) & 0xff];\n"
               "\tcrc ^= (%s)data;\n"
               "\tcrc = (crc << 8) ^ crc_table[(crc >> 8) & 0xff];\n"
               "\tcrc = (crc << 8) ^ crc_table[(crc >> 8) & 0xff];\n"
               "#endif\n",
               crc_type, crc_type, crc_type);
    } else if (param->bits <= 32) {
        printf("#ifdef ARCH_LITTLE_ENDIAN\n"
               "\tcrc ^= data;\n"
               "\tcrc = (crc >> 8) ^ crc_table[crc & 0xff];\n"
               "\tcrc = (crc >> 8) ^ crc_table[crc & 0xff];\n"
               "\tcrc = (crc >> 8) ^ crc_table[crc & 0xff];\n"
               "\tcrc = (crc >> 8) ^ crc_table[crc & 0xff];\n"
               "#else\n"
               "\tcrc ^= data;\n"
               "\tcrc = (crc << 8) ^ crc_table[(crc >> 24) & 0xff];\n"
               "\tcrc = (crc << 8) ^ crc_table[(crc >> 24) & 0xff];\n"
               "\tcrc = (crc << 8) ^ crc_table[(crc >> 24) & 0xff];\n"
               "\tcrc = (crc << 8) ^ crc_table[(crc >> 24) & 0xff];\n"
               "#endif\n");
    } else {
        printf("#ifdef ARCH_LITTLE_ENDIAN\n"
               "\tcrc ^= data;\n"
               "\tcrc = (crc >> 8) ^ crc_table[crc & 0xff];\n"
               "\tcrc = (crc >> 8) ^ crc_table[crc & 0xff];\n"
               "\tcrc = (crc >> 8) ^ crc_table[crc & 0xff];\n"
               "\tcrc = (crc >> 8) ^ crc_table[crc & 0xff];\n"
               "#else\n"
               "\tcrc ^= (%s)data << 32;\n"
               "\tcrc = (crc << 8) ^ crc_table[(crc >> 56) & 0xff];\n"
               "\tcrc = (crc << 8) ^ crc_table[(crc >> 56) & 0xff];\n"
               "\tcrc = (crc << 8) ^ crc_table[(crc >> 56) & 0xff];\n"
               "\tcrc = (crc << 8) ^ crc_table[(crc >> 56) & 0xff];\n"
               "#endif\n",
               crc_type);
    }
    printf("\treturn crc;\n"
           "}\n\n");

    /* crc_final */
    {
        char post_mask[24] = "";
        const char *pre_xor = "";
        char post_xor[24] = "";
        const char *pre_shift = "";
        char post_shift[8] = "";
        if (crc_type_bits != param->bits && param->bits > 8) {
            /* Masking */
            sprintf(post_mask, " & 0x%" REGFMT, mask);
        }
        if (param->eor) {
            /* XORing */
            if (param->eor == mask &&
                (post_mask[0] || crc_type_bits == param->bits))
            {
                pre_xor = "~";
            } else {
                pre_xor = "(";
                sprintf(post_xor, " ^ 0x%" REGFMT ")", param->eor);
            }
        }
        if (crc_shift && !param->reverse) {
            /* Shifting */
            pre_shift = "(";
            sprintf(post_shift, " >> %u)", crc_shift);
        }
        printf("static inline %s crc_final(%s crc)\n"
               "{\n",
               crc_type,
               crc_type);
        if (param->bits > 8) {
            printf("#if%sdef ARCH_LITTLE_ENDIAN\n",
                   param->reverse ? "n" : "");
            if (param->bits <= 16) {
                printf("\tcrc = (crc >> 8) | ((crc & 0xff) << 8);\n");
            } else if (param->bits <= 32) {
                printf("\tcrc = (crc >> 16) | ((crc & 0xffff) << 16);\n"
                       "\tcrc = ((crc >> 8) & 0xff00ff) | ((crc & 0xff00ff) << 8);\n");
            } else {
                printf("\tcrc = (crc >> 32) | ((crc & 0xffffffff) << 32);\n"
                       "\tcrc = ((crc >> 16) & 0xffff0000ffff) | ((crc & 0xffff0000ffff) << 16);\n"
                       "\tcrc = ((crc >> 8) & 0xff00ff00ff00ff) | ((crc & 0xff00ff00ff00ff) << 8);\n");
            }
            printf("#endif\n");
        }
        printf("\treturn %s%scrc%s%s%s;\n"
               "}\n\n",
               pre_xor,
               pre_shift,
               post_shift,
               post_xor,
               post_mask);
    }

    printf("#endif\n\n");

    /*
     * Generate the C file
     */

    printf("/*\n * crc.c\n */\n\n");
    if (code_param->test_code) {
        printf("#include <stdio.h>\n");
        printf("#include <stdlib.h>\n\n");
    } else {
        printf("#include \"crc.h\"\n\n");
    }

    /* Generate the CRC table */
    per_line = 1;
    while (4 + (2 + print_width + 2) * (per_line * 2) <= 84) {
        per_line *= 2;
    }
    printf("const %s crc_table[256] = {\n", tab_type);
    if (param->bits > 8) printf("#ifdef ARCH_LITTLE_ENDIAN\n");
    for (i = 0; i < (param->bits > 8 ? 512 : 256); i++) {
        struct crc_param p = *param;
        uint8_t data = i & 0xff;
        crc_reg_t crc;
        p.init = 0;
        p.eor = 0;
        crc = crc_calc(&data, 8, &p);
        if (!param->reverse) crc <<= crc_shift;
        if (((i < 256 && !param->reverse) ||
             (i >= 256 && param->reverse)))
        {
            crc = byte_swap(crc, param->bits);
        }
        if (i % per_line == 0) printf("\t");
        printf("0x%0*" REGFMT, print_width, crc);
        tab_post_value(i % 256, 256, per_line);
        if (i == 255 && param->bits > 8) printf("\n#else\n");
    }
    if (param->bits > 8) printf("\n#endif");
    printf("\n};\n\n");

    /* crc_calc */
    printf("%s crc_calc(const uint8_t *data, size_t len)\n"
           "{\n"
           "\t%s crc;\n\n"
           "\tcrc = crc_init();\n\n"
           "\twhile (((uintptr_t)data & 3) && len) {\n"
           "\t\tcrc = crc_next(crc, *data++);\n"
           "\t\tlen--;\n"
           "\t}\n\n"
           "\twhile (len >= 16) {\n"
           "\t\tlen -= 16;\n"
#if defined(TEST_BIG_ENDIAN)
           "\t\tcrc = crc_next4(crc, data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]);\n"
           "\t\tdata += 4;\n"
           "\t\tcrc = crc_next4(crc, data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]);\n"
           "\t\tdata += 4;\n"
           "\t\tcrc = crc_next4(crc, data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]);\n"
           "\t\tdata += 4;\n"
           "\t\tcrc = crc_next4(crc, data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]);\n"
           "\t\tdata += 4;\n"
#elif defined(TEST_LITTLE_ENDIAN)
           "\t\tcrc = crc_next4(crc, data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0]);\n"
           "\t\tdata += 4;\n"
           "\t\tcrc = crc_next4(crc, data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0]);\n"
           "\t\tdata += 4;\n"
           "\t\tcrc = crc_next4(crc, data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0]);\n"
           "\t\tdata += 4;\n"
           "\t\tcrc = crc_next4(crc, data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0]);\n"
           "\t\tdata += 4;\n"
#else
           "\t\tcrc = crc_next4(crc, ((uint32_t *)data)[0]);\n"
           "\t\tcrc = crc_next4(crc, ((uint32_t *)data)[1]);\n"
           "\t\tcrc = crc_next4(crc, ((uint32_t *)data)[2]);\n"
           "\t\tcrc = crc_next4(crc, ((uint32_t *)data)[3]);\n"
           "\t\tdata += 16;\n"
#endif
           "\t}\n\n"
           "\twhile (len--) {\n"
           "\t\tcrc = crc_next(crc, *data++);\n"
           "\t}\n\n"
           "\treturn crc_final(crc);\n"
           "}\n\n",
           crc_type, crc_type);

    /* Generate test code */
    if (code_param->test_code) {
        gen_test_code(param, crc_type);
    }
}    

static void gen_code_tabi(const struct crc_param *param,
                          const struct code_param *code_param)
{
    const char *crc_type;
    const char *tab_type;
    unsigned int crc_type_bits;
    unsigned int crc_shift;
    crc_reg_t init = init_to_direct(param);
    crc_reg_t top_bit = (crc_reg_t)1 << (param->bits - 1);
    crc_reg_t mask = top_bit | (top_bit - 1);
    unsigned int print_width;
    unsigned int i;
    unsigned int per_line;

    print_width = 2;
    while (4 * print_width < param->bits) print_width *= 2;

    get_types(param, code_param, &crc_type, &tab_type, &crc_type_bits, &crc_shift);

    /*
     * Generate the header file
     */

    gen_header_comment(param);
    printf("#ifndef CRC_H_\n"
           "#define CRC_H_\n\n"
#if defined(TEST_BIG_ENDIAN)
           "#undef ARCH_LITTLE_ENDIAN\n\n"
#elif defined(TEST_LITTLE_ENDIAN)
           "#define ARCH_LITTLE_ENDIAN\n\n"
#endif
           "#include <stddef.h>\n");
    printf("#include <stdint.h>\n");
    printf("\n%s crc_calc(const uint8_t *data, size_t len);\n\n", crc_type);
    printf("extern const %s crc_table[1024];\n\n", tab_type);

    /* crc_init */
    if (code_param->algo == ALGO_TABIW) {
        crc_reg_t init_shift = param->reverse ? init : init << crc_shift;
        crc_reg_t init_bsw = byte_swap(init_shift, param->bits);
        crc_reg_t init_le = param->reverse ? init_shift : init_bsw;
        crc_reg_t init_be = param->reverse ? init_bsw : init_shift;
        if (init_le != init_be) {
            printf("static inline %s crc_init(void)\n"
                   "{\n"
                   "#ifdef ARCH_LITTLE_ENDIAN\n"
                   "\treturn 0x%" REGFMT ";\n"
                   "#else\n"
                   "\treturn 0x%" REGFMT ";\n"
                   "#endif\n"
                   "}\n\n",
                   crc_type,
                   init_le,
                   init_be);
        } else {
            printf("static inline %s crc_init(void)\n"
                   "{\n"
                   "\treturn %s%" REGFMT ";\n"
                   "}\n\n",
                   crc_type,
                   init_le ? "0x" : "",
                   init_le);
        }
    } else {
        printf("static inline %s crc_init(void)\n"
               "{\n"
               "\treturn %s%" REGFMT ";\n"
               "}\n\n",
               crc_type,
               init ? "0x" : "",
               param->reverse ? init : init << crc_shift);
    }

    /* crc_next */
    printf("static inline %s crc_next(%s crc, uint8_t data)\n",
           crc_type, crc_type);
    if (param->bits > 8) {
        if (code_param->algo == ALGO_TABIW) {
            printf("{\n"
                   "#ifdef ARCH_LITTLE_ENDIAN\n"
                   "\treturn (crc >> 8) ^ crc_table[(crc & 0xff) ^ data];\n"
                   "#else\n"
                   "\treturn (crc << 8) ^ crc_table[((crc >> %u) & 0xff) ^ data];\n"
                   "#endif\n"
                   "}\n\n",
                   param->bits + crc_shift - 8);
        } else {
            if (param->reverse) {
                printf("{\n"
                       "\treturn (crc >> 8) ^ crc_table[(crc & 0xff) ^ data];\n"
                       "}\n\n");
            } else {
                printf("{\n"
                       "\treturn (crc << 8) ^ crc_table[((crc >> %u) & 0xff) ^ data];\n"
                       "}\n\n",
                       param->bits + crc_shift - 8);
            }
        }
    } else {
        printf("{\n"
               "\treturn crc_table[crc ^ data];\n"
               "}\n\n");
    }

    /* crc_next4 */
    if (code_param->algo == ALGO_TABIW) {
        printf("/*\n"
               " * Process 4 bytes in one go\n"
               " * The data parameter must contain all 4 bytes;\n"
               " *   On big-endian machines: first message byte is the most significant byte\n"
               " *   On little-endian machines: first message byte is the least significant byte\n"
               " */\n"
               "static inline %s crc_next4(%s crc, uint32_t data)\n"
               "{\n",
               crc_type, crc_type);
        if (param->bits <= 8) {
            printf("#ifdef ARCH_LITTLE_ENDIAN\n"
                   "\tcrc ^= data & 0xff;\n"
                   "\tcrc =\n"
                   "\t\tcrc_table[(crc & 0xff) + 0x300] ^\n"
                   "\t\tcrc_table[((data >> 8) & 0xff) + 0x200] ^\n"
                   "\t\tcrc_table[((data >> 16) & 0xff) + 0x100] ^\n"
                   "\t\tcrc_table[data >> 24];\n"
                   "#else\n"
                   "\tcrc ^= data >> 24;\n"
                   "\tcrc =\n"
                   "\t\tcrc_table[data & 0xff] ^\n"
                   "\t\tcrc_table[((data >> 8) & 0xff) + 0x100] ^\n"
                   "\t\tcrc_table[((data >> 16) & 0xff) + 0x200] ^\n"
                   "\t\tcrc_table[(crc & 0xff) + 0x300];\n"
                   "#endif\n");
        } else if (param->bits <= 16) {
            printf("#ifdef ARCH_LITTLE_ENDIAN\n"
                   "\tcrc ^= data & 0xffff;\n"
                   "\tcrc =\n"
                   "\t\tcrc_table[(crc & 0xff) + 0x300] ^\n"
                   "\t\tcrc_table[((crc >> 8) & 0xff) + 0x200] ^\n"
                   "\t\tcrc_table[((data >> 16) & 0xff) + 0x100] ^\n"
                   "\t\tcrc_table[data >> 24];\n"
                   "#else\n"
                   "\tcrc ^= data >> 16;\n"
                   "\tcrc =\n"
                   "\t\tcrc_table[data & 0xff] ^\n"
                   "\t\tcrc_table[((data >> 8) & 0xff) + 0x100] ^\n"
                   "\t\tcrc_table[(crc & 0xff) + 0x200] ^\n"
                   "\t\tcrc_table[((crc >> 8) & 0xff) + 0x300];\n"
                   "#endif\n");
        } else if (param->bits <= 32) {
            printf("#ifdef ARCH_LITTLE_ENDIAN\n"
                   "\tcrc ^= data;\n"
                   "\tcrc =\n"
                   "\t\tcrc_table[(crc & 0xff) + 0x300] ^\n"
                   "\t\tcrc_table[((crc >> 8) & 0xff) + 0x200] ^\n"
                   "\t\tcrc_table[((crc >> 16) & 0xff) + 0x100] ^\n"
                   "\t\tcrc_table[(crc >> 24) & 0xff];\n"
                   "#else\n"
                   "\tcrc ^= data;\n"
                   "\tcrc =\n"
                   "\t\tcrc_table[crc & 0xff] ^\n"
                   "\t\tcrc_table[((crc >> 8) & 0xff) + 0x100] ^\n"
                   "\t\tcrc_table[((crc >> 16) & 0xff) + 0x200] ^\n"
                   "\t\tcrc_table[((crc >> 24) & 0xff) + 0x300];\n"
                   "#endif\n");
        } else {
            printf("#ifdef ARCH_LITTLE_ENDIAN\n"
                   "\tcrc ^= data;\n"
                   "\tcrc = (crc >> 32) ^\n"
                   "\t\tcrc_table[(crc & 0xff) + 0x300] ^\n"
                   "\t\tcrc_table[((crc >> 8) & 0xff) + 0x200] ^\n"
                   "\t\tcrc_table[((crc >> 16) & 0xff) + 0x100] ^\n"
                   "\t\tcrc_table[(crc >> 24) & 0xff];\n"
                   "#else\n"
                   "\tcrc ^= (%s)data << 32;\n"
                   "\tcrc = (crc << 32) ^\n"
                   "\t\tcrc_table[(crc >> 32) & 0xff] ^\n"
                   "\t\tcrc_table[((crc >> 40) & 0xff) + 0x100] ^\n"
                   "\t\tcrc_table[((crc >> 48) & 0xff) + 0x200] ^\n"
                   "\t\tcrc_table[((crc >> 56) & 0xff) + 0x300];\n"
                   "#endif\n",
                   crc_type);
        }
    } else {
        printf("/*\n"
               " * Process 4 bytes in one go\n"
               " */\n"
               "static inline %s crc_next4(%s crc, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3)\n"
               "{\n",
               crc_type, crc_type);
        if (param->bits <= 8) {
            printf("\tcrc =\n"
                   "\t\tcrc_table[d3] ^\n"
                   "\t\tcrc_table[d2 + 0x100] ^\n"
                   "\t\tcrc_table[d1 + 0x200] ^\n"
                   "\t\tcrc_table[((crc & 0xff) ^ d0) + 0x300];\n");
        } else if (param->bits <= 16) {
            if (param->reverse) {
                printf("\tcrc =\n"
                       "\t\tcrc_table[d3] ^\n"
                       "\t\tcrc_table[d2 + 0x100] ^\n"
                       "\t\tcrc_table[(((crc >> 8) & 0xff) ^ d1) + 0x200] ^\n"
                       "\t\tcrc_table[((crc & 0xff) ^ d0) + 0x300];\n");
            } else {
                printf("\tcrc =\n"
                       "\t\tcrc_table[d3] ^\n"
                       "\t\tcrc_table[d2 + 0x100] ^\n"
                       "\t\tcrc_table[((crc & 0xff) ^ d1) + 0x200] ^\n"
                       "\t\tcrc_table[(((crc >> 8) & 0xff) ^ d0) + 0x300];\n");
            }
        } else if (param->bits <= 32) {
            if (param->reverse) {
                printf("\tcrc =\n"
                       "\t\tcrc_table[((crc & 0xff) ^ d0) + 0x300] ^\n"
                       "\t\tcrc_table[(((crc >> 8) & 0xff) ^ d1) + 0x200] ^\n"
                       "\t\tcrc_table[(((crc >> 16) & 0xff) ^ d2) + 0x100] ^\n"
                       "\t\tcrc_table[(((crc >> 24) & 0xff) ^ d3)];\n");
            } else {
                printf("\tcrc =\n"
                       "\t\tcrc_table[(crc & 0xff) ^ d3] ^\n"
                       "\t\tcrc_table[(((crc >> 8) & 0xff) ^ d2) + 0x100] ^\n"
                       "\t\tcrc_table[(((crc >> 16) & 0xff) ^ d1) + 0x200] ^\n"
                       "\t\tcrc_table[(((crc >> 24) & 0xff) ^ d0) + 0x300];\n");
            }
        } else {
            if (param->reverse) {
                printf("\tcrc = (crc >> 32) ^\n"
                       "\t\tcrc_table[(((crc >> 24) & 0xff) ^ d3)] ^\n"
                       "\t\tcrc_table[(((crc >> 16) & 0xff) ^ d2) + 0x100] ^\n"
                       "\t\tcrc_table[(((crc >> 8) & 0xff) ^ d1) + 0x200] ^\n"
                       "\t\tcrc_table[((crc & 0xff) ^ d0) + 0x300];\n");
            } else {
                printf("\tcrc = (crc << 32) ^\n"
                       "\t\tcrc_table[((crc >> 32) & 0xff) ^ d3] ^\n"
                       "\t\tcrc_table[(((crc >> 40) & 0xff) ^ d2) + 0x100] ^\n"
                       "\t\tcrc_table[(((crc >> 48) & 0xff) ^ d1) + 0x200] ^\n"
                       "\t\tcrc_table[(((crc >> 56) & 0xff) ^ d0) + 0x300];\n");
            }
        }
    }
    printf("\treturn crc;\n"
           "}\n\n");

    /* crc_final */
    {
        char post_mask[24] = "";
        const char *pre_xor = "";
        char post_xor[24] = "";
        const char *pre_shift = "";
        char post_shift[8] = "";
        if (crc_type_bits != param->bits && param->bits > 8) {
            /* Masking */
            sprintf(post_mask, " & 0x%" REGFMT, mask);
        }
        if (param->eor) {
            /* XORing */
            if (param->eor == mask &&
                (post_mask[0] || crc_type_bits == param->bits))
            {
                pre_xor = "~";
            } else {
                pre_xor = "(";
                sprintf(post_xor, " ^ 0x%" REGFMT ")", param->eor);
            }
        }
        if (crc_shift && !param->reverse) {
            /* Shifting */
            pre_shift = "(";
            sprintf(post_shift, " >> %u)", crc_shift);
        }
        printf("static inline %s crc_final(%s crc)\n"
               "{\n",
               crc_type,
               crc_type);
        if (param->bits > 8 && code_param->algo == ALGO_TABIW) {
            printf("#if%sdef ARCH_LITTLE_ENDIAN\n",
                   param->reverse ? "n" : "");
            if (param->bits <= 16) {
                printf("\tcrc = (crc >> 8) | ((crc & 0xff) << 8);\n");
            } else if (param->bits <= 32) {
                printf("\tcrc = (crc >> 16) | ((crc & 0xffff) << 16);\n"
                       "\tcrc = ((crc >> 8) & 0xff00ff) | ((crc & 0xff00ff) << 8);\n");
            } else {
                printf("\tcrc = (crc >> 32) | ((crc & 0xffffffff) << 32);\n"
                       "\tcrc = ((crc >> 16) & 0xffff0000ffff) | ((crc & 0xffff0000ffff) << 16);\n"
                       "\tcrc = ((crc >> 8) & 0xff00ff00ff00ff) | ((crc & 0xff00ff00ff00ff) << 8);\n");
            }
            printf("#endif\n");
        }
        printf("\treturn %s%scrc%s%s%s;\n"
               "}\n\n",
               pre_xor,
               pre_shift,
               post_shift,
               post_xor,
               post_mask);
    }

    printf("#endif\n\n");

    /*
     * Generate the C file
     */

    printf("/*\n * crc.c\n */\n\n");
    if (code_param->test_code) {
        printf("#include <stdio.h>\n");
        printf("#include <stdlib.h>\n\n");
    } else {
        printf("#include \"crc.h\"\n\n");
    }

    /* Generate the CRC table */
    per_line = 1;
    while (4 + (2 + print_width + 2) * (per_line * 2) <= 84) {
        per_line *= 2;
    }
    printf("const %s crc_table[1024] = {\n", tab_type);
    if (param->bits > 8 && code_param->algo == ALGO_TABIW) {
        printf("#ifdef ARCH_LITTLE_ENDIAN\n");
    }
    for (i = 0;
         i < (param->bits > 8 && code_param->algo == ALGO_TABIW ? 2048 : 1024);
         i++)
    {
        struct crc_param p = *param;
        unsigned char data[4] = { 0, 0, 0, 0 };
        crc_reg_t crc;
        data[0] = i & 0xff;
        p.init = 0;
        p.eor = 0;
        crc = crc_calc(data, ((i / 256) % 4 + 1) * 8, &p);
        if (!param->reverse) crc <<= crc_shift;
        if (code_param->algo == ALGO_TABIW &&
            ((i < 1024 && !param->reverse) ||
             (i >= 1024 && param->reverse)))
        {
            crc = byte_swap(crc, param->bits);
        }
        if (i % per_line == 0) printf("\t");
        printf("0x%0*" REGFMT, print_width, crc);
        tab_post_value(i % 1024, 1024, per_line);
        if (i == 1023 && (param->bits > 8 && code_param->algo == ALGO_TABIW)) {
            printf("\n#else\n");
        }
    }
    if (param->bits > 8 && code_param->algo == ALGO_TABIW) {
        printf("\n#endif");
    }
    printf("\n};\n\n");

    /* crc_calc */
    if (code_param->algo == ALGO_TABIW) {
        printf("%s crc_calc(const uint8_t *data, size_t len)\n"
               "{\n"
               "\t%s crc;\n\n"
               "\tcrc = crc_init();\n\n"
               "\twhile (((uintptr_t)data & 3) && len) {\n"
               "\t\tcrc = crc_next(crc, *data++);\n"
               "\t\tlen--;\n"
               "\t}\n\n"
               "\twhile (len >= 16) {\n"
               "\t\tlen -= 16;\n"
#if defined(TEST_BIG_ENDIAN)
               "\t\tcrc = crc_next4(crc, data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]);\n"
               "\t\tdata += 4;\n"
               "\t\tcrc = crc_next4(crc, data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]);\n"
               "\t\tdata += 4;\n"
               "\t\tcrc = crc_next4(crc, data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]);\n"
               "\t\tdata += 4;\n"
               "\t\tcrc = crc_next4(crc, data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3]);\n"
               "\t\tdata += 4;\n"
#elif defined(TEST_LITTLE_ENDIAN)
               "\t\tcrc = crc_next4(crc, data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0]);\n"
               "\t\tdata += 4;\n"
               "\t\tcrc = crc_next4(crc, data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0]);\n"
               "\t\tdata += 4;\n"
               "\t\tcrc = crc_next4(crc, data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0]);\n"
               "\t\tdata += 4;\n"
               "\t\tcrc = crc_next4(crc, data[3] << 24 | data[2] << 16 | data[1] << 8 | data[0]);\n"
               "\t\tdata += 4;\n"
#else
               "\t\tcrc = crc_next4(crc, ((uint32_t *)data)[0]);\n"
               "\t\tcrc = crc_next4(crc, ((uint32_t *)data)[1]);\n"
               "\t\tcrc = crc_next4(crc, ((uint32_t *)data)[2]);\n"
               "\t\tcrc = crc_next4(crc, ((uint32_t *)data)[3]);\n"
               "\t\tdata += 16;\n"
#endif
               "\t}\n\n"
               "\twhile (len--) {\n"
               "\t\tcrc = crc_next(crc, *data++);\n"
               "\t}\n\n"
               "\treturn crc_final(crc);\n"
               "}\n\n",
               crc_type, crc_type);
    } else {
        printf("%s crc_calc(const uint8_t *data, size_t len)\n"
               "{\n"
               "\t%s crc;\n\n"
               "\tcrc = crc_init();\n\n"
               "\twhile (len >= 16) {\n"
               "\t\tlen -= 16;\n"
               "\t\tcrc = crc_next4(crc, data[0], data[1], data[2], data[3]);\n"
               "\t\tcrc = crc_next4(crc, data[4], data[5], data[6], data[7]);\n"
               "\t\tcrc = crc_next4(crc, data[8], data[9], data[10], data[11]);\n"
               "\t\tcrc = crc_next4(crc, data[12], data[13], data[14], data[15]);\n"
               "\t\tdata += 16;\n"
               "\t}\n\n"
               "\twhile (len--) {\n"
               "\t\tcrc = crc_next(crc, *data++);\n"
               "\t}\n\n"
               "\treturn crc_final(crc);\n"
               "}\n\n",
               crc_type, crc_type);
    }

    /* Generate test code */
    if (code_param->test_code) {
        gen_test_code(param, crc_type);
    }
}    

/*
 * Usage
 */

static int usage(void)
{
    fprintf(stderr,
            "universal_crc version " VERSION "\n"
            "Copyright (C) 2011 Danjel McGougan <danjel.mcgougan@gmail.com>\n\n"
            "Usage: universal_crc <parameters>\n\n"
            "Parameters:\n"
            "  -b <bits> | --bits=<bits>\n"
            "    Number of bits in the CRC register, 1-64 is supported\n"
            "    Mandatory parameter\n\n"
            "  -p <polynomial> | --poly=<polynomial>\n"
            "    CRC polynomial value;\n"
            "    Coefficient of x^0 is bit 0 (LSB) of this value\n"
            "    Coefficient of x^1 is bit 1 of this value, etc.\n"
            "    Coefficient of x^<bits> is implied 1\n"
            "    Bit-reversed automatically if -r is used\n"
            "    Mandatory parameter\n\n"
            "  -i <init> | --init=<init>\n"
            "    Initial value of the CRC register\n"
            "    Not bit-reversed even if -r is used\n"
            "    Default 0 if not specified\n\n"
            "  -x <xor> | --xor=<xor>\n"
            "    Value that is XORed to the final CRC register value\n"
            "    Not bit-reversed even if -r is used\n"
            "    Default 0 if not specified\n\n"
            "  -r | --reverse\n"
            "    Bit-reverse the CRC register (LSB is shifted out and MSB in)\n"
            "    This also means that message bits are processed LSB first\n"
            "    Default is not to reverse\n\n"
            "  -n | --non-direct\n"
            "    Shift in message bits into the CRC register and augment the\n"
            "    message. This is equivalent to the direct method of not\n"
            "    augmenting the message and XORing the message bits with the\n"
            "    bits shifted out of the CRC register, but the initial CRC\n"
            "    register value needs to be converted (if it is non-zero) for\n"
            "    compatibility.\n"
            "    Default is direct mode.\n\n"
            "  -a <algorithm> | --algorithm=<algorithm>\n"
            "    CRC algorithm to use:\n"
            "      bit    standard bit-at-a-time algorithm (default, smallest cache footprint)\n"
            "      tab16  table-driven algorithm with small cache footprint (16 table entries)\n"
            "      tab16i table-driven using two independent lookups (32 table entries)\n"
            "             good for superscalar cores\n"
            "      tab    standard table-driven algorithm (256 table entries)\n"
            "      tabw   standard table-driven algorithm, word-at-a-time\n"
            "             same as tab but reads 32 bits at a time from memory\n"
            "      tabi   table-driven algorithm, four independent lookups (1024 entries)\n"
            "             good for superscalar cores\n"
            "             inspired by crc32 algorithm in zlib originally by Rodney Brown\n"
            "      tabiw  table-driven algorithm, four independent lookups, word-at-a-time\n"
            "             same as tabi but reads 32 bits at a time from memory\n\n"
            "  --crc-type=<type>\n"
            "    Use <type> as the unsigned integer type to hold the CRC value\n\n"
            "  --tab-type=<type>\n"
            "    Use <type> as the unsigned integer type to hold the CRC table entries\n\n"
            "  --test\n"
            "    Generate test code\n\n"
            "Examples:\n"
            "  universal_crc -b 32 -p 0x04c11db7 -i 0xffffffff -x 0xffffffff -r\n"
            "  (CRC used in i.e. Ethernet, commonly known as CRC-32)\n\n"
            "  universal_crc -b 16 -p 0x1021\n"
            "  (CRC used in XMODEM/CRC)\n\n"
            "This program is free software; you can redistribute it and/or modify\n"
            "it under the terms of the GNU General Public License as published by\n"
            "the Free Software Foundation; version 2 of the License.\n\n"
            "This program is distributed in the hope that it will be useful,\n"
            "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
            "GNU General Public License for more details.\n");
    return 1;
}

/*
 * Main, parse parameters
 */

int main(int argc, char *argv[])
{
    struct crc_param param;
    struct code_param code_param;
    bool_t got_poly = FALSE;
    int arg;

    memset(&param, 0, sizeof(param));
    memset(&code_param, 0, sizeof(code_param));

    for (arg = 1; arg < argc; arg++) {
        const char *args;
        if ((!strcmp(argv[arg], "-b") && arg < argc - 1 && (args = argv[++arg])) ||
            (!strncmp(argv[arg], "--bits=", 7) && (args = &argv[arg][7])))
        {
            unsigned long val = strtoul(args, NULL, 0);
            if (val > 0 && val <= 64) {
                param.bits = (unsigned int)val;
                continue;
            }
            return usage();
        }
        if ((!strcmp(argv[arg], "-p") && arg < argc - 1 && (args = argv[++arg])) ||
            (!strncmp(argv[arg], "--poly=", 7) && (args = &argv[arg][7])))
        {
            crc_reg_t val = strtoreg(args);
            param.poly = val;
            got_poly = TRUE;
            continue;
        }
        if ((!strcmp(argv[arg], "-i") && arg < argc - 1 && (args = argv[++arg])) ||
            (!strncmp(argv[arg], "--init=", 7) && (args = &argv[arg][7])))
        {
            crc_reg_t val = strtoreg(args);
            param.init = val;
            continue;
        }
        if ((!strcmp(argv[arg], "-x") && arg < argc - 1 && (args = argv[++arg])) ||
            (!strncmp(argv[arg], "--xor=", 6) && (args = &argv[arg][6])))
        {
            crc_reg_t val = strtoreg(args);
            param.eor = val;
            continue;
        }
        if (!strcmp(argv[arg], "-r") || !strcmp(argv[arg], "--reverse")) {
            param.reverse = TRUE;
            continue;
        }
        if (!strcmp(argv[arg], "-n") || !strcmp(argv[arg], "--non-direct")) {
            param.non_direct = TRUE;
            continue;
        }
        if ((!strcmp(argv[arg], "-a") && arg < argc - 1 && (args = argv[++arg])) ||
            (!strncmp(argv[arg], "--algorithm=", 12) && (args = &argv[arg][12])))
        {
            if (!strcmp(args, "bit")) code_param.algo = ALGO_BIT;
            else if (!strcmp(args, "tab16")) code_param.algo = ALGO_TAB16;
            else if (!strcmp(args, "tab16i")) code_param.algo = ALGO_TAB16I;
            else if (!strcmp(args, "tab")) code_param.algo = ALGO_TAB;
            else if (!strcmp(args, "tabw")) code_param.algo = ALGO_TABW;
            else if (!strcmp(args, "tabi")) code_param.algo = ALGO_TABI;
            else if (!strcmp(args, "tabiw")) code_param.algo = ALGO_TABIW;
            else return usage();
            continue;
        }
        if (!strncmp(argv[arg], "--crc-type=", 11)) {
            code_param.crc_type = &argv[arg][11];
            continue;
        }
        if (!strncmp(argv[arg], "--tab-type=", 11)) {
            code_param.tab_type = &argv[arg][11];
            continue;
        }
        if (!strcmp(argv[arg], "--test")) {
            code_param.test_code = TRUE;
            continue;
        }
        return usage();
    }

    if (param.bits == 0 || !got_poly) {
        return usage();
    }

    {
        /* Mask off excessive bits */
        crc_reg_t top_bit = (crc_reg_t)1 << (param.bits - 1);
        crc_reg_t mask = top_bit | (top_bit - 1);
        param.poly &= mask;
        param.init &= mask;
        param.eor &= mask;
    }

    if (code_param.algo == ALGO_BIT || code_param.algo == ALGO_TAB) {
        gen_code_standard(&param, &code_param);
    } else if (code_param.algo == ALGO_TAB16) {
        gen_code_tab16(&param, &code_param);
    } else if (code_param.algo == ALGO_TAB16I) {
        gen_code_tab16i(&param, &code_param);
    } else if (code_param.algo == ALGO_TABW) {
        gen_code_tabw(&param, &code_param);
    } else {
        gen_code_tabi(&param, &code_param);
    }

    return 0;
}
