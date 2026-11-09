#ifndef ZUCCHINI_STANDALONE_BASE_NOTREACHED_H_
#define ZUCCHINI_STANDALONE_BASE_NOTREACHED_H_
#include "base/logging.h"
#define NOTREACHED() LOG(FATAL) << "NOTREACHED reached. "
#define NOTIMPLEMENTED() LOG(ERROR) << "Not implemented. "
#endif
