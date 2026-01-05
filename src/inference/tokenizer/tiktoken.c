#include "inference/tokenizer/tiktoken.h"
#include "inference/tokenizer/simd.h"
#include "inference/tokenizer/unicode_tables.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint32_t start;
  uint32_t end;
} UnicodeRange;

static const UnicodeRange LETTER_RANGES[] = {
    {0x0041, 0x005A},   {0x0061, 0x007A},   {0x00AA, 0x00AA},
    {0x00B5, 0x00B5},   {0x00BA, 0x00BA},   {0x00C0, 0x00D6},
    {0x00D8, 0x00F6},   {0x00F8, 0x02C1},   {0x02C6, 0x02D1},
    {0x02E0, 0x02E4},   {0x02EC, 0x02EC},   {0x02EE, 0x02EE},
    {0x0370, 0x0374},   {0x0376, 0x0377},   {0x037A, 0x037D},
    {0x037F, 0x037F},   {0x0386, 0x0386},   {0x0388, 0x038A},
    {0x038C, 0x038C},   {0x038E, 0x03A1},   {0x03A3, 0x03F5},
    {0x03F7, 0x0481},   {0x048A, 0x052F},   {0x0531, 0x0556},
    {0x0559, 0x0559},   {0x0560, 0x0588},   {0x05D0, 0x05EA},
    {0x05EF, 0x05F2},   {0x0620, 0x064A},   {0x066E, 0x066F},
    {0x0671, 0x06D3},   {0x06D5, 0x06D5},   {0x06E5, 0x06E6},
    {0x06EE, 0x06EF},   {0x06FA, 0x06FC},   {0x06FF, 0x06FF},
    {0x0710, 0x0710},   {0x0712, 0x072F},   {0x074D, 0x07A5},
    {0x07B1, 0x07B1},   {0x07CA, 0x07EA},   {0x07F4, 0x07F5},
    {0x07FA, 0x07FA},   {0x0800, 0x0815},   {0x081A, 0x081A},
    {0x0824, 0x0824},   {0x0828, 0x0828},   {0x0840, 0x0858},
    {0x0860, 0x086A},   {0x0870, 0x0887},   {0x0889, 0x088E},
    {0x08A0, 0x08C9},   {0x0904, 0x0939},   {0x093D, 0x093D},
    {0x0950, 0x0950},   {0x0958, 0x0961},   {0x0971, 0x0980},
    {0x0985, 0x098C},   {0x098F, 0x0990},   {0x0993, 0x09A8},
    {0x09AA, 0x09B0},   {0x09B2, 0x09B2},   {0x09B6, 0x09B9},
    {0x09BD, 0x09BD},   {0x09CE, 0x09CE},   {0x09DC, 0x09DD},
    {0x09DF, 0x09E1},   {0x09F0, 0x09F1},   {0x09FC, 0x09FC},
    {0x0A05, 0x0A0A},   {0x0A0F, 0x0A10},   {0x0A13, 0x0A28},
    {0x0A2A, 0x0A30},   {0x0A32, 0x0A33},   {0x0A35, 0x0A36},
    {0x0A38, 0x0A39},   {0x0A59, 0x0A5C},   {0x0A5E, 0x0A5E},
    {0x0A72, 0x0A74},   {0x0A85, 0x0A8D},   {0x0A8F, 0x0A91},
    {0x0A93, 0x0AA8},   {0x0AAA, 0x0AB0},   {0x0AB2, 0x0AB3},
    {0x0AB5, 0x0AB9},   {0x0ABD, 0x0ABD},   {0x0AD0, 0x0AD0},
    {0x0AE0, 0x0AE1},   {0x0AF9, 0x0AF9},   {0x0B05, 0x0B0C},
    {0x0B0F, 0x0B10},   {0x0B13, 0x0B28},   {0x0B2A, 0x0B30},
    {0x0B32, 0x0B33},   {0x0B35, 0x0B39},   {0x0B3D, 0x0B3D},
    {0x0B5C, 0x0B5D},   {0x0B5F, 0x0B61},   {0x0B71, 0x0B71},
    {0x0B83, 0x0B83},   {0x0B85, 0x0B8A},   {0x0B8E, 0x0B90},
    {0x0B92, 0x0B95},   {0x0B99, 0x0B9A},   {0x0B9C, 0x0B9C},
    {0x0B9E, 0x0B9F},   {0x0BA3, 0x0BA4},   {0x0BA8, 0x0BAA},
    {0x0BAE, 0x0BB9},   {0x0BD0, 0x0BD0},   {0x0C05, 0x0C0C},
    {0x0C0E, 0x0C10},   {0x0C12, 0x0C28},   {0x0C2A, 0x0C39},
    {0x0C3D, 0x0C3D},   {0x0C58, 0x0C5A},   {0x0C5D, 0x0C5D},
    {0x0C60, 0x0C61},   {0x0C80, 0x0C80},   {0x0C85, 0x0C8C},
    {0x0C8E, 0x0C90},   {0x0C92, 0x0CA8},   {0x0CAA, 0x0CB3},
    {0x0CB5, 0x0CB9},   {0x0CBD, 0x0CBD},   {0x0CDD, 0x0CDE},
    {0x0CE0, 0x0CE1},   {0x0CF1, 0x0CF2},   {0x0D04, 0x0D0C},
    {0x0D0E, 0x0D10},   {0x0D12, 0x0D3A},   {0x0D3D, 0x0D3D},
    {0x0D4E, 0x0D4E},   {0x0D54, 0x0D56},   {0x0D5F, 0x0D61},
    {0x0D7A, 0x0D7F},   {0x0D85, 0x0D96},   {0x0D9A, 0x0DB1},
    {0x0DB3, 0x0DBB},   {0x0DBD, 0x0DBD},   {0x0DC0, 0x0DC6},
    {0x0E01, 0x0E30},   {0x0E32, 0x0E33},   {0x0E40, 0x0E46},
    {0x0E81, 0x0E82},   {0x0E84, 0x0E84},   {0x0E86, 0x0E8A},
    {0x0E8C, 0x0EA3},   {0x0EA5, 0x0EA5},   {0x0EA7, 0x0EB0},
    {0x0EB2, 0x0EB3},   {0x0EBD, 0x0EBD},   {0x0EC0, 0x0EC4},
    {0x0EC6, 0x0EC6},   {0x0EDC, 0x0EDF},   {0x0F00, 0x0F00},
    {0x0F40, 0x0F47},   {0x0F49, 0x0F6C},   {0x0F88, 0x0F8C},
    {0x1000, 0x102A},   {0x103F, 0x103F},   {0x1050, 0x1055},
    {0x105A, 0x105D},   {0x1061, 0x1061},   {0x1065, 0x1066},
    {0x106E, 0x1070},   {0x1075, 0x1081},   {0x108E, 0x108E},
    {0x10A0, 0x10C5},   {0x10C7, 0x10C7},   {0x10CD, 0x10CD},
    {0x10D0, 0x10FA},   {0x10FC, 0x1248},   {0x124A, 0x124D},
    {0x1250, 0x1256},   {0x1258, 0x1258},   {0x125A, 0x125D},
    {0x1260, 0x1288},   {0x128A, 0x128D},   {0x1290, 0x12B0},
    {0x12B2, 0x12B5},   {0x12B8, 0x12BE},   {0x12C0, 0x12C0},
    {0x12C2, 0x12C5},   {0x12C8, 0x12D6},   {0x12D8, 0x1310},
    {0x1312, 0x1315},   {0x1318, 0x135A},   {0x1380, 0x138F},
    {0x13A0, 0x13F5},   {0x13F8, 0x13FD},   {0x1401, 0x166C},
    {0x166F, 0x167F},   {0x1681, 0x169A},   {0x16A0, 0x16EA},
    {0x16F1, 0x16F8},   {0x1700, 0x1711},   {0x171F, 0x1731},
    {0x1740, 0x1751},   {0x1760, 0x176C},   {0x176E, 0x1770},
    {0x1780, 0x17B3},   {0x17D7, 0x17D7},   {0x17DC, 0x17DC},
    {0x1820, 0x1878},   {0x1880, 0x1884},   {0x1887, 0x18A8},
    {0x18AA, 0x18AA},   {0x18B0, 0x18F5},   {0x1900, 0x191E},
    {0x1950, 0x196D},   {0x1970, 0x1974},   {0x1980, 0x19AB},
    {0x19B0, 0x19C9},   {0x1A00, 0x1A16},   {0x1A20, 0x1A54},
    {0x1AA7, 0x1AA7},   {0x1B05, 0x1B33},   {0x1B45, 0x1B4C},
    {0x1B83, 0x1BA0},   {0x1BAE, 0x1BAF},   {0x1BBA, 0x1BE5},
    {0x1C00, 0x1C23},   {0x1C4D, 0x1C4F},   {0x1C5A, 0x1C7D},
    {0x1C80, 0x1C88},   {0x1C90, 0x1CBA},   {0x1CBD, 0x1CBF},
    {0x1CE9, 0x1CEC},   {0x1CEE, 0x1CF3},   {0x1CF5, 0x1CF6},
    {0x1CFA, 0x1CFA},   {0x1D00, 0x1DBF},   {0x1E00, 0x1F15},
    {0x1F18, 0x1F1D},   {0x1F20, 0x1F45},   {0x1F48, 0x1F4D},
    {0x1F50, 0x1F57},   {0x1F59, 0x1F59},   {0x1F5B, 0x1F5B},
    {0x1F5D, 0x1F5D},   {0x1F5F, 0x1F7D},   {0x1F80, 0x1FB4},
    {0x1FB6, 0x1FBC},   {0x1FBE, 0x1FBE},   {0x1FC2, 0x1FC4},
    {0x1FC6, 0x1FCC},   {0x1FD0, 0x1FD3},   {0x1FD6, 0x1FDB},
    {0x1FE0, 0x1FEC},   {0x1FF2, 0x1FF4},   {0x1FF6, 0x1FFC},
    {0x2071, 0x2071},   {0x207F, 0x207F},   {0x2090, 0x209C},
    {0x2102, 0x2102},   {0x2107, 0x2107},   {0x210A, 0x2113},
    {0x2115, 0x2115},   {0x2119, 0x211D},   {0x2124, 0x2124},
    {0x2126, 0x2126},   {0x2128, 0x2128},   {0x212A, 0x212D},
    {0x212F, 0x2139},   {0x213C, 0x213F},   {0x2145, 0x2149},
    {0x214E, 0x214E},   {0x2183, 0x2184},   {0x2C00, 0x2CE4},
    {0x2CEB, 0x2CEE},   {0x2CF2, 0x2CF3},   {0x2D00, 0x2D25},
    {0x2D27, 0x2D27},   {0x2D2D, 0x2D2D},   {0x2D30, 0x2D67},
    {0x2D6F, 0x2D6F},   {0x2D80, 0x2D96},   {0x2DA0, 0x2DA6},
    {0x2DA8, 0x2DAE},   {0x2DB0, 0x2DB6},   {0x2DB8, 0x2DBE},
    {0x2DC0, 0x2DC6},   {0x2DC8, 0x2DCE},   {0x2DD0, 0x2DD6},
    {0x2DD8, 0x2DDE},   {0x2E2F, 0x2E2F},   {0x3005, 0x3006},
    {0x3031, 0x3035},   {0x303B, 0x303C},   {0x3041, 0x3096},
    {0x309D, 0x309F},   {0x30A1, 0x30FA},   {0x30FC, 0x30FF},
    {0x3105, 0x312F},   {0x3131, 0x318E},   {0x31A0, 0x31BF},
    {0x31F0, 0x31FF},   {0x3400, 0x4DBF},   {0x4E00, 0xA48C},
    {0xA4D0, 0xA4FD},   {0xA500, 0xA60C},   {0xA610, 0xA61F},
    {0xA62A, 0xA62B},   {0xA640, 0xA66E},   {0xA67F, 0xA69D},
    {0xA6A0, 0xA6E5},   {0xA717, 0xA71F},   {0xA722, 0xA788},
    {0xA78B, 0xA7CA},   {0xA7D0, 0xA7D1},   {0xA7D3, 0xA7D3},
    {0xA7D5, 0xA7D9},   {0xA7F2, 0xA801},   {0xA803, 0xA805},
    {0xA807, 0xA80A},   {0xA80C, 0xA822},   {0xA840, 0xA873},
    {0xA882, 0xA8B3},   {0xA8F2, 0xA8F7},   {0xA8FB, 0xA8FB},
    {0xA8FD, 0xA8FE},   {0xA90A, 0xA925},   {0xA930, 0xA946},
    {0xA960, 0xA97C},   {0xA984, 0xA9B2},   {0xA9CF, 0xA9CF},
    {0xA9E0, 0xA9E4},   {0xA9E6, 0xA9EF},   {0xA9FA, 0xA9FE},
    {0xAA00, 0xAA28},   {0xAA40, 0xAA42},   {0xAA44, 0xAA4B},
    {0xAA60, 0xAA76},   {0xAA7A, 0xAA7A},   {0xAA7E, 0xAAAF},
    {0xAAB1, 0xAAB1},   {0xAAB5, 0xAAB6},   {0xAAB9, 0xAABD},
    {0xAAC0, 0xAAC0},   {0xAAC2, 0xAAC2},   {0xAADB, 0xAADD},
    {0xAAE0, 0xAAEA},   {0xAAF2, 0xAAF4},   {0xAB01, 0xAB06},
    {0xAB09, 0xAB0E},   {0xAB11, 0xAB16},   {0xAB20, 0xAB26},
    {0xAB28, 0xAB2E},   {0xAB30, 0xAB5A},   {0xAB5C, 0xAB69},
    {0xAB70, 0xABE2},   {0xAC00, 0xD7A3},   {0xD7B0, 0xD7C6},
    {0xD7CB, 0xD7FB},   {0xF900, 0xFA6D},   {0xFA70, 0xFAD9},
    {0xFB00, 0xFB06},   {0xFB13, 0xFB17},   {0xFB1D, 0xFB1D},
    {0xFB1F, 0xFB28},   {0xFB2A, 0xFB36},   {0xFB38, 0xFB3C},
    {0xFB3E, 0xFB3E},   {0xFB40, 0xFB41},   {0xFB43, 0xFB44},
    {0xFB46, 0xFBB1},   {0xFBD3, 0xFD3D},   {0xFD50, 0xFD8F},
    {0xFD92, 0xFDC7},   {0xFDF0, 0xFDFB},   {0xFE70, 0xFE74},
    {0xFE76, 0xFEFC},   {0xFF21, 0xFF3A},   {0xFF41, 0xFF5A},
    {0xFF66, 0xFFBE},   {0xFFC2, 0xFFC7},   {0xFFCA, 0xFFCF},
    {0xFFD2, 0xFFD7},   {0xFFDA, 0xFFDC},   {0x10000, 0x1000B},
    {0x1000D, 0x10026}, {0x10028, 0x1003A}, {0x1003C, 0x1003D},
    {0x1003F, 0x1004D}, {0x10050, 0x1005D}, {0x10080, 0x100FA},
    {0x10280, 0x1029C}, {0x102A0, 0x102D0}, {0x10300, 0x1031F},
    {0x1032D, 0x10340}, {0x10342, 0x10349}, {0x10350, 0x10375},
    {0x10380, 0x1039D}, {0x103A0, 0x103C3}, {0x103C8, 0x103CF},
    {0x10400, 0x1049D}, {0x104B0, 0x104D3}, {0x104D8, 0x104FB},
    {0x10500, 0x10527}, {0x10530, 0x10563}, {0x10570, 0x1057A},
    {0x1057C, 0x1058A}, {0x1058C, 0x10592}, {0x10594, 0x10595},
    {0x10597, 0x105A1}, {0x105A3, 0x105B1}, {0x105B3, 0x105B9},
    {0x105BB, 0x105BC},
};
#define LETTER_RANGE_COUNT (sizeof(LETTER_RANGES) / sizeof(LETTER_RANGES[0]))

static const UnicodeRange NUMBER_RANGES[] = {
    {0x0030, 0x0039},   {0x00B2, 0x00B3},   {0x00B9, 0x00B9},
    {0x00BC, 0x00BE},   {0x0660, 0x0669},   {0x06F0, 0x06F9},
    {0x07C0, 0x07C9},   {0x0966, 0x096F},   {0x09E6, 0x09EF},
    {0x09F4, 0x09F9},   {0x0A66, 0x0A6F},   {0x0AE6, 0x0AEF},
    {0x0B66, 0x0B6F},   {0x0B72, 0x0B77},   {0x0BE6, 0x0BF2},
    {0x0C66, 0x0C6F},   {0x0C78, 0x0C7E},   {0x0CE6, 0x0CEF},
    {0x0D58, 0x0D5E},   {0x0D66, 0x0D78},   {0x0DE6, 0x0DEF},
    {0x0E50, 0x0E59},   {0x0ED0, 0x0ED9},   {0x0F20, 0x0F33},
    {0x1040, 0x1049},   {0x1090, 0x1099},   {0x1369, 0x137C},
    {0x16EE, 0x16F0},   {0x17E0, 0x17E9},   {0x17F0, 0x17F9},
    {0x1810, 0x1819},   {0x1946, 0x194F},   {0x19D0, 0x19DA},
    {0x1A80, 0x1A89},   {0x1A90, 0x1A99},   {0x1B50, 0x1B59},
    {0x1BB0, 0x1BB9},   {0x1C40, 0x1C49},   {0x1C50, 0x1C59},
    {0x2070, 0x2070},   {0x2074, 0x2079},   {0x2080, 0x2089},
    {0x2150, 0x2182},   {0x2185, 0x2189},   {0x2460, 0x249B},
    {0x24EA, 0x24FF},   {0x2776, 0x2793},   {0x2CFD, 0x2CFD},
    {0x3007, 0x3007},   {0x3021, 0x3029},   {0x3038, 0x303A},
    {0x3192, 0x3195},   {0x3220, 0x3229},   {0x3248, 0x324F},
    {0x3251, 0x325F},   {0x3280, 0x3289},   {0x32B1, 0x32BF},
    {0xA620, 0xA629},   {0xA6E6, 0xA6EF},   {0xA830, 0xA835},
    {0xA8D0, 0xA8D9},   {0xA900, 0xA909},   {0xA9D0, 0xA9D9},
    {0xA9F0, 0xA9F9},   {0xAA50, 0xAA59},   {0xABF0, 0xABF9},
    {0xFF10, 0xFF19},   {0x10107, 0x10133}, {0x10140, 0x10178},
    {0x1018A, 0x1018B}, {0x102E1, 0x102FB}, {0x10320, 0x10323},
    {0x10341, 0x10341}, {0x1034A, 0x1034A}, {0x103D1, 0x103D5},
    {0x104A0, 0x104A9}, {0x10858, 0x1085F}, {0x10879, 0x1087F},
    {0x108A7, 0x108AF}, {0x108FB, 0x108FF}, {0x10916, 0x1091B},
    {0x109BC, 0x109BD}, {0x109C0, 0x109CF}, {0x109D2, 0x109FF},
    {0x10A40, 0x10A48}, {0x10A7D, 0x10A7E}, {0x10A9D, 0x10A9F},
    {0x10AEB, 0x10AEF}, {0x10B58, 0x10B5F}, {0x10B78, 0x10B7F},
    {0x10BA9, 0x10BAF}, {0x10CFA, 0x10CFF}, {0x10D30, 0x10D39},
    {0x10E60, 0x10E7E}, {0x10F1D, 0x10F26}, {0x10F51, 0x10F54},
    {0x10FC5, 0x10FCB}, {0x11052, 0x1106F}, {0x110F0, 0x110F9},
    {0x11136, 0x1113F}, {0x111D0, 0x111D9}, {0x111E1, 0x111F4},
    {0x112F0, 0x112F9}, {0x11450, 0x11459}, {0x114D0, 0x114D9},
    {0x11650, 0x11659}, {0x116C0, 0x116C9}, {0x11730, 0x1173B},
    {0x118E0, 0x118F2}, {0x11950, 0x11959}, {0x11C50, 0x11C6C},
    {0x11D50, 0x11D59}, {0x11DA0, 0x11DA9}, {0x11F50, 0x11F59},
    {0x11FC0, 0x11FD4}, {0x12400, 0x1246E}, {0x16A60, 0x16A69},
    {0x16AC0, 0x16AC9}, {0x16B50, 0x16B59}, {0x16B5B, 0x16B61},
    {0x16E80, 0x16E96}, {0x1D2C0, 0x1D2D3}, {0x1D2E0, 0x1D2F3},
    {0x1D360, 0x1D378}, {0x1D7CE, 0x1D7FF}, {0x1E140, 0x1E149},
    {0x1E2F0, 0x1E2F9}, {0x1E4F0, 0x1E4F9}, {0x1E8C7, 0x1E8CF},
    {0x1E950, 0x1E959}, {0x1EC71, 0x1ECAB}, {0x1ECAD, 0x1ECAF},
    {0x1ECB1, 0x1ECB4}, {0x1ED01, 0x1ED2D}, {0x1ED2F, 0x1ED3D},
    {0x1F100, 0x1F10C}, {0x1FBF0, 0x1FBF9},
};
#define NUMBER_RANGE_COUNT (sizeof(NUMBER_RANGES) / sizeof(NUMBER_RANGES[0]))

static bool in_ranges(uint32_t cp, const UnicodeRange *ranges, size_t count) {
  size_t lo = 0, hi = count;
  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2;
    if (cp < ranges[mid].start) {
      hi = mid;
    } else if (cp > ranges[mid].end) {
      lo = mid + 1;
    } else {
      return true;
    }
  }
  return false;
}

bool unicode_is_letter(uint32_t cp) {
  if (cp < 0x10000)
    return unicode_bmp_is_letter(cp);
  return in_ranges(cp, LETTER_RANGES, LETTER_RANGE_COUNT);
}

bool unicode_is_number(uint32_t cp) {
  if (cp < 0x10000)
    return unicode_bmp_is_number(cp);
  return in_ranges(cp, NUMBER_RANGES, NUMBER_RANGE_COUNT);
}

bool unicode_is_whitespace(uint32_t cp) {
  return cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == '\f' ||
         cp == '\v' || cp == 0x85 || cp == 0xA0 || cp == 0x1680 ||
         (cp >= 0x2000 && cp <= 0x200A) || cp == 0x2028 || cp == 0x2029 ||
         cp == 0x202F || cp == 0x205F || cp == 0x3000;
}

static inline int utf8_decode_inline(const uint8_t *bytes, size_t len,
                                     uint32_t *out_cp) {
  if (len == 0)
    return 0;

  uint8_t b0 = bytes[0];
  if (b0 < 0x80) {
    *out_cp = b0;
    return 1;
  } else if ((b0 & 0xE0) == 0xC0 && len >= 2) {
    *out_cp = ((b0 & 0x1F) << 6) | (bytes[1] & 0x3F);
    return 2;
  } else if ((b0 & 0xF0) == 0xE0 && len >= 3) {
    *out_cp =
        ((b0 & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
    return 3;
  } else if ((b0 & 0xF8) == 0xF0 && len >= 4) {
    *out_cp = ((b0 & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) |
              ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
    return 4;
  }
  *out_cp = 0xFFFD;
  return 1;
}

int utf8_decode(const uint8_t *bytes, size_t len, uint32_t *out_cp) {
  return utf8_decode_inline(bytes, len, out_cp);
}

int utf8_encode(uint32_t cp, uint8_t *out) {
  if (cp < 0x80) {
    out[0] = (uint8_t)cp;
    return 1;
  } else if (cp < 0x800) {
    out[0] = 0xC0 | (cp >> 6);
    out[1] = 0x80 | (cp & 0x3F);
    return 2;
  } else if (cp < 0x10000) {
    out[0] = 0xE0 | (cp >> 12);
    out[1] = 0x80 | ((cp >> 6) & 0x3F);
    out[2] = 0x80 | (cp & 0x3F);
    return 3;
  } else {
    out[0] = 0xF0 | (cp >> 18);
    out[1] = 0x80 | ((cp >> 12) & 0x3F);
    out[2] = 0x80 | ((cp >> 6) & 0x3F);
    out[3] = 0x80 | (cp & 0x3F);
    return 4;
  }
}

static bool match_contraction(const uint8_t *text, size_t len,
                              size_t *out_len) {
  if (len < 2 || text[0] != '\'')
    return false;

  if (len >= 2) {
    char c = (char)tolower(text[1]);
    if (c == 's' || c == 't' || c == 'm' || c == 'd') {
      *out_len = 2;
      return true;
    }
  }
  if (len >= 3) {
    char c1 = (char)tolower(text[1]);
    char c2 = (char)tolower(text[2]);
    if ((c1 == 'l' && c2 == 'l') || (c1 == 'v' && c2 == 'e') ||
        (c1 == 'r' && c2 == 'e')) {
      *out_len = 3;
      return true;
    }
  }
  return false;
}

static size_t match_letters(const uint8_t *text, size_t len) {
  size_t pos = 0;

  size_t ascii_run = simd_match_ascii_letters(text, len);
  pos += ascii_run;

  while (pos < len) {
    uint8_t b = text[pos];
    if (b < 0x80) {
      if (!((b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z')))
        break;
      pos++;
      continue;
    }

    uint32_t cp;
    int cplen = utf8_decode_inline(text + pos, len - pos, &cp);
    if (cplen == 0 || !unicode_is_letter(cp))
      break;
    pos += cplen;
  }
  return pos;
}

static size_t match_numbers(const uint8_t *text, size_t len, int max_digits) {
  size_t pos = 0;
  int count = 0;
  while (pos < len && (max_digits == 0 || count < max_digits)) {
    uint32_t cp;
    int cplen = utf8_decode(text + pos, len - pos, &cp);
    if (cplen == 0 || !unicode_is_number(cp))
      break;
    pos += cplen;
    count++;
  }
  return pos;
}

static int add_span(SpanList *spans, size_t start, size_t end) {
  if (start >= end)
    return 0;
  if (spans->count >= spans->cap) {
    size_t newcap = spans->cap == 0 ? 64 : spans->cap * 2;
    TextSpan *new_spans = realloc(spans->spans, newcap * sizeof(TextSpan));
    if (!new_spans)
      return -1;
    spans->spans = new_spans;
    spans->cap = newcap;
  }
  spans->spans[spans->count].start = start;
  spans->spans[spans->count].end = end;
  spans->count++;
  return 0;
}

static bool is_newline(uint32_t cp) { return cp == '\n' || cp == '\r'; }

static size_t match_punct_no_ws(const uint8_t *text, size_t len) {
  size_t pos = 0;
  while (pos < len) {
    uint32_t cp;
    int cplen = utf8_decode(text + pos, len - pos, &cp);
    if (cplen == 0)
      break;
    if (unicode_is_letter(cp) || unicode_is_number(cp) ||
        unicode_is_whitespace(cp))
      break;
    pos += cplen;
  }
  return pos;
}

static size_t match_newlines(const uint8_t *text, size_t len) {
  size_t pos = 0;
  while (pos < len) {
    uint32_t cp;
    int cplen = utf8_decode(text + pos, len - pos, &cp);
    if (cplen == 0 || !is_newline(cp))
      break;
    pos += cplen;
  }
  return pos;
}

int pretokenize_cl100k(const char *text, SpanList *spans) {
  const uint8_t *bytes = (const uint8_t *)text;
  size_t len = strlen(text);
  size_t pos = 0;

  spans->count = 0;

  while (pos < len) {
    size_t start = pos;
    size_t match_len = 0;

    if (match_contraction(bytes + pos, len - pos, &match_len)) {
      pos += match_len;
      if (add_span(spans, start, pos) < 0)
        return -1;
      continue;
    }

    uint32_t cp;
    int cplen = utf8_decode(bytes + pos, len - pos, &cp);
    if (cplen == 0) {
      pos++;
      continue;
    }

    if (!is_newline(cp) && !unicode_is_letter(cp) && !unicode_is_number(cp)) {
      size_t prefix_len = cplen;
      size_t letters =
          match_letters(bytes + pos + prefix_len, len - pos - prefix_len);
      if (letters > 0) {
        pos += prefix_len + letters;
        if (add_span(spans, start, pos) < 0)
          return -1;
        continue;
      }
    }

    if (unicode_is_letter(cp)) {
      pos += cplen;
      size_t more = match_letters(bytes + pos, len - pos);
      pos += more;
      if (add_span(spans, start, pos) < 0)
        return -1;
      continue;
    }

    if (unicode_is_number(cp)) {
      size_t num_match = match_numbers(bytes + pos, len - pos, 3);
      pos += num_match;
      if (add_span(spans, start, pos) < 0)
        return -1;
      continue;
    }

    if (cp == ' ') {
      uint32_t next_cp;
      int next_len =
          utf8_decode(bytes + pos + cplen, len - pos - cplen, &next_cp);
      if (next_len > 0 && !unicode_is_letter(next_cp) &&
          !unicode_is_number(next_cp) && !unicode_is_whitespace(next_cp)) {
        pos += cplen;
        size_t punct = match_punct_no_ws(bytes + pos, len - pos);
        pos += punct;
        size_t newlines = match_newlines(bytes + pos, len - pos);
        pos += newlines;
        if (add_span(spans, start, pos) < 0)
          return -1;
        continue;
      }
    }

    if (!unicode_is_letter(cp) && !unicode_is_number(cp) &&
        !unicode_is_whitespace(cp)) {
      size_t punct = match_punct_no_ws(bytes + pos, len - pos);
      pos += punct;
      size_t newlines = match_newlines(bytes + pos, len - pos);
      pos += newlines;
      if (add_span(spans, start, pos) < 0)
        return -1;
      continue;
    }

    if (unicode_is_whitespace(cp)) {
      size_t ws_end = pos;
      while (ws_end < len) {
        uint32_t ws_cp;
        int ws_len = utf8_decode(bytes + ws_end, len - ws_end, &ws_cp);
        if (ws_len == 0 || !unicode_is_whitespace(ws_cp))
          break;
        ws_end += ws_len;
      }

      if (ws_end >= len) {
        pos = ws_end;
        if (add_span(spans, start, pos) < 0)
          return -1;
        continue;
      }

      size_t best_end = pos + cplen;
      for (size_t try_end = ws_end; try_end > pos;) {
        uint32_t prev_cp;
        size_t prev_start = try_end;
        while (prev_start > pos) {
          prev_start--;
          if ((bytes[prev_start] & 0xC0) != 0x80)
            break;
        }
        int prev_len =
            utf8_decode(bytes + prev_start, try_end - prev_start, &prev_cp);
        if (prev_len <= 0)
          break;
        try_end = prev_start;

        uint32_t after_cp;
        size_t after_pos = try_end + prev_len;
        int after_len =
            utf8_decode(bytes + after_pos, len - after_pos, &after_cp);

        if (after_len == 0 || unicode_is_whitespace(after_cp) ||
            after_pos >= len) {
          best_end = after_pos;
          break;
        }
      }

      uint32_t after_cp;
      int after_len = utf8_decode(bytes + best_end, len - best_end, &after_cp);
      if (after_len > 0 && is_newline(after_cp)) {
        best_end += after_len;
      }

      pos = best_end;
      if (add_span(spans, start, pos) < 0)
        return -1;
      continue;
    }

    pos += cplen;
    if (add_span(spans, start, pos) < 0)
      return -1;
  }

  return 0;
}

static uint64_t hash_bytes(const uint8_t *bytes, size_t len) {
  return simd_hash_bytes(bytes, len);
}

uint32_t lookup_rank(const Tokenizer *t, const uint8_t *bytes, size_t len) {
  if (len == 1) {
    return t->byte_to_rank[bytes[0]];
  }

  if (!t->hash_table)
    return UINT32_MAX;

  uint64_t hash = hash_bytes(bytes, len);
  size_t idx = hash & (t->hash_size - 1);
  size_t start_idx = idx;

  do {
    HashEntry *e = &t->hash_table[idx];
    if (!e->occupied)
      return UINT32_MAX;
    if (e->len == len && memcmp(e->bytes, bytes, len) == 0)
      return e->rank;
    idx = (idx + 1) & (t->hash_size - 1);
  } while (idx != start_idx);

  return UINT32_MAX;
}

typedef struct {
  size_t start;
  uint32_t rank;
} BPEPart;

int bpe_encode_piece(const Tokenizer *t, const uint8_t *piece, size_t piece_len,
                     uint32_t *out_tokens, size_t max_tokens) {
  if (piece_len == 0)
    return 0;

  if (piece_len == 1) {
    if (max_tokens < 1)
      return -1;
    uint32_t rank = t->byte_to_rank[piece[0]];
    if (rank == UINT32_MAX)
      return -1;
    out_tokens[0] = rank;
    return 1;
  }

  uint32_t direct = lookup_rank(t, piece, piece_len);
  if (direct != UINT32_MAX) {
    if (max_tokens < 1)
      return -1;
    out_tokens[0] = direct;
    return 1;
  }

  BPEPart *parts = malloc((piece_len + 1) * sizeof(BPEPart));
  if (!parts)
    return -1;

  for (size_t i = 0; i < piece_len; i++) {
    uint32_t rank = UINT32_MAX;
    if (i + 1 < piece_len) {
      rank = lookup_rank(t, piece + i, 2);
    }
    parts[i].start = i;
    parts[i].rank = rank;
  }
  parts[piece_len].start = piece_len;
  parts[piece_len].rank = UINT32_MAX;

  size_t num_parts = piece_len + 1;

  while (1) {
    uint32_t min_rank = UINT32_MAX;
    size_t min_idx = 0;

    for (size_t i = 0; i + 1 < num_parts; i++) {
      if (parts[i].rank < min_rank) {
        min_rank = parts[i].rank;
        min_idx = i;
      }
    }

    if (min_rank == UINT32_MAX)
      break;

    size_t i = min_idx;

    memmove(&parts[i + 1], &parts[i + 2],
            (num_parts - i - 2) * sizeof(BPEPart));
    num_parts--;

    if (i > 0) {
      if (i + 1 < num_parts) {
        size_t new_start = parts[i - 1].start;
        size_t new_end = parts[i + 1].start;
        parts[i - 1].rank =
            lookup_rank(t, piece + new_start, new_end - new_start);
      } else {
        parts[i - 1].rank = UINT32_MAX;
      }
    }

    if (i + 2 < num_parts) {
      size_t new_start = parts[i].start;
      size_t new_end = parts[i + 2].start;
      parts[i].rank = lookup_rank(t, piece + new_start, new_end - new_start);
    } else {
      parts[i].rank = UINT32_MAX;
    }
  }

  int token_count = 0;
  for (size_t i = 0; i + 1 < num_parts; i++) {
    size_t start = parts[i].start;
    size_t end = parts[i + 1].start;
    size_t seg_len = end - start;

    uint32_t rank = lookup_rank(t, piece + start, seg_len);
    if (rank != UINT32_MAX) {
      if (token_count >= (int)max_tokens) {
        free(parts);
        return -1;
      }
      out_tokens[token_count++] = rank;
    } else {
      for (size_t b = start; b < end; b++) {
        uint32_t byte_rank = t->byte_to_rank[piece[b]];
        if (byte_rank == UINT32_MAX) {
          free(parts);
          return -1;
        }
        if (token_count >= (int)max_tokens) {
          free(parts);
          return -1;
        }
        out_tokens[token_count++] = byte_rank;
      }
    }
  }

  free(parts);
  return token_count;
}

void tokenizer_init(Tokenizer *t) {
  memset(t, 0, sizeof(*t));
  t->byte_to_rank = malloc(256 * sizeof(uint32_t));
  if (t->byte_to_rank) {
    for (int i = 0; i < 256; i++) {
      t->byte_to_rank[i] = UINT32_MAX;
    }
  }
  t->hash_size = HASH_TABLE_SIZE;
  t->hash_table = calloc(t->hash_size, sizeof(HashEntry));
}

void tokenizer_free(Tokenizer *t) {
  if (t->hash_table) {
    for (size_t i = 0; i < t->hash_size; i++) {
      if (t->hash_table[i].occupied) {
        free(t->hash_table[i].bytes);
      }
    }
    free(t->hash_table);
  }
  free(t->entries);
  free(t->byte_to_rank);
  memset(t, 0, sizeof(*t));
}

static bool hash_insert(Tokenizer *t, const uint8_t *bytes, size_t len,
                        uint32_t rank) {
  if (len <= 1)
    return true;

  uint64_t hash = hash_bytes(bytes, len);
  size_t idx = hash & (t->hash_size - 1);
  size_t start_idx = idx;

  do {
    HashEntry *e = &t->hash_table[idx];
    if (!e->occupied) {
      e->bytes = malloc(len);
      if (!e->bytes)
        return false;
      memcpy(e->bytes, bytes, len);
      e->len = len;
      e->rank = rank;
      e->occupied = true;
      return true;
    }
    idx = (idx + 1) & (t->hash_size - 1);
  } while (idx != start_idx);

  return false;
}

static size_t base64_decode(const char *in, size_t in_len, uint8_t *out,
                            size_t out_cap) {
  return simd_base64_decode(in, in_len, out, out_cap);
}

bool tokenizer_load_tiktoken(Tokenizer *t, const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *data = malloc(size);
  if (!data) {
    fclose(f);
    return false;
  }

  size_t read = fread(data, 1, size, f);
  fclose(f);

  if ((long)read != size) {
    free(data);
    return false;
  }

  bool result = tokenizer_load_tiktoken_from_memory(t, data, size);
  free(data);
  return result;
}

bool tokenizer_load_tiktoken_from_memory(Tokenizer *t, const uint8_t *data,
                                         size_t len) {
  t->entries = malloc(MAX_VOCAB_SIZE * sizeof(TokenEntry));
  if (!t->entries)
    return false;
  t->capacity = MAX_VOCAB_SIZE;
  t->count = 0;

  const char *p = (const char *)data;
  const char *end = p + len;

  while (p < end) {
    const char *line_end = memchr(p, '\n', end - p);
    if (!line_end)
      line_end = end;

    const char *space = memchr(p, ' ', line_end - p);
    if (!space) {
      p = line_end + 1;
      continue;
    }

    size_t b64_len = space - p;
    uint8_t decoded[MAX_TOKEN_BYTES];
    size_t decoded_len = base64_decode(p, b64_len, decoded, MAX_TOKEN_BYTES);

    uint32_t rank = 0;
    const char *rank_start = space + 1;
    while (rank_start < line_end && *rank_start >= '0' && *rank_start <= '9') {
      rank = rank * 10 + (*rank_start - '0');
      rank_start++;
    }

    if (decoded_len > 0 && t->count < t->capacity) {
      TokenEntry *entry = &t->entries[t->count];
      memcpy(entry->bytes, decoded, decoded_len);
      entry->len = decoded_len;
      entry->rank = rank;
      t->count++;

      if (decoded_len == 1) {
        t->byte_to_rank[decoded[0]] = rank;
      } else {
        hash_insert(t, decoded, decoded_len, rank);
      }
    }

    p = line_end + 1;
  }

  t->loaded = true;
  return true;
}

int tokenizer_encode(const Tokenizer *t, const char *text, uint32_t *out_tokens,
                     size_t max_tokens) {
  if (!t->loaded || !text)
    return -1;

  SpanList spans = {0};
  if (pretokenize_cl100k(text, &spans) < 0) {
    return -1;
  }

  int total = 0;
  for (size_t i = 0; i < spans.count; i++) {
    const uint8_t *piece = (const uint8_t *)text + spans.spans[i].start;
    size_t piece_len = spans.spans[i].end - spans.spans[i].start;

    int n = bpe_encode_piece(t, piece, piece_len, out_tokens + total,
                             max_tokens - total);
    if (n < 0) {
      free(spans.spans);
      return -1;
    }
    total += n;
  }

  free(spans.spans);
  return total;
}

int tokenizer_count_tokens(const Tokenizer *t, const char *text) {
  if (!t->loaded || !text)
    return -1;

  uint32_t *tokens = malloc(strlen(text) * sizeof(uint32_t));
  if (!tokens)
    return -1;

  int count = tokenizer_encode(t, text, tokens, strlen(text));
  free(tokens);
  return count;
}

char *tokenizer_decode(const Tokenizer *t, const uint32_t *tokens,
                       size_t count) {
  if (!t->loaded)
    return NULL;

  size_t buf_size = count * MAX_TOKEN_BYTES;
  char *result = malloc(buf_size + 1);
  if (!result)
    return NULL;

  size_t pos = 0;
  for (size_t i = 0; i < count; i++) {
    uint32_t token = tokens[i];

    for (size_t j = 0; j < t->count; j++) {
      if (t->entries[j].rank == token) {
        size_t copy_len = t->entries[j].len;
        if (pos + copy_len < buf_size) {
          memcpy(result + pos, t->entries[j].bytes, copy_len);
          pos += copy_len;
        }
        break;
      }
    }
  }

  result[pos] = '\0';
  return result;
}
