//#define NUM_FORMAT_INVALID                          0x000000ff
#define NUM_FORMAT_UNORM                            0x00000000
#define NUM_FORMAT_SNORM                            0x00000001
#define NUM_FORMAT_USCALED                          0x00000002
#define NUM_FORMAT_SSCALED                          0x00000003
#define NUM_FORMAT_UINT                             0x00000004
#define NUM_FORMAT_SINT                             0x00000005
#define NUM_FORMAT_SNORM_OGL                        0x00000006
#define NUM_FORMAT_FLOAT                            0x00000007

//#define DATA_FORMAT_INVALID                         0x00000000
#define DATA_FORMAT_8                               0x00000001
#define DATA_FORMAT_16                              0x00000002
#define DATA_FORMAT_8_8                             0x00000003
#define DATA_FORMAT_32                              0x00000004
#define DATA_FORMAT_16_16                           0x00000005
#define DATA_FORMAT_10_11_11                        0x00000006
//#define DATA_FORMAT_11_11_10                        0x00000007
#define DATA_FORMAT_10_10_10_2                      0x00000008
#define DATA_FORMAT_2_10_10_10                      0x00000009
#define DATA_FORMAT_8_8_8_8                         0x0000000a
#define DATA_FORMAT_32_32                           0x0000000b
#define DATA_FORMAT_16_16_16_16                     0x0000000c
#define DATA_FORMAT_32_32_32                        0x0000000d
#define DATA_FORMAT_32_32_32_32                     0x0000000e

#define TYPEMASK(NUM_FORMAT, DATA_FORMAT) (((NUM_FORMAT & 0x7) << 4) | (DATA_FORMAT & 0xf))
