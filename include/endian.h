// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 2022 LeavaTail
 */
#ifndef _ENDIAN_H
#define _ENDIAN_H

#include <byteswap.h>

#if __BYTE_ORDER == __BIG_ENDIAN
#define cpu_to_le8(x)        (x)
#define cpu_to_le16(x)       bswap_16(x)
#define cpu_to_le32(x)       bswap_32(x)
#define cpu_to_le64(x)       bswap_64(x)
#define le8_to_cpu(x)        (x)
#define le16_to_cpu(x)       bswap_16(x)
#define le32_to_cpu(x)       bswap_32(x)
#define le64_to_cpu(x)       bswap_64(x)
#else /* __BYTE_ORDER == __LITTLE_ENDIAN */
#define cpu_to_le8(x)        (x)
#define cpu_to_le16(x)       (x)
#define cpu_to_le32(x)       (x)
#define cpu_to_le64(x)       (x)
#define le8_to_cpu(x)        (x)
#define le16_to_cpu(x)       (x)
#define le32_to_cpu(x)       (x)
#define le64_to_cpu(x)       (x)
#endif /* __BYTE_ORDER == __LITTLE_ENDIAN */

#endif /*_ENDIAN_H */
