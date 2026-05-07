// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: © 2026 Efremov Alexey <4osogi@gmail.com>

#include <reoxide/logging.hh>

#if defined(PCODE_WEAVER_DEBUG) || defined(DEBUG)
#define PCODE_WEAVER_DEBUG_LOG(...) LOG_DEBUG(__VA_ARGS__)
#define PCODE_WEAVER_DEBUG_SEND(reox, message) (reox).sendString(message)
#else
#define PCODE_WEAVER_DEBUG_LOG(...)                                            \
  do {                                                                         \
  } while (0)
#define PCODE_WEAVER_DEBUG_SEND(reox, message)                                 \
  do {                                                                         \
  } while (0)
#endif