/**
 * MIT License
 * Copyright (c) 2020-present Leandro Zungri
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <log.h>
#include <stdarg.h>
#include <printf.h>
#include <stdbool.h>
#include <string.h>

#include <core.h>
#include <board/types.h>
#include <board/core.h>
#include <driver/core.h>
#include <dstruct/circbuf.h>
#include <utils/utils.h>
#include <time/core.h>
#include <time/timeconv.h>
#include <mm/heap.h>
#include <dstruct/list.h>
#include <component/timer.h>
#include <component/component.h>
#include <component/logger.h>
#include <process/core.h>


static char logb[CONFIG_LOG_BUFSIZE_BYTES];

DEF_STATIC_CIRCBUF(logcb, logb, sizeof(logb));


int __add_log_msg(bool sync, char *level, char *tag, char *fmt, ...) {
    // Discard log messages if we are not running in a process context
    if (!_laritos.process_mode) {
        return 0;
    }

    int ret = 0;
    char lineb[CONFIG_LOG_MAX_LINE_SIZE] = { 0 };

    time_t curtime = { 0 };
    calendar_t cal = { 0 };
    // Do not add any timestamp info if there are still some time-related
    // components yet to be loaded
    if (_laritos.components_loaded && time_get_ns_rtc_time(&curtime) >= 0) {
        if (epoch_to_localtime_calendar(curtime.secs, &cal) < 0) {
            memset(&cal, 0, sizeof(cal));
        }
    }

    pcb_t *pcb = process_get_current();
    int nchars = 0;

    if (level != NULL) {
        nchars = snprintf(lineb, sizeof(lineb), "%02d:%02d:%02d.%03d %3u %-6.6s %s %s: ",
                cal.hour, cal.min, cal.sec, NS_TO_MS(curtime.ns), pcb->pid, pcb->name, level, tag);
        // If the required number of chars is bigger than the size of the buffer, then truncate string
        if (nchars > sizeof(lineb)) {
            goto full_buf;
        }
    }

    va_list ap;
    va_start(ap, fmt);
    int nchars2 = vsnprintf(lineb + nchars, sizeof(lineb) - nchars, fmt, ap);
    va_end(ap);
    // If the required number of chars is bigger than the remaining buffer, then truncate string
    if (nchars2 > sizeof(lineb) - nchars) {
        goto full_buf;
    }

    ret = circbuf_nb_write(&logcb, lineb, nchars + nchars2);
    if (sync) {
        log_flush();
    }
    return ret;

full_buf:
    lineb[sizeof(lineb) - 1] = '\n';
    ret = circbuf_nb_write(&logcb, lineb, sizeof(lineb));
    if (sync) {
        log_flush();
    }
    return ret;
}

int log_flush(void) {
    if (!component_any_of(COMP_TYPE_LOGGER)) {
        return 0;
    }

    int bread = 0;
    char buf[CONFIG_LOG_MAX_LINE_SIZE * 5] = { 0 };
    while ((bread = circbuf_nb_read(&logcb, buf, sizeof(buf))) > 0) {
        component_t *c;
        for_each_component_type(c, COMP_TYPE_LOGGER) {
            logger_comp_t *l = (logger_comp_t *) c;
            l->ops.write(l, buf, bread, false);
        }
    }
    return 0;
}
