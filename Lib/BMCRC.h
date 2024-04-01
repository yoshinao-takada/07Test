#if !defined(BMCRC_H)
#define BMCRC_H
#include "BMBase.h"
#pragma region CRC_CORE_DEFINITIONS
/*!
\biref CRC endec
*/
typedef struct {
    uint32_t MSBMask;
    uint32_t Shifter;
    uint32_t CRCPoly;
    uint32_t Input;
} BMCRC_t;
typedef BMCRC_t *BMCRC_pt;
typedef const BMCRC_t *BMCRC_cpt;

/*!
\brief Convert uint32_t Shifter to an array of uint8_t.
*/
#define BMCRC_SHIFTER_BYTES(__crc) ((const uint8_t*)&(__crc->Shifter))

/*!
\brief Put a byte.
*/
void BMCRC_Put(BMCRC_pt crc, uint8_t b);

/*!
\brief Put a byte string.
*/
void BMCRC_Puts(BMCRC_pt crc, const uint8_t* b, uint32_t count);

/*!
\brief Put post zero paddings.
\param count [in] byte count of zero paddings; e.g. 2 for CRC-16, 1 for CRC-8.
*/
void BMCRC_Put0s(BMCRC_pt crc, uint32_t count);
#pragma endregion CRC_CORE_DEFINITIONS

#pragma region CRC_MACROS
// default CRC seed
#define BMCRC_SEED16                0xC000

#define BMCRC_CCITT16_MSBMASK       0x10000
// Standard CCITT16 generator polynomial = x^16 + x^12 + x^5 + 1
// 0b 1 0001 0000 0010 0001 = 0x11021
#define BMCRC_CCITT16_CRCPOLY       0x11021

// CCITT8 generator polynomial = x^8 + x^2 + x + 1
// 0b 1 0000 0111
#define BMCRC_CCITT8_CRCPOLY        0x107
#define BMCRC_CCITT8_MSBMASK        0x100

#define BMCRC_CCITT16_INIT(_seed) { \
    BMCRC_CCITT16_MSBMASK, _seed, BMCRC_CCITT16_CRCPOLY, 0 }
#define BMCRC_CCITT8_INIT(_seed) { \
    BMCRC_CCITT8_MSBMASK, _seed, BMCRC_CCITT8_CRCPOLY, 0 }
#pragma endregion CRC_MACROS
#endif /* BMCRC_H */