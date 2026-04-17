#ifndef LIBTUSK_MSVC_COMPAT_INTTYPES_H
#define LIBTUSK_MSVC_COMPAT_INTTYPES_H

#include <stdint.h>

#ifndef PRId8
#define PRId8 "hhd"
#endif
#ifndef PRIi8
#define PRIi8 "hhi"
#endif
#ifndef PRIo8
#define PRIo8 "hho"
#endif
#ifndef PRIu8
#define PRIu8 "hhu"
#endif
#ifndef PRIx8
#define PRIx8 "hhx"
#endif
#ifndef PRIX8
#define PRIX8 "hhX"
#endif

#ifndef PRId16
#define PRId16 "hd"
#endif
#ifndef PRIi16
#define PRIi16 "hi"
#endif
#ifndef PRIo16
#define PRIo16 "ho"
#endif
#ifndef PRIu16
#define PRIu16 "hu"
#endif
#ifndef PRIx16
#define PRIx16 "hx"
#endif
#ifndef PRIX16
#define PRIX16 "hX"
#endif

#ifndef PRId32
#define PRId32 "d"
#endif
#ifndef PRIi32
#define PRIi32 "i"
#endif
#ifndef PRIo32
#define PRIo32 "o"
#endif
#ifndef PRIu32
#define PRIu32 "u"
#endif
#ifndef PRIx32
#define PRIx32 "x"
#endif
#ifndef PRIX32
#define PRIX32 "X"
#endif

#ifndef PRId64
#define PRId64 "I64d"
#endif
#ifndef PRIi64
#define PRIi64 "I64i"
#endif
#ifndef PRIo64
#define PRIo64 "I64o"
#endif
#ifndef PRIu64
#define PRIu64 "I64u"
#endif
#ifndef PRIx64
#define PRIx64 "I64x"
#endif
#ifndef PRIX64
#define PRIX64 "I64X"
#endif

#ifndef PRIdLEAST8
#define PRIdLEAST8 PRId8
#endif
#ifndef PRIiLEAST8
#define PRIiLEAST8 PRIi8
#endif
#ifndef PRIoLEAST8
#define PRIoLEAST8 PRIo8
#endif
#ifndef PRIuLEAST8
#define PRIuLEAST8 PRIu8
#endif
#ifndef PRIxLEAST8
#define PRIxLEAST8 PRIx8
#endif
#ifndef PRIXLEAST8
#define PRIXLEAST8 PRIX8
#endif

#ifndef PRIdLEAST16
#define PRIdLEAST16 PRId16
#endif
#ifndef PRIiLEAST16
#define PRIiLEAST16 PRIi16
#endif
#ifndef PRIoLEAST16
#define PRIoLEAST16 PRIo16
#endif
#ifndef PRIuLEAST16
#define PRIuLEAST16 PRIu16
#endif
#ifndef PRIxLEAST16
#define PRIxLEAST16 PRIx16
#endif
#ifndef PRIXLEAST16
#define PRIXLEAST16 PRIX16
#endif

#ifndef PRIdLEAST32
#define PRIdLEAST32 PRId32
#endif
#ifndef PRIiLEAST32
#define PRIiLEAST32 PRIi32
#endif
#ifndef PRIoLEAST32
#define PRIoLEAST32 PRIo32
#endif
#ifndef PRIuLEAST32
#define PRIuLEAST32 PRIu32
#endif
#ifndef PRIxLEAST32
#define PRIxLEAST32 PRIx32
#endif
#ifndef PRIXLEAST32
#define PRIXLEAST32 PRIX32
#endif

#ifndef PRIdLEAST64
#define PRIdLEAST64 PRId64
#endif
#ifndef PRIiLEAST64
#define PRIiLEAST64 PRIi64
#endif
#ifndef PRIoLEAST64
#define PRIoLEAST64 PRIo64
#endif
#ifndef PRIuLEAST64
#define PRIuLEAST64 PRIu64
#endif
#ifndef PRIxLEAST64
#define PRIxLEAST64 PRIx64
#endif
#ifndef PRIXLEAST64
#define PRIXLEAST64 PRIX64
#endif

#ifndef PRIdFAST8
#define PRIdFAST8 PRId8
#endif
#ifndef PRIiFAST8
#define PRIiFAST8 PRIi8
#endif
#ifndef PRIoFAST8
#define PRIoFAST8 PRIo8
#endif
#ifndef PRIuFAST8
#define PRIuFAST8 PRIu8
#endif
#ifndef PRIxFAST8
#define PRIxFAST8 PRIx8
#endif
#ifndef PRIXFAST8
#define PRIXFAST8 PRIX8
#endif

#ifndef PRIdFAST16
#define PRIdFAST16 PRId16
#endif
#ifndef PRIiFAST16
#define PRIiFAST16 PRIi16
#endif
#ifndef PRIoFAST16
#define PRIoFAST16 PRIo16
#endif
#ifndef PRIuFAST16
#define PRIuFAST16 PRIu16
#endif
#ifndef PRIxFAST16
#define PRIxFAST16 PRIx16
#endif
#ifndef PRIXFAST16
#define PRIXFAST16 PRIX16
#endif

#ifndef PRIdFAST32
#define PRIdFAST32 PRId32
#endif
#ifndef PRIiFAST32
#define PRIiFAST32 PRIi32
#endif
#ifndef PRIoFAST32
#define PRIoFAST32 PRIo32
#endif
#ifndef PRIuFAST32
#define PRIuFAST32 PRIu32
#endif
#ifndef PRIxFAST32
#define PRIxFAST32 PRIx32
#endif
#ifndef PRIXFAST32
#define PRIXFAST32 PRIX32
#endif

#ifndef PRIdFAST64
#define PRIdFAST64 PRId64
#endif
#ifndef PRIiFAST64
#define PRIiFAST64 PRIi64
#endif
#ifndef PRIoFAST64
#define PRIoFAST64 PRIo64
#endif
#ifndef PRIuFAST64
#define PRIuFAST64 PRIu64
#endif
#ifndef PRIxFAST64
#define PRIxFAST64 PRIx64
#endif
#ifndef PRIXFAST64
#define PRIXFAST64 PRIX64
#endif

#ifndef PRIdMAX
#define PRIdMAX PRId64
#endif
#ifndef PRIiMAX
#define PRIiMAX PRIi64
#endif
#ifndef PRIoMAX
#define PRIoMAX PRIo64
#endif
#ifndef PRIuMAX
#define PRIuMAX PRIu64
#endif
#ifndef PRIxMAX
#define PRIxMAX PRIx64
#endif
#ifndef PRIXMAX
#define PRIXMAX PRIX64
#endif

#ifndef PRIdPTR
#define PRIdPTR "Id"
#endif
#ifndef PRIiPTR
#define PRIiPTR "Ii"
#endif
#ifndef PRIoPTR
#define PRIoPTR "Io"
#endif
#ifndef PRIuPTR
#define PRIuPTR "Iu"
#endif
#ifndef PRIxPTR
#define PRIxPTR "Ix"
#endif
#ifndef PRIXPTR
#define PRIXPTR "IX"
#endif

#ifndef SCNd8
#define SCNd8 "hhd"
#endif
#ifndef SCNi8
#define SCNi8 "hhi"
#endif
#ifndef SCNo8
#define SCNo8 "hho"
#endif
#ifndef SCNu8
#define SCNu8 "hhu"
#endif
#ifndef SCNx8
#define SCNx8 "hhx"
#endif

#ifndef SCNd16
#define SCNd16 "hd"
#endif
#ifndef SCNi16
#define SCNi16 "hi"
#endif
#ifndef SCNo16
#define SCNo16 "ho"
#endif
#ifndef SCNu16
#define SCNu16 "hu"
#endif
#ifndef SCNx16
#define SCNx16 "hx"
#endif

#ifndef SCNd32
#define SCNd32 "d"
#endif
#ifndef SCNi32
#define SCNi32 "i"
#endif
#ifndef SCNo32
#define SCNo32 "o"
#endif
#ifndef SCNu32
#define SCNu32 "u"
#endif
#ifndef SCNx32
#define SCNx32 "x"
#endif

#ifndef SCNd64
#define SCNd64 "I64d"
#endif
#ifndef SCNi64
#define SCNi64 "I64i"
#endif
#ifndef SCNo64
#define SCNo64 "I64o"
#endif
#ifndef SCNu64
#define SCNu64 "I64u"
#endif
#ifndef SCNx64
#define SCNx64 "I64x"
#endif

#ifndef SCNdMAX
#define SCNdMAX SCNd64
#endif
#ifndef SCNiMAX
#define SCNiMAX SCNi64
#endif
#ifndef SCNoMAX
#define SCNoMAX SCNo64
#endif
#ifndef SCNuMAX
#define SCNuMAX SCNu64
#endif
#ifndef SCNxMAX
#define SCNxMAX SCNx64
#endif

#ifndef SCNdPTR
#define SCNdPTR "Id"
#endif
#ifndef SCNiPTR
#define SCNiPTR "Ii"
#endif
#ifndef SCNoPTR
#define SCNoPTR "Io"
#endif
#ifndef SCNuPTR
#define SCNuPTR "Iu"
#endif
#ifndef SCNxPTR
#define SCNxPTR "Ix"
#endif

#endif