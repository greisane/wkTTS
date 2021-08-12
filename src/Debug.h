#ifndef WKTTS_DEBUG_H
#define WKTTS_DEBUG_H

extern bool useDebugPrint;
#define debugf(fmt, ...) if (useDebugPrint) printf("%s:%d: " fmt, __func__ , __LINE__, __VA_ARGS__);

#endif //WKTTS_DEBUG_H
