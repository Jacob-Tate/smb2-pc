/* FONT_DEFINITIONS, extracted from the GM2E8P main.dol .data at
 * 0x80081D80 (the port does not load the DOL image), as a native table. */
#include "bitmap.h"

static u16 s_font0_string[] = { 0 };
static s16 s_font0_pages[1];
static u16 s_font1_string[] = { 0 };
static s16 s_font1_pages[1];
static u16 s_font2_string[] = { 0 };
static s16 s_font2_pages[1];
static u16 s_font3_string[] = { 0x8141, 0x8142, 0x8143, 0x8144, 0x8145, 0x8146, 0x8147, 0x8148, 0x8149, 0x814D, 0x814F, 0x8151, 0x8158, 0x815B, 0x815C, 0x815E, 0x815F, 0x8160, 0x8162, 0x8163, 0x8165, 0x8166, 0x8167, 0x8168, 0x8169, 0x816A, 0x816D, 0x816E, 0x8175, 0x8176, 0x8177, 0x8178, 0x817B, 0x817C, 0x817E, 0x8180, 0x8181, 0x8182, 0x8183, 0x8184, 0x8187, 0x8189, 0x818A, 0x818F, 0x8190, 0x8193, 0x8194, 0x8195, 0x8196, 0x8197, 0x8199, 0x819A, 0x819B, 0x819C, 0x81A0, 0x81A1, 0x81A2, 0x81A3, 0x81A6, 0x81A8, 0x81CB, 0x81F4, 0x824F, 0x8250, 0x8251, 0x8252, 0x8253, 0x8254, 0x8255, 0x8256, 0x8257, 0x8258, 0x8260, 0x8261, 0x8262, 0x8263, 0x8264, 0x8265, 0x8266, 0x8267, 0x8268, 0x8269, 0x826A, 0x826B, 0x826C, 0x826D, 0x826E, 0x826F, 0x8270, 0x8271, 0x8272, 0x8273, 0x8274, 0x8275, 0x8276, 0x8277, 0x8278, 0x8279, 0x8281, 0x8282, 0x8283, 0x8284, 0x8285, 0x8286, 0x8287, 0x8288, 0x8289, 0x828A, 0x828B, 0x828C, 0x828D, 0x828E, 0x828F, 0x8290, 0x8291, 0x8292, 0x8293, 0x8294, 0x8295, 0x8296, 0x8297, 0x8298, 0x8299, 0x829A, 0x829F, 0x82A0, 0x82A1, 0x82A2, 0x82A3, 0x82A4, 0x82A5, 0x82A6, 0x82A7, 0x82A8, 0x82A9, 0x82AA, 0x82AB, 0x82AC, 0x82AD, 0x82AE, 0x82AF, 0x82B0, 0x82B1, 0x82B2, 0x82B3, 0x82B4, 0x82B5, 0x82B6, 0x82B7, 0x82B8, 0x82B9, 0x82BA, 0x82BB, 0x82BC, 0x82BD, 0x82BE, 0x82BF, 0x82C0, 0x82C1, 0x82C2, 0x82C3, 0x82C4, 0x82C5, 0x82C6, 0x82C7, 0x82C8, 0x82C9, 0x82CA, 0x82CB, 0x82CC, 0x82CD, 0x82CE, 0x82CF, 0x82D0, 0x82D1, 0x82D2, 0x82D3, 0x82D4, 0x82D5, 0x82D6, 0x82D7, 0x82D8, 0x82D9, 0x82DA, 0x82DB, 0x82DC, 0x82DD, 0x82DE, 0x82DF, 0x82E0, 0x82E1, 0x82E2, 0x82E3, 0x82E4, 0x82E5, 0x82E6, 0x82E7, 0x82E8, 0x82E9, 0x82EA, 0x82EB, 0x82EC, 0x82ED, 0x82EE, 0x82EF, 0x82F0, 0x82F1, 0x8340, 0x8341, 0x8342, 0x8343, 0x8344, 0x8345, 0x8346, 0x8347, 0x8348, 0x8349, 0x834A, 0x834B, 0x834C, 0x834D, 0x834E, 0x834F, 0x8350, 0x8351, 0x8352, 0x8353, 0x8354, 0x8355, 0x8356, 0x8357, 0x8358, 0x8359, 0x835A, 0x835B, 0x835C, 0x835D, 0x835E, 0x835F, 0x8360, 0x8361, 0x8362, 0x8363, 0x8364, 0x8365, 0x8366, 0x8367, 0x8368, 0x8369, 0x836A, 0x836B, 0x836C, 0x836D, 0x836E, 0x836F, 0x8370, 0x8371, 0x8372, 0x8373, 0x8374, 0x8375, 0x8376, 0x8377, 0x8378, 0x8379, 0x837A, 0x837B, 0x837C, 0x837D, 0x837E, 0x8380, 0x8381, 0x8382, 0x8383, 0x8384, 0x8385, 0x8386, 0x8387, 0x8388, 0x8389, 0x838A, 0x838B, 0x838C, 0x838D, 0x838E, 0x838F, 0x8390, 0x8391, 0x8392, 0x8393, 0x8394, 0x839F, 0x83A0, 0x83A1, 0x83A4, 0x83A6, 0x83AD, 0x83AF, 0x83B2, 0x83B4, 0x83B5, 0x84D0, 0x84D2, 0x84D4, 0x84D6, 0 };
static s16 s_font3_pages[305];
static u16 s_font4_string[] = { 0 };
static s16 s_font4_pages[1];
static u16 s_font5_string[] = { 0 };
static s16 s_font5_pages[1];
static u16 s_font6_string[] = { 0 };
static s16 s_font6_pages[1];
static u16 s_font7_string[] = { 0 };
static s16 s_font7_pages[1];
static u16 s_font8_string[] = { 0 };
static s16 s_font8_pages[1];
static u16 s_font9_string[] = { 0 };
static s16 s_font9_pages[1];
static u16 s_font10_string[] = { 0 };
static s16 s_font10_pages[1];
static u16 s_font11_string[] = { 0 };
static s16 s_font11_pages[1];
static u16 s_font12_string[] = { 0 };
static s16 s_font12_pages[1];
static u16 s_font13_string[] = { 0 };
static s16 s_font13_pages[1];
static u16 s_font14_string[] = { 0 };
static s16 s_font14_pages[1];
static u16 s_font15_string[] = { 0 };
static s16 s_font15_pages[1];
static u16 s_font16_string[] = { 0 };
static s16 s_font16_pages[1];
static u16 s_font17_string[] = { 0 };
static s16 s_font17_pages[1];
static u16 s_font18_string[] = { 0 };
static s16 s_font18_pages[1];
static u16 s_font19_string[] = { 0 };
static s16 s_font19_pages[1];
static u16 s_font20_string[] = { 0 };
static s16 s_font20_pages[1];
static u16 s_font21_string[] = { 0 };
static s16 s_font21_pages[1];
static u16 s_font22_string[] = { 0 };
static s16 s_font22_pages[1];
static u16 s_font23_string[] = { 0 };
static s16 s_font23_pages[1];
static u16 s_font24_string[] = { 0 };
static s16 s_font24_pages[1];
static u16 s_font25_string[] = { 0 };
static s16 s_font25_pages[1];
static u16 s_font26_string[] = { 0 };
static s16 s_font26_pages[1];
static u16 s_font27_string[] = { 0 };
static s16 s_font27_pages[1];
static u16 s_font28_string[] = { 0 };
static s16 s_font28_pages[1];
static u16 s_font29_string[] = { 0 };
static s16 s_font29_pages[1];
static u16 s_font30_string[] = { 0 };
static s16 s_font30_pages[1];
static u16 s_font31_string[] = { 0 };
static s16 s_font31_pages[1];
static u16 s_font32_string[] = { 0 };
static s16 s_font32_pages[1];
static u16 s_font33_string[] = { 0 };
static s16 s_font33_pages[1];
static u16 s_font34_string[] = { 0 };
static s16 s_font34_pages[1];
static u16 s_font35_string[] = { 0 };
static s16 s_font35_pages[1];
static u16 s_font36_string[] = { 0 };
static s16 s_font36_pages[1];
static u16 s_font37_string[] = { 0 };
static s16 s_font37_pages[1];
static u16 s_font38_string[] = { 0 };
static s16 s_font38_pages[1];
static u16 s_font39_string[] = { 0 };
static s16 s_font39_pages[1];
static u16 s_font40_string[] = { 0 };
static s16 s_font40_pages[1];
static u16 s_font41_string[] = { 0 };
static s16 s_font41_pages[1];
static u16 s_font42_string[] = { 0 };
static s16 s_font42_pages[1];
static u16 s_font43_string[] = { 0 };
static s16 s_font43_pages[1];
static u16 s_font44_string[] = { 0 };
static s16 s_font44_pages[1];
static u16 s_font45_string[] = { 0 };
static s16 s_font45_pages[1];
static u16 s_font46_string[] = { 0 };
static s16 s_font46_pages[1];
static u16 s_font47_string[] = { 0 };
static s16 s_font47_pages[1];
static u16 s_font48_string[] = { 0 };
static s16 s_font48_pages[1];
static u16 s_font49_string[] = { 0 };
static s16 s_font49_pages[1];
static u16 s_font50_string[] = { 0 };
static s16 s_font50_pages[1];
static u16 s_font51_string[] = { 0 };
static s16 s_font51_pages[1];
static u16 s_font52_string[] = { 0 };
static s16 s_font52_pages[1];
static u16 s_font53_string[] = { 0 };
static s16 s_font53_pages[1];
static u16 s_font54_string[] = { 0 };
static s16 s_font54_pages[1];
static u16 s_font55_string[] = { 0 };
static s16 s_font55_pages[1];
static u16 s_font56_string[] = { 0 };
static s16 s_font56_pages[1];
static u16 s_font57_string[] = { 0 };
static s16 s_font57_pages[1];
static u16 s_font58_string[] = { 0 };
static s16 s_font58_pages[1];
static u16 s_font59_string[] = { 0 };
static s16 s_font59_pages[1];
static u16 s_font60_string[] = { 0 };
static s16 s_font60_pages[1];
static u16 s_font61_string[] = { 0 };
static s16 s_font61_pages[1];
static u16 s_font62_string[] = { 0 };
static s16 s_font62_pages[1];
static u16 s_font63_string[] = { 0 };
static s16 s_font63_pages[1];
static u16 s_font64_string[] = { 0 };
static s16 s_font64_pages[1];
static u16 s_font65_string[] = { 0 };
static s16 s_font65_pages[1];
static u16 s_font66_string[] = { 0 };
static s16 s_font66_pages[1];
static u16 s_font67_string[] = { 0 };
static s16 s_font67_pages[1];
static u16 s_font68_string[] = { 0 };
static s16 s_font68_pages[1];
static u16 s_font69_string[] = { 0 };
static s16 s_font69_pages[1];
static u16 s_font70_string[] = { 0 };
static s16 s_font70_pages[1];
static u16 s_font71_string[] = { 0 };
static s16 s_font71_pages[1];
static u16 s_font72_string[] = { 0 };
static s16 s_font72_pages[1];
static u16 s_font73_string[] = { 0 };
static s16 s_font73_pages[1];
static u16 s_font74_string[] = { 0 };
static s16 s_font74_pages[1];
static u16 s_font75_string[] = { 0 };
static s16 s_font75_pages[1];
static u16 s_font76_string[] = { 0 };
static s16 s_font76_pages[1];
static u16 s_font77_string[] = { 0 };
static s16 s_font77_pages[1];
static u16 s_font78_string[] = { 0 };
static s16 s_font78_pages[1];
static u16 s_font79_string[] = { 0 };
static s16 s_font79_pages[1];
static u16 s_font80_string[] = { 0 };
static s16 s_font80_pages[1];
static u16 s_font81_string[] = { 0 };
static s16 s_font81_pages[1];
static u16 s_font82_string[] = { 0 };
static s16 s_font82_pages[1];
static u16 s_font83_string[] = { 0 };
static s16 s_font83_pages[1];
static u16 s_font84_string[] = { 0 };
static s16 s_font84_pages[1];
static u16 s_font85_string[] = { 0 };
static s16 s_font85_pages[1];
static u16 s_font86_string[] = { 0 };
static s16 s_font86_pages[1];
static u16 s_font87_string[] = { 0 };
static s16 s_font87_pages[1];
static u16 s_font88_string[] = { 0 };
static s16 s_font88_pages[1];
static u16 s_font89_string[] = { 0 };
static s16 s_font89_pages[1];
static u16 s_font90_string[] = { 0 };
static s16 s_font90_pages[1];
static u16 s_font91_string[] = { 0 };
static s16 s_font91_pages[1];
static u16 s_font92_string[] = { 0 };
static s16 s_font92_pages[1];
static u16 s_font93_string[] = { 0 };
static s16 s_font93_pages[1];
static u16 s_font94_string[] = { 0 };
static s16 s_font94_pages[1];
static u16 s_font95_string[] = { 0 };
static s16 s_font95_pages[1];
static u16 s_font96_string[] = { 0 };
static s16 s_font96_pages[1];
static u16 s_font97_string[] = { 0 };
static s16 s_font97_pages[1];
static u16 s_font98_string[] = { 0 };
static s16 s_font98_pages[1];
static u16 s_font99_string[] = { 0 };
static s16 s_font99_pages[1];
static u16 s_font100_string[] = { 0 };
static s16 s_font100_pages[1];
static u16 s_font101_string[] = { 0 };
static s16 s_font101_pages[1];
static u16 s_font102_string[] = { 0 };
static s16 s_font102_pages[1];
static u16 s_font103_string[] = { 0 };
static s16 s_font103_pages[1];
static u16 s_font104_string[] = { 0 };
static s16 s_font104_pages[1];
static u16 s_font105_string[] = { 0 };
static s16 s_font105_pages[1];
static u16 s_font106_string[] = { 0 };
static s16 s_font106_pages[1];
static u16 s_font107_string[] = { 0 };
static s16 s_font107_pages[1];
static u16 s_font108_string[] = { 0 };
static s16 s_font108_pages[1];
static u16 s_font109_string[] = { 0 };
static s16 s_font109_pages[1];
static u16 s_font110_string[] = { 0 };
static s16 s_font110_pages[1];
static u16 s_font111_string[] = { 0 };
static s16 s_font111_pages[1];
static u16 s_font112_string[] = { 0 };
static s16 s_font112_pages[1];
static u16 s_font113_string[] = { 0 };
static s16 s_font113_pages[1];
static u16 s_font114_string[] = { 0 };
static s16 s_font114_pages[1];
static u16 s_font115_string[] = { 0 };
static s16 s_font115_pages[1];
static u16 s_font116_string[] = { 0 };
static s16 s_font116_pages[1];
static u16 s_font117_string[] = { 0 };
static s16 s_font117_pages[1];
static u16 s_font118_string[] = { 0 };
static s16 s_font118_pages[1];
static u16 s_font119_string[] = { 0 };
static s16 s_font119_pages[1];
static u16 s_font120_string[] = { 0 };
static s16 s_font120_pages[1];
static u16 s_font121_string[] = { 0 };
static s16 s_font121_pages[1];
static u16 s_font122_string[] = { 0 };
static s16 s_font122_pages[1];
static u16 s_font123_string[] = { 0 };
static s16 s_font123_pages[1];
static u16 s_font124_string[] = { 0 };
static s16 s_font124_pages[1];
static u16 s_font125_string[] = { 0 };
static s16 s_font125_pages[1];
static u16 s_font126_string[] = { 0 };
static s16 s_font126_pages[1];
static u16 s_font127_string[] = { 0 };
static s16 s_font127_pages[1];
static u16 s_font128_string[] = { 0 };
static s16 s_font128_pages[1];
static u16 s_font129_string[] = { 0 };
static s16 s_font129_pages[1];
static u16 s_font130_string[] = { 0 };
static s16 s_font130_pages[1];
static u16 s_font131_string[] = { 0 };
static s16 s_font131_pages[1];
static u16 s_font132_string[] = { 0 };
static s16 s_font132_pages[1];
static u16 s_font133_string[] = { 0 };
static s16 s_font133_pages[1];
static u16 s_font134_string[] = { 0 };
static s16 s_font134_pages[1];
static u16 s_font135_string[] = { 0 };
static s16 s_font135_pages[1];
static u16 s_font136_string[] = { 0 };
static s16 s_font136_pages[1];
static u16 s_font137_string[] = { 0 };
static s16 s_font137_pages[1];
static u16 s_font138_string[] = { 0 };
static s16 s_font138_pages[1];
static u16 s_font139_string[] = { 0 };
static s16 s_font139_pages[1];
static u16 s_font140_string[] = { 0 };
static s16 s_font140_pages[1];
static u16 s_font141_string[] = { 0 };
static s16 s_font141_pages[1];
static u16 s_font142_string[] = { 0 };
static s16 s_font142_pages[1];
static u16 s_font143_string[] = { 0 };
static s16 s_font143_pages[1];
static u16 s_font144_string[] = { 0 };
static s16 s_font144_pages[1];

struct FontParams FONT_DEFINITIONS[145] =
{
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font0_string, .pageIdx = s_font0_pages, .unk34 = 1.0f }, /* 0: NULL */
    { .bmpId = 6, .spaceWidth = 8, .lineHeight = 16, .firstChar = 32, .lastChar = 122, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.0078125f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font1_string, .pageIdx = s_font1_pages, .unk34 = 1.0f }, /* 1: NULL */
    { .bmpId = 31, .spaceWidth = 12, .lineHeight = 12, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0052083334885537624f, .unk1C = 0.010416666977107525f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font2_string, .pageIdx = s_font2_pages, .unk34 = 1.0f }, /* 2: NULL */
    { .bmpId = -1, .spaceWidth = 24, .lineHeight = 24, .firstChar = 0, .lastChar = 0, .charsPerRow = 1, .unk10 = 1.0f, .unk14 = 1.0f, .unk18 = 0.0416666679084301f, .unk1C = 0.0416666679084301f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x240, .string = s_font3_string, .pageIdx = s_font3_pages, .unk34 = 1.0f }, /* 3: asc_24x24 */
    { .bmpId = 85, .spaceWidth = 16, .lineHeight = 16, .firstChar = 32, .lastChar = 91, .charsPerRow = 12, .unk10 = 0.0833333358168602f, .unk14 = 0.20000000298023224f, .unk18 = 0.0052083334885537624f, .unk1C = 0.012500000186264515f, .unk20 = 0, .unk21 = 1, .unk22 = 1, .unk23 = 1, .size = 0x0, .string = s_font4_string, .pageIdx = s_font4_pages, .unk34 = 1.0f }, /* 4: NULL */
    { .bmpId = 85, .spaceWidth = 16, .lineHeight = 16, .firstChar = 32, .lastChar = 91, .charsPerRow = 12, .unk10 = 0.0833333358168602f, .unk14 = 0.20000000298023224f, .unk18 = 0.0052083334885537624f, .unk1C = 0.012500000186264515f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font5_string, .pageIdx = s_font5_pages, .unk34 = 1.0f }, /* 5: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font6_string, .pageIdx = s_font6_pages, .unk34 = 1.0f }, /* 6: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font7_string, .pageIdx = s_font7_pages, .unk34 = 1.0f }, /* 7: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font8_string, .pageIdx = s_font8_pages, .unk34 = 1.0f }, /* 8: NULL */
    { .bmpId = 80, .spaceWidth = 72, .lineHeight = 64, .firstChar = 48, .lastChar = 96, .charsPerRow = 7, .unk10 = 0.140625f, .unk14 = 0.1428571492433548f, .unk18 = 0.001953125f, .unk1C = 0.0022321429569274187f, .unk20 = 0, .unk21 = 1, .unk22 = 0, .unk23 = 2, .size = 0x0, .string = s_font9_string, .pageIdx = s_font9_pages, .unk34 = 1.0f }, /* 9: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font10_string, .pageIdx = s_font10_pages, .unk34 = 1.0f }, /* 10: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font11_string, .pageIdx = s_font11_pages, .unk34 = 1.0f }, /* 11: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font12_string, .pageIdx = s_font12_pages, .unk34 = 1.0f }, /* 12: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font13_string, .pageIdx = s_font13_pages, .unk34 = 1.0f }, /* 13: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font14_string, .pageIdx = s_font14_pages, .unk34 = 1.0f }, /* 14: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font15_string, .pageIdx = s_font15_pages, .unk34 = 1.0f }, /* 15: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font16_string, .pageIdx = s_font16_pages, .unk34 = 1.0f }, /* 16: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font17_string, .pageIdx = s_font17_pages, .unk34 = 1.0f }, /* 17: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font18_string, .pageIdx = s_font18_pages, .unk34 = 1.0f }, /* 18: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font19_string, .pageIdx = s_font19_pages, .unk34 = 1.0f }, /* 19: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font20_string, .pageIdx = s_font20_pages, .unk34 = 1.0f }, /* 20: NULL */
    { .bmpId = 74, .spaceWidth = 24, .lineHeight = 24, .firstChar = 1, .lastChar = 25, .charsPerRow = 5, .unk10 = 0.1875f, .unk14 = 0.1875f, .unk18 = 0.0078125f, .unk1C = 0.0078125f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font21_string, .pageIdx = s_font21_pages, .unk34 = 1.0f }, /* 21: NULL */
    { .bmpId = 73, .spaceWidth = 24, .lineHeight = 24, .firstChar = 1, .lastChar = 25, .charsPerRow = 5, .unk10 = 0.1875f, .unk14 = 0.1875f, .unk18 = 0.0078125f, .unk1C = 0.0078125f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font22_string, .pageIdx = s_font22_pages, .unk34 = 1.0f }, /* 22: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font23_string, .pageIdx = s_font23_pages, .unk34 = 1.0f }, /* 23: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font24_string, .pageIdx = s_font24_pages, .unk34 = 1.0f }, /* 24: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font25_string, .pageIdx = s_font25_pages, .unk34 = 1.0f }, /* 25: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font26_string, .pageIdx = s_font26_pages, .unk34 = 1.0f }, /* 26: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font27_string, .pageIdx = s_font27_pages, .unk34 = 1.0f }, /* 27: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.03125f, .unk14 = 0.125f, .unk18 = 0.00390625f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font28_string, .pageIdx = s_font28_pages, .unk34 = 1.0f }, /* 28: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font29_string, .pageIdx = s_font29_pages, .unk34 = 1.0f }, /* 29: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.03125f, .unk14 = 0.125f, .unk18 = 0.00390625f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font30_string, .pageIdx = s_font30_pages, .unk34 = 1.0f }, /* 30: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font31_string, .pageIdx = s_font31_pages, .unk34 = 1.0f }, /* 31: NULL */
    { .bmpId = 787, .spaceWidth = 32, .lineHeight = 32, .firstChar = 32, .lastChar = 95, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.25f, .unk18 = 0.001953125f, .unk1C = 0.0078125f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font32_string, .pageIdx = s_font32_pages, .unk34 = 1.0f }, /* 32: NULL */
    { .bmpId = 798, .spaceWidth = 16, .lineHeight = 28, .firstChar = 48, .lastChar = 59, .charsPerRow = 6, .unk10 = 0.1666666716337204f, .unk14 = 0.5f, .unk18 = 0.010416666977107525f, .unk1C = 0.01785714365541935f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font33_string, .pageIdx = s_font33_pages, .unk34 = 1.0f }, /* 33: NULL */
    { .bmpId = 802, .spaceWidth = 64, .lineHeight = 34, .firstChar = 49, .lastChar = 53, .charsPerRow = 1, .unk10 = 1.0f, .unk14 = 0.1931818127632141f, .unk18 = 0.015625f, .unk1C = 0.005681818351149559f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font34_string, .pageIdx = s_font34_pages, .unk34 = 1.0f }, /* 34: NULL */
    { .bmpId = 803, .spaceWidth = 20, .lineHeight = 28, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.009999999776482582f, .unk1C = 0.01785714365541935f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font35_string, .pageIdx = s_font35_pages, .unk34 = 1.0f }, /* 35: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font36_string, .pageIdx = s_font36_pages, .unk34 = 1.0f }, /* 36: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font37_string, .pageIdx = s_font37_pages, .unk34 = 1.0f }, /* 37: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font38_string, .pageIdx = s_font38_pages, .unk34 = 1.0f }, /* 38: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font39_string, .pageIdx = s_font39_pages, .unk34 = 1.0f }, /* 39: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font40_string, .pageIdx = s_font40_pages, .unk34 = 1.0f }, /* 40: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font41_string, .pageIdx = s_font41_pages, .unk34 = 1.0f }, /* 41: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font42_string, .pageIdx = s_font42_pages, .unk34 = 1.0f }, /* 42: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font43_string, .pageIdx = s_font43_pages, .unk34 = 1.0f }, /* 43: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font44_string, .pageIdx = s_font44_pages, .unk34 = 1.0f }, /* 44: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font45_string, .pageIdx = s_font45_pages, .unk34 = 1.0f }, /* 45: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font46_string, .pageIdx = s_font46_pages, .unk34 = 1.0f }, /* 46: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font47_string, .pageIdx = s_font47_pages, .unk34 = 1.0f }, /* 47: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font48_string, .pageIdx = s_font48_pages, .unk34 = 1.0f }, /* 48: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font49_string, .pageIdx = s_font49_pages, .unk34 = 1.0f }, /* 49: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font50_string, .pageIdx = s_font50_pages, .unk34 = 1.0f }, /* 50: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font51_string, .pageIdx = s_font51_pages, .unk34 = 1.0f }, /* 51: NULL */
    { .bmpId = 5950, .spaceWidth = 32, .lineHeight = 24, .firstChar = 48, .lastChar = 51, .charsPerRow = 2, .unk10 = 0.5f, .unk14 = 0.5f, .unk18 = 0.015625f, .unk1C = 0.02083333395421505f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font52_string, .pageIdx = s_font52_pages, .unk34 = 1.0f }, /* 52: NULL */
    { .bmpId = 5949, .spaceWidth = 40, .lineHeight = 20, .firstChar = 48, .lastChar = 75, .charsPerRow = 7, .unk10 = 0.1428571492433548f, .unk14 = 0.25f, .unk18 = 0.0035714285913854837f, .unk1C = 0.012500000186264515f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 1, .size = 0x0, .string = s_font53_string, .pageIdx = s_font53_pages, .unk34 = 1.0f }, /* 53: NULL */
    { .bmpId = 5949, .spaceWidth = 40, .lineHeight = 20, .firstChar = 41, .lastChar = 68, .charsPerRow = 7, .unk10 = 0.1428571492433548f, .unk14 = 0.25f, .unk18 = 0.0035714285913854837f, .unk1C = 0.012500000186264515f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 1, .size = 0x0, .string = s_font54_string, .pageIdx = s_font54_pages, .unk34 = 1.0f }, /* 54: NULL */
    { .bmpId = 5949, .spaceWidth = 40, .lineHeight = 20, .firstChar = 34, .lastChar = 61, .charsPerRow = 7, .unk10 = 0.1428571492433548f, .unk14 = 0.25f, .unk18 = 0.0035714285913854837f, .unk1C = 0.012500000186264515f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 1, .size = 0x0, .string = s_font55_string, .pageIdx = s_font55_pages, .unk34 = 1.0f }, /* 55: NULL */
    { .bmpId = 5949, .spaceWidth = 40, .lineHeight = 20, .firstChar = 27, .lastChar = 54, .charsPerRow = 7, .unk10 = 0.1428571492433548f, .unk14 = 0.25f, .unk18 = 0.0035714285913854837f, .unk1C = 0.012500000186264515f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 1, .size = 0x0, .string = s_font56_string, .pageIdx = s_font56_pages, .unk34 = 1.0f }, /* 56: NULL */
    { .bmpId = 5951, .spaceWidth = 40, .lineHeight = 28, .firstChar = 48, .lastChar = 54, .charsPerRow = 7, .unk10 = 0.1428571492433548f, .unk14 = 1.0f, .unk18 = 0.0035714285913854837f, .unk1C = 0.0357142873108387f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font57_string, .pageIdx = s_font57_pages, .unk34 = 1.0f }, /* 57: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font58_string, .pageIdx = s_font58_pages, .unk34 = 1.0f }, /* 58: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font59_string, .pageIdx = s_font59_pages, .unk34 = 1.0f }, /* 59: NULL */
    { .bmpId = 1563, .spaceWidth = 19, .lineHeight = 13, .firstChar = 0, .lastChar = 25, .charsPerRow = 5, .unk10 = 0.18269230425357819f, .unk14 = 0.16249999403953552f, .unk18 = 0.009615384973585606f, .unk1C = 0.012500000186264515f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font60_string, .pageIdx = s_font60_pages, .unk34 = 1.0f }, /* 60: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font61_string, .pageIdx = s_font61_pages, .unk34 = 1.0f }, /* 61: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font62_string, .pageIdx = s_font62_pages, .unk34 = 1.0f }, /* 62: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font63_string, .pageIdx = s_font63_pages, .unk34 = 1.0f }, /* 63: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font64_string, .pageIdx = s_font64_pages, .unk34 = 1.0f }, /* 64: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font65_string, .pageIdx = s_font65_pages, .unk34 = 1.0f }, /* 65: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font66_string, .pageIdx = s_font66_pages, .unk34 = 1.0f }, /* 66: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font67_string, .pageIdx = s_font67_pages, .unk34 = 1.0f }, /* 67: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font68_string, .pageIdx = s_font68_pages, .unk34 = 1.0f }, /* 68: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font69_string, .pageIdx = s_font69_pages, .unk34 = 1.0f }, /* 69: NULL */
    { .bmpId = 1814, .spaceWidth = 28, .lineHeight = 16, .firstChar = 49, .lastChar = 64, .charsPerRow = 4, .unk10 = 0.25f, .unk14 = 0.25f, .unk18 = 0.008928571827709675f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font70_string, .pageIdx = s_font70_pages, .unk34 = 1.0f }, /* 70: NULL */
    { .bmpId = 1815, .spaceWidth = 64, .lineHeight = 40, .firstChar = 49, .lastChar = 56, .charsPerRow = 2, .unk10 = 0.5f, .unk14 = 0.25f, .unk18 = 0.0078125f, .unk1C = 0.0062500000931322575f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font71_string, .pageIdx = s_font71_pages, .unk34 = 1.0f }, /* 71: NULL */
    { .bmpId = 1818, .spaceWidth = 18, .lineHeight = 18, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.1875f, .unk14 = 0.44999998807907104f, .unk18 = 0.010416666977107525f, .unk1C = 0.02500000037252903f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font72_string, .pageIdx = s_font72_pages, .unk34 = 1.0f }, /* 72: NULL */
    { .bmpId = 1821, .spaceWidth = 20, .lineHeight = 24, .firstChar = 48, .lastChar = 58, .charsPerRow = 6, .unk10 = 0.1666666716337204f, .unk14 = 0.5f, .unk18 = 0.008333333767950535f, .unk1C = 0.02083333395421505f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font73_string, .pageIdx = s_font73_pages, .unk34 = 1.0f }, /* 73: NULL */
    { .bmpId = 1827, .spaceWidth = 16, .lineHeight = 20, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.012500000186264515f, .unk1C = 0.02500000037252903f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font74_string, .pageIdx = s_font74_pages, .unk34 = 1.0f }, /* 74: NULL */
    { .bmpId = 1819, .spaceWidth = 10, .lineHeight = 18, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.1785714328289032f, .unk14 = 0.44999998807907104f, .unk18 = 0.01785714365541935f, .unk1C = 0.02500000037252903f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font75_string, .pageIdx = s_font75_pages, .unk34 = 1.0f }, /* 75: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font76_string, .pageIdx = s_font76_pages, .unk34 = 1.0f }, /* 76: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font77_string, .pageIdx = s_font77_pages, .unk34 = 1.0f }, /* 77: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font78_string, .pageIdx = s_font78_pages, .unk34 = 1.0f }, /* 78: NULL */
    { .bmpId = 1841, .spaceWidth = 16, .lineHeight = 16, .firstChar = 48, .lastChar = 62, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.3333333432674408f, .unk18 = 0.012500000186264515f, .unk1C = 0.02083333395421505f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font79_string, .pageIdx = s_font79_pages, .unk34 = 1.0f }, /* 79: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font80_string, .pageIdx = s_font80_pages, .unk34 = 1.0f }, /* 80: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font81_string, .pageIdx = s_font81_pages, .unk34 = 1.0f }, /* 81: NULL */
    { .bmpId = 2820, .spaceWidth = 28, .lineHeight = 16, .firstChar = 48, .lastChar = 63, .charsPerRow = 4, .unk10 = 0.25f, .unk14 = 0.25f, .unk18 = 0.008928571827709675f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font82_string, .pageIdx = s_font82_pages, .unk34 = 1.0f }, /* 82: NULL */
    { .bmpId = 2818, .spaceWidth = 16, .lineHeight = 24, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.012500000186264515f, .unk1C = 0.02083333395421505f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font83_string, .pageIdx = s_font83_pages, .unk34 = 1.0f }, /* 83: NULL */
    { .bmpId = 2839, .spaceWidth = 16, .lineHeight = 20, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.012500000186264515f, .unk1C = 0.02500000037252903f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font84_string, .pageIdx = s_font84_pages, .unk34 = 1.0f }, /* 84: NULL */
    { .bmpId = 2840, .spaceWidth = 12, .lineHeight = 16, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.01666666753590107f, .unk1C = 0.03125f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font85_string, .pageIdx = s_font85_pages, .unk34 = 1.0f }, /* 85: NULL */
    { .bmpId = 2841, .spaceWidth = 16, .lineHeight = 16, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.012500000186264515f, .unk1C = 0.03125f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font86_string, .pageIdx = s_font86_pages, .unk34 = 1.0f }, /* 86: NULL */
    { .bmpId = 2837, .spaceWidth = 12, .lineHeight = 16, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.01666666753590107f, .unk1C = 0.03125f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font87_string, .pageIdx = s_font87_pages, .unk34 = 1.0f }, /* 87: NULL */
    { .bmpId = 2852, .spaceWidth = 8, .lineHeight = 10, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.02500000037252903f, .unk1C = 0.05000000074505806f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font88_string, .pageIdx = s_font88_pages, .unk34 = 1.0f }, /* 88: NULL */
    { .bmpId = 2842, .spaceWidth = 8, .lineHeight = 12, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.02500000037252903f, .unk1C = 0.0416666679084301f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font89_string, .pageIdx = s_font89_pages, .unk34 = 1.0f }, /* 89: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font90_string, .pageIdx = s_font90_pages, .unk34 = 1.0f }, /* 90: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font91_string, .pageIdx = s_font91_pages, .unk34 = 1.0f }, /* 91: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font92_string, .pageIdx = s_font92_pages, .unk34 = 1.0f }, /* 92: NULL */
    { .bmpId = 1288, .spaceWidth = 16, .lineHeight = 20, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.012500000186264515f, .unk1C = 0.02500000037252903f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font93_string, .pageIdx = s_font93_pages, .unk34 = 1.0f }, /* 93: NULL */
    { .bmpId = 1293, .spaceWidth = 28, .lineHeight = 40, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.0071428571827709675f, .unk1C = 0.012500000186264515f, .unk20 = 1, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font94_string, .pageIdx = s_font94_pages, .unk34 = 1.0f }, /* 94: NULL */
    { .bmpId = 1294, .spaceWidth = 16, .lineHeight = 24, .firstChar = 48, .lastChar = 62, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.3333333432674408f, .unk18 = 0.012500000186264515f, .unk1C = 0.013888888992369175f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font95_string, .pageIdx = s_font95_pages, .unk34 = 1.0f }, /* 95: NULL */
    { .bmpId = 1290, .spaceWidth = 16, .lineHeight = 16, .firstChar = 48, .lastChar = 62, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.3333333432674408f, .unk18 = 0.012500000186264515f, .unk1C = 0.02083333395421505f, .unk20 = 0, .unk21 = 0, .unk22 = 1, .unk23 = 0, .size = 0x0, .string = s_font96_string, .pageIdx = s_font96_pages, .unk34 = 1.0f }, /* 96: NULL */
    { .bmpId = 1286, .spaceWidth = 20, .lineHeight = 24, .firstChar = 48, .lastChar = 62, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.3333333432674408f, .unk18 = 0.009999999776482582f, .unk1C = 0.013888888992369175f, .unk20 = 0, .unk21 = 0, .unk22 = 1, .unk23 = 0, .size = 0x0, .string = s_font97_string, .pageIdx = s_font97_pages, .unk34 = 1.0f }, /* 97: NULL */
    { .bmpId = 31, .spaceWidth = 12, .lineHeight = 12, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0052083334885537624f, .unk1C = 0.010416666977107525f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font98_string, .pageIdx = s_font98_pages, .unk34 = 1.0f }, /* 98: NULL */
    { .bmpId = 31, .spaceWidth = 12, .lineHeight = 12, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0052083334885537624f, .unk1C = 0.010416666977107525f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font99_string, .pageIdx = s_font99_pages, .unk34 = 1.0f }, /* 99: NULL */
    { .bmpId = 31, .spaceWidth = 12, .lineHeight = 12, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0052083334885537624f, .unk1C = 0.010416666977107525f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font100_string, .pageIdx = s_font100_pages, .unk34 = 1.0f }, /* 100: NULL */
    { .bmpId = 31, .spaceWidth = 12, .lineHeight = 12, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0052083334885537624f, .unk1C = 0.010416666977107525f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font101_string, .pageIdx = s_font101_pages, .unk34 = 1.0f }, /* 101: NULL */
    { .bmpId = 31, .spaceWidth = 12, .lineHeight = 12, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0052083334885537624f, .unk1C = 0.010416666977107525f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font102_string, .pageIdx = s_font102_pages, .unk34 = 1.0f }, /* 102: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font103_string, .pageIdx = s_font103_pages, .unk34 = 1.0f }, /* 103: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font104_string, .pageIdx = s_font104_pages, .unk34 = 1.0f }, /* 104: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font105_string, .pageIdx = s_font105_pages, .unk34 = 1.0f }, /* 105: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font106_string, .pageIdx = s_font106_pages, .unk34 = 1.0f }, /* 106: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font107_string, .pageIdx = s_font107_pages, .unk34 = 1.0f }, /* 107: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font108_string, .pageIdx = s_font108_pages, .unk34 = 1.0f }, /* 108: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font109_string, .pageIdx = s_font109_pages, .unk34 = 1.0f }, /* 109: NULL */
    { .bmpId = 3603, .spaceWidth = 16, .lineHeight = 20, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.012500000186264515f, .unk1C = 0.02500000037252903f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font110_string, .pageIdx = s_font110_pages, .unk34 = 1.0f }, /* 110: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font111_string, .pageIdx = s_font111_pages, .unk34 = 1.0f }, /* 111: NULL */
    { .bmpId = 3606, .spaceWidth = 64, .lineHeight = 40, .firstChar = 49, .lastChar = 56, .charsPerRow = 2, .unk10 = 0.5f, .unk14 = 0.25f, .unk18 = 0.0078125f, .unk1C = 0.0062500000931322575f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font112_string, .pageIdx = s_font112_pages, .unk34 = 1.0f }, /* 112: NULL */
    { .bmpId = 3600, .spaceWidth = 28, .lineHeight = 16, .firstChar = 49, .lastChar = 64, .charsPerRow = 4, .unk10 = 0.25f, .unk14 = 0.25f, .unk18 = 0.008928571827709675f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font113_string, .pageIdx = s_font113_pages, .unk34 = 1.0f }, /* 113: NULL */
    { .bmpId = 3620, .spaceWidth = 16, .lineHeight = 16, .firstChar = 48, .lastChar = 62, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.3333333432674408f, .unk18 = 0.012500000186264515f, .unk1C = 0.02083333395421505f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font114_string, .pageIdx = s_font114_pages, .unk34 = 1.0f }, /* 114: NULL */
    { .bmpId = 3623, .spaceWidth = 18, .lineHeight = 18, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.1875f, .unk14 = 0.44999998807907104f, .unk18 = 0.010416666977107525f, .unk1C = 0.02500000037252903f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font115_string, .pageIdx = s_font115_pages, .unk34 = 1.0f }, /* 115: NULL */
    { .bmpId = 3624, .spaceWidth = 10, .lineHeight = 18, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.1785714328289032f, .unk14 = 0.44999998807907104f, .unk18 = 0.01785714365541935f, .unk1C = 0.02500000037252903f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font116_string, .pageIdx = s_font116_pages, .unk34 = 1.0f }, /* 116: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font117_string, .pageIdx = s_font117_pages, .unk34 = 1.0f }, /* 117: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font118_string, .pageIdx = s_font118_pages, .unk34 = 1.0f }, /* 118: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font119_string, .pageIdx = s_font119_pages, .unk34 = 1.0f }, /* 119: NULL */
    { .bmpId = 3627, .spaceWidth = 20, .lineHeight = 24, .firstChar = 48, .lastChar = 58, .charsPerRow = 6, .unk10 = 0.1666666716337204f, .unk14 = 0.5f, .unk18 = 0.008333333767950535f, .unk1C = 0.02083333395421505f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font120_string, .pageIdx = s_font120_pages, .unk34 = 1.0f }, /* 120: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font121_string, .pageIdx = s_font121_pages, .unk34 = 1.0f }, /* 121: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font122_string, .pageIdx = s_font122_pages, .unk34 = 1.0f }, /* 122: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font123_string, .pageIdx = s_font123_pages, .unk34 = 1.0f }, /* 123: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font124_string, .pageIdx = s_font124_pages, .unk34 = 1.0f }, /* 124: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font125_string, .pageIdx = s_font125_pages, .unk34 = 1.0f }, /* 125: NULL */
    { .bmpId = 4658, .spaceWidth = 16, .lineHeight = 16, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.012500000186264515f, .unk1C = 0.03125f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font126_string, .pageIdx = s_font126_pages, .unk34 = 1.0f }, /* 126: NULL */
    { .bmpId = 4672, .spaceWidth = 12, .lineHeight = 12, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.01666666753590107f, .unk1C = 0.0416666679084301f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font127_string, .pageIdx = s_font127_pages, .unk34 = 1.0f }, /* 127: NULL */
    { .bmpId = 4635, .spaceWidth = 16, .lineHeight = 24, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.012500000186264515f, .unk1C = 0.02083333395421505f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font128_string, .pageIdx = s_font128_pages, .unk34 = 1.0f }, /* 128: NULL */
    { .bmpId = 4656, .spaceWidth = 16, .lineHeight = 20, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.012500000186264515f, .unk1C = 0.02500000037252903f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font129_string, .pageIdx = s_font129_pages, .unk34 = 1.0f }, /* 129: NULL */
    { .bmpId = 4671, .spaceWidth = 12, .lineHeight = 16, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.01666666753590107f, .unk1C = 0.03125f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font130_string, .pageIdx = s_font130_pages, .unk34 = 1.0f }, /* 130: NULL */
    { .bmpId = 4657, .spaceWidth = 8, .lineHeight = 12, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.02500000037252903f, .unk1C = 0.0416666679084301f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font131_string, .pageIdx = s_font131_pages, .unk34 = 1.0f }, /* 131: NULL */
    { .bmpId = 4654, .spaceWidth = 12, .lineHeight = 16, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.01666666753590107f, .unk1C = 0.03125f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font132_string, .pageIdx = s_font132_pages, .unk34 = 1.0f }, /* 132: NULL */
    { .bmpId = 4615, .spaceWidth = 8, .lineHeight = 10, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.02500000037252903f, .unk1C = 0.05000000074505806f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font133_string, .pageIdx = s_font133_pages, .unk34 = 1.0f }, /* 133: NULL */
    { .bmpId = 4646, .spaceWidth = 12, .lineHeight = 14, .firstChar = 48, .lastChar = 51, .charsPerRow = 2, .unk10 = 0.5f, .unk14 = 0.5f, .unk18 = 0.0416666679084301f, .unk1C = 0.0357142873108387f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font134_string, .pageIdx = s_font134_pages, .unk34 = 1.0f }, /* 134: NULL */
    { .bmpId = 4662, .spaceWidth = 16, .lineHeight = 20, .firstChar = 48, .lastChar = 57, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.012500000186264515f, .unk1C = 0.02500000037252903f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font135_string, .pageIdx = s_font135_pages, .unk34 = 1.0f }, /* 135: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font136_string, .pageIdx = s_font136_pages, .unk34 = 1.0f }, /* 136: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font137_string, .pageIdx = s_font137_pages, .unk34 = 1.0f }, /* 137: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font138_string, .pageIdx = s_font138_pages, .unk34 = 1.0f }, /* 138: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font139_string, .pageIdx = s_font139_pages, .unk34 = 1.0f }, /* 139: NULL */
    { .bmpId = 0, .spaceWidth = 0, .lineHeight = 0, .firstChar = 0, .lastChar = 0, .charsPerRow = 0, .unk10 = 0.0f /* was nan/inf */, .unk14 = 0.0f /* was nan/inf */, .unk18 = 0.0f /* was nan/inf */, .unk1C = 0.0f /* was nan/inf */, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font140_string, .pageIdx = s_font140_pages, .unk34 = 1.0f }, /* 140: NULL */
    { .bmpId = 0, .spaceWidth = 8, .lineHeight = 8, .firstChar = 0, .lastChar = 127, .charsPerRow = 16, .unk10 = 0.0625f, .unk14 = 0.125f, .unk18 = 0.0078125f, .unk1C = 0.015625f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font141_string, .pageIdx = s_font141_pages, .unk34 = 1.0f }, /* 141: NULL */
    { .bmpId = 90, .spaceWidth = 24, .lineHeight = 24, .firstChar = 0, .lastChar = 400, .charsPerRow = 20, .unk10 = 0.0476190485060215f, .unk14 = 0.0476190485060215f, .unk18 = 0.0019841270986944437f, .unk1C = 0.0019841270986944437f, .unk20 = 0, .unk21 = 0, .unk22 = 1, .unk23 = 0, .size = 0x0, .string = s_font142_string, .pageIdx = s_font142_pages, .unk34 = 1.0f }, /* 142: NULL */
    { .bmpId = 90, .spaceWidth = 24, .lineHeight = 24, .firstChar = 0, .lastChar = 400, .charsPerRow = 20, .unk10 = 0.0476190485060215f, .unk14 = 0.0476190485060215f, .unk18 = 0.0019841270986944437f, .unk1C = 0.0019841270986944437f, .unk20 = 0, .unk21 = 0, .unk22 = 1, .unk23 = 0, .size = 0x0, .string = s_font143_string, .pageIdx = s_font143_pages, .unk34 = 1.0f }, /* 143: NULL */
    { .bmpId = 2, .spaceWidth = 24, .lineHeight = 24, .firstChar = 0, .lastChar = 400, .charsPerRow = 5, .unk10 = 0.20000000298023224f, .unk14 = 0.5f, .unk18 = 0.008333333767950535f, .unk1C = 0.02083333395421505f, .unk20 = 0, .unk21 = 0, .unk22 = 0, .unk23 = 0, .size = 0x0, .string = s_font144_string, .pageIdx = s_font144_pages, .unk34 = 1.0f }, /* 144: NULL */
};
