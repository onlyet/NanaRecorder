#include "filter.h"

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#include <libavutil/channel_layout.h>

#ifdef __cplusplus
};
#endif  // __cplusplus

namespace onlyet {

void format_pad_arg(char* arg, int size, const FILTER_CTX& ctx) {
    snprintf(arg, size, "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%lld",
              ctx.time_base.num,
              ctx.time_base.den,
              ctx.sample_rate,
              av_get_sample_fmt_name(ctx.sample_fmt),
              (long long int)av_get_default_channel_layout(ctx.nb_channel));
}

}  // namespace onlyet
