#include "qmdutil.h"

namespace util {

QString
formatDuration(const QTime &duration)
{
    if (duration < QTime(1, 0, 0)) {
        return duration.toString("m:ss");
    } else {
        return duration.toString("h:mm:ss");
    }
}

} // namespace util
