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

#include <stdbool.h>
#include <board/core.h>
#include <core.h>
#include <component/ticker.h>
#include <component/timer.h>
#include <component/vrtimer.h>
#include <component/component.h>
#include <time/tick.h>
#include <time/system-tick.h>
#include <math.h>
#include <dstruct/list.h>
#include <mm/heap.h>


static int ticker_cb(vrtimer_comp_t *t, void *data) {
    // Increment global tick
    tick_inc_os_ticks();

    ticker_comp_t *ticker = (ticker_comp_t *) data;
    ticker_cb_info_t *ti;
    list_for_each_entry(ti, &ticker->cbs, list) {
        insane_async("Executing ticker callback 0x%p(data=0x%p)", ti->cb, ti->data);
        if (ti->cb(ticker, ti->data) < 0) {
            error_async("Failed to execute callback 0x%p(data=0x%p)", ti->cb, ti->data);
        }
    }

    return 0;
}

static int ticker_pause(ticker_comp_t *t) {
    verbose_async("Pausing ticker '%s'", ((component_t *) t)->id);
    return t->vrtimer->ops.remove_vrtimer(t->vrtimer, ticker_cb, t, true);
}

static int ticker_resume(ticker_comp_t *t) {
    verbose_async("Resuming ticker '%s'", ((component_t *) t)->id);
    tick_t timer_ticks = t->vrtimer->hrtimer->curfreq / t->ticks_per_sec;
    if (timer_ticks == 0) {
        // If the timer resolution cannot handle the required ticks_per_sec, then
        // expire at the next tick
        timer_ticks = 1;
    }
    return t->vrtimer->ops.add_vrtimer(t->vrtimer, timer_ticks, ticker_cb, t, true);
}

int ticker_init(ticker_comp_t *t) {
    info("OS ticker frequency: %lu HZ", t->ticks_per_sec);
    // Reset global ticks
    tick_reset_os_ticks();

    INIT_LIST_HEAD(&t->cbs);

    return 0;
}

int ticker_deinit(ticker_comp_t *t) {
    return ticker_pause(t);
}

static int add_callback(ticker_comp_t *t, ticker_cb_t cb, void *data) {
    verbose_async("Adding ticker callback 0x%p(data=0x%p)", cb, data);

    ticker_cb_info_t *ti = calloc(1, sizeof(ticker_cb_info_t));
    if (ti == NULL) {
        error("Couldn't allocate memory for ticker_cb_info_t");
        return -1;
    }

    ti->cb = cb;
    ti->data = data;
    INIT_LIST_HEAD(&ti->list);

    list_add_tail(&ti->list, &t->cbs);
    return 0;
}

static int remove_callback(ticker_comp_t *t, ticker_cb_t cb, void *data) {
    ticker_cb_info_t *pos;
    ticker_cb_info_t *tmp;
    list_for_each_entry_safe(pos, tmp, &t->cbs, list) {
        if (pos->cb == cb && pos->data == data) {
            verbose_async("Removing ticker callback 0x%p(data=0x%p)", cb, data);
            list_del(&pos->list);
            free(pos);
            break;
        }
    }
    return 0;
}

int ticker_component_init(ticker_comp_t *t, board_comp_t *bcomp,
        int (*init)(component_t *c), int (*deinit)(component_t *c)) {

    if (component_init((component_t *) t, bcomp->id, bcomp, COMP_TYPE_TICKER, init, deinit) < 0) {
        error("Failed to initialize '%s' component", bcomp->id);
        return -1;
    }

    t->ops.add_callback = add_callback;
    t->ops.remove_callback = remove_callback;
    t->ops.pause = ticker_pause;
    t->ops.resume = ticker_resume;

    if (board_get_component_attr(bcomp, "vrtimer", (component_t **) &t->vrtimer) < 0) {
        error("Invalid or no virtual timer component specified in the board info");
        return -1;
    }

    board_get_int_attr_def(bcomp, "ticks_per_sec", (int *) &t->ticks_per_sec,
            min(CONFIG_TICKER_DEF_FREQ, t->vrtimer->hrtimer->maxfreq));

    if (t->ticks_per_sec == 0) {
        error("Ticks per second cannot be zero");
        return -1;
    }

    return 0;
}

int ticker_start_all(void) {
    // Start OS tickers
    component_t *comp;
    for_each_component_type(comp, COMP_TYPE_TICKER) {
        ticker_comp_t *ticker = (ticker_comp_t *) comp;
        info("Starting ticker '%s'", comp->id);
        if (ticker->ops.resume(ticker) < 0) {
            error("Could not start ticker %s", comp->id);
            return -1;
        }
    }
    return 0;
}



#ifdef CONFIG_TEST_CORE_COMPONENT_TICKER
#include __FILE__
#endif
