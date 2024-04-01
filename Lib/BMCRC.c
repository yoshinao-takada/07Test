#include "BMCRC.h"
// MSB Bit masks of typical size integers
#define BM_MSB32        0x80000000
#define BM_MSB16        0x8000
#define BM_MSB8         0x80

void BMCRC_Put(BMCRC_pt crc, uint8_t b)
{
    int32_t mask = BM_MSB8;
    crc->Input = (int32_t)b;
    for (int ib = 0; ib < 8; ib++)
    {
        // move Input MSB to Shifter LSB.
        crc->Shifter = (crc->Shifter << 1) | ((mask & crc->Input) ? 1 : 0);
        crc->Input <<= 1;
        // eliminate the shifter bit corresponding to CRC generator polynomial MSB
        // if they match; i.e. The shifter bit value is 1.
        if (crc->Shifter & crc->MSBMask)
        {
            crc->Shifter ^= crc->CRCPoly;
        }
    }
}

void BMCRC_Puts(BMCRC_pt crc, const uint8_t* b, uint32_t count)
{
    for (int32_t i = 0; i < count; i++)
    {
        BMCRC_Put(crc, b[i]);
    }
}

void BMCRC_Put0s(BMCRC_pt crc, uint32_t count)
{
    for (int32_t i = 0; i < count; i++)
    {
        BMCRC_Put(crc, 0);
    }
}
