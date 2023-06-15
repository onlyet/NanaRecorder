#ifndef ONLYET_DUMP_H
#define ONLYET_DUMP_H

#include <QString>

#define DyLoad_ ///< 动态加载dbghelp库

namespace Dump {
using Callback_Dump = void(*)();
void Init(const QString &dirpath, Callback_Dump after = nullptr);
}

#endif // !ONLYET_DUMP_H
