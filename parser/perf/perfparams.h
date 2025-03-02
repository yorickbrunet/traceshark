// SPDX-License-Identifier: (GPL-2.0-or-later OR BSD-2-Clause)
/*
 * Traceshark - a visualizer for visualizing ftrace and perf traces
 * Copyright (C) 2015-2020, 2023  Viktor Rosendahl <viktor.rosendahl@gmail.com>
 *
 * This file is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 *
 *  a) This program is free software; you can redistribute it and/or
 *     modify it under the terms of the GNU General Public License as
 *     published by the Free Software Foundation; either version 2 of the
 *     License, or (at your option) any later version.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public
 *     License along with this library; if not, write to the Free
 *     Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 *     MA 02110-1301 USA
 *
 * Alternatively,
 *
 *  b) Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *     1. Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *     2. Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *     CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *     INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *     CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *     NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *     HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *     CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *     OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 *     EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PERFPARAMS_H
#define PERFPARAMS_H

#include "mm/stringpool.h"
#include "parser/traceevent.h"
#include "parser/paramhelpers.h"
#include "parser/perf/helpers.h"
#include "misc/errors.h"
#include "misc/string.h"
#include "misc/traceshark.h"
#include <cstring>
#include <cstdint>

#define perf_cpufreq_args_ok(EVENT) (EVENT.argc >= 2)
#define perf_cpufreq_cpu(EVENT)     (uint_after_pfix(EVENT, 1, FREQ_CPUID_PFIX))
#define perf_cpufreq_freq(EVENT)    (uint_after_pfix(EVENT, 0, FREQ_STATE_PFIX))

#define perf_cpuidle_args_ok(EVENT) (EVENT.argc >= 2)
#define perf_cpuidle_cpu(EVENT)     (uint_after_pfix(EVENT, 1, IDLE_CPUID_PFIX))
static vtl_always_inline int perf_cpuidle_state(const TraceEvent &event)
{
	int32_t state;
	uint32_t ustate;
	ustate = uint_after_pfix(event, 0, IDLE_STATE_PFIX);

	/* the string is a signed printed as unsigned :) */
	state = *((int*) &ustate);

	return state;
}

/* Normally we would require >= 5 but we don't need the first comm= arg */
#define perf_sched_migrate_args_ok(EVENT) (EVENT.argc >= 4)
#define perf_sched_migrate_destCPU(EVENT) (uint_after_pfix(EVENT,	\
							   EVENT.argc - 1, \
						           MIGRATE_DEST_PFIX))
#define perf_sched_migrate_origCPU(EVENT) (uint_after_pfix(EVENT,	\
							   EVENT.argc - 2, \
						           MIGRATE_ORIG_PFIX))
#define perf_sched_migrate_prio(EVENT) (uint_after_pfix(EVENT,		\
							EVENT.argc - 3, \
							MIGRATE_PRIO_PFIX))
#define perf_sched_migrate_pid(EVENT) (int_after_pfix(EVENT,		\
						      EVENT.argc - 4,	\
						      MIGRATE_PID_PFIX))


static vtl_always_inline
bool perf_sched_switch_parse(const TraceEvent &event,
			     sched_switch_handle& handle)
{
	int i;

	i = perf_sched_switch_find_arrow_(event, handle.perf.is_distro_style);
	if( i <= 0)
		return false;
	handle.perf.index = i;
	return true;
}

static vtl_always_inline taskstate_t
perf_sched_switch_handle_state(const TraceEvent &event,
			       const sched_switch_handle &handle)
{
	int i, j;
	TString st;

	i = handle.perf.index - 1;

	if (handle.perf.is_distro_style)
		goto distro_style;

	/* This is the regular format */
	for (; i >= 0; i--) {
		const TString *t = event.argv[i];
		if (!prefixcmp(t->ptr, SWITCH_PSTA_PFIX)) {
			for (j = t->len - 2; j > 0; j--) {
				if (t->ptr[j] == '=') {
					st.len = t->len - 1 - j;
					st.ptr = t->ptr + j + 1;
					return  sched_state_from_tstring_(&st);
				}
			}
		}
	}
	return TASK_STATE_PARSER_ERROR;

distro_style:
	/* This is the distro format */
	const TString *stateArgStr = event.argv[i];
	if (stateArgStr->len == 1 || stateArgStr->len == 2) {
		return sched_state_from_tstring_(stateArgStr);
	}
	return TASK_STATE_PARSER_ERROR;
}

static vtl_always_inline int
perf_sched_switch_handle_oldpid(const TraceEvent &event,
				const sched_switch_handle &handle)
{
	int idx = handle.perf.index;
	int oldpid;

	if (handle.perf.is_distro_style) {
		oldpid = int_after_char(event, idx - 3, ':');
	} else {
		oldpid = perf_sched_switch_handle_oldpid_newformat_(event,
								    handle);
	}
	return oldpid;
}

static vtl_always_inline int
perf_sched_switch_handle_newpid(const TraceEvent &event,
				const sched_switch_handle &handle)
{
	int newpid;

	if (handle.perf.is_distro_style) {
		newpid = int_after_char(event, event.argc - 2, ':');
	} else {
		//newpid = int_after_char(event, event.argc - 2, '=');
		newpid = perf_sched_switch_handle_newpid_newformat_(event,
								    handle);
	}
	return newpid;
}

static vtl_always_inline unsigned int
perf_sched_switch_handle_oldprio(const TraceEvent &event,
				 const sched_switch_handle &handle)
{
	int i = handle.perf.index;

	if (i <= 3)
		return ABSURD_UNSIGNED;

	if (handle.perf.is_distro_style) {
		return param_inside_braces(event, i - 2);
	} else {
		/*
		 * Since this function is not used, we just assume that the
		 * argument is in the usual place.
		 */
		return uint_after_char(event, i - 2, '=');
	}
}

static vtl_always_inline unsigned int
perf_sched_switch_handle_newprio(const TraceEvent &event,
				 const sched_switch_handle &handle)
{
	if (handle.perf.is_distro_style) {
		return param_inside_braces(event, event.argc - 1);
	} else {
		/*
		 * Since this function is not used, we just assume that the
		 * argument is in the usual place.
		 */
		return uint_after_char(event, event.argc - 1, '=');
	}
}

static vtl_always_inline const char *
perf_sched_switch_handle_newname_strdup_(const TraceEvent &event,
					 StringPool<> *pool,
					 const sched_switch_handle &handle)
{
	int i;
	int beginidx;
	int endidx;
	int len = 0;
	char *c;
	bool ok;
	char sbuf[TASKNAME_MAXLEN + 1];
	TString ts;
	const TString *retstr;

	c = &sbuf[0];
	ts.ptr = c;

	i = handle.perf.index;


	if (!handle.perf.is_distro_style) {
		const TString *first = event.argv[i + 1];
		beginidx = i + 2;
		endidx = event.argc - 3;
		copy_tstring_after_char_(first, '=', c, len, TASKNAME_MAXLEN,
					 ok);
		if (!ok)
			return NullStr;

		merge_args_into_cstring_nullterminate(event, beginidx, endidx,
						      c, len, TASKNAME_MAXLEN,
						      ok);
		if (!ok)
			return NullStr;
	} else {
		const TString *last = event.argv[event.argc - 2];
		beginidx = i + 1;
		endidx = event.argc - 3;

		merge_args_into_cstring(event, beginidx, endidx,
					c, len, TASKNAME_MAXLEN,
					ok);
		if (!ok)
			return NullStr;

		copy_tstring_before_char_(last, ':', c, len, TASKNAME_MAXLEN,
					  ok);
		if (!ok)
			return NullStr;
	}

	ts.len = len;
	retstr = pool->allocString(&ts, 0);
	if (retstr == nullptr)
		return NullStr;

	return retstr->ptr;
}

const char *
perf_sched_switch_handle_newname_strdup(const TraceEvent &event,
					StringPool<> *pool,
					const sched_switch_handle &handle);

static vtl_always_inline const char *
perf_sched_switch_handle_oldname_strdup_(const TraceEvent &event,
					 StringPool<> *pool,
					 const sched_switch_handle &handle)
{
	int i;
	int beginidx;
	int endidx;
	int len = 0;
	char *c;
	bool ok;
	char sbuf[TASKNAME_MAXLEN + 1];
	TString ts;
	const TString *retstr;

	c = &sbuf[0];
	ts.ptr = c;

	i = handle.perf.index;

	if (!handle.perf.is_distro_style) {
		const TString *first = event.argv[0];
		beginidx = 1;
		endidx = i - 4;
		copy_tstring_after_char_(first, '=', c, len,
					 TASKNAME_MAXLEN, ok);
		if (!ok)
			return NullStr;

		merge_args_into_cstring_nullterminate(event, beginidx, endidx,
						      c, len, TASKNAME_MAXLEN,
						      ok);
		if (!ok)
			return NullStr;

	} else {
		const TString *last = event.argv[i - 3];
		beginidx = 0;
		endidx = i - 4;

		merge_args_into_cstring(event, beginidx, endidx,
					c, len, TASKNAME_MAXLEN,
					ok);
		if (!ok)
			return NullStr;

		copy_tstring_before_char_(last, ':',
					  c, len, TASKNAME_MAXLEN,
					  ok);

		if (!ok)
			return NullStr;
	}
	ts.len = len;
	retstr = pool->allocString(&ts, 0);
	if (retstr == nullptr)
		return NullStr;

	return retstr->ptr;
}

const char *
perf_sched_switch_handle_oldname_strdup(const TraceEvent &event,
					StringPool<> *pool,
					const sched_switch_handle &handle);

/*
 * These functions for sched_wakeup assume that the arguments is in one of the
 * following formats:
 *
 * With a perf that uses libtraceevent, we would get something like this:
 *
 * <PNAME>:<PID> [<PRIO>] CPU:<CPU>
 * <PNAME>:<PID> [<PRIO>] success=1 CPU:<CPU>
 * <PNAME>:<PID> [<PRIO>]<CANT FIND FIELD success> CPU:<CPU>
 * (The above happens when a newer perf is used with an older libtraceeevent.)
 *
 * These are the old formats without libtraceevent:
 * comm=<PNAME> pid=<PID> prio=<PRIO> target_cpu=<CPU>
 * comm=<PNAME> pid=<PID> prio=<PRIO> success=1 target_cpu=<CPU>
 */

#define perf_sched_wakeup_args_ok(EVENT) (EVENT.argc >= 3)

/* The last argument is target_cpu, regardless of old or new */
#define perf_sched_wakeup_cpu(EVENT) (uint_after_pfix(EVENT, EVENT.argc - 1, \
						      WAKE_TCPU_PFIX))

static vtl_always_inline bool perf_sched_wakeup_success(const TraceEvent &event)
{
	const TString *ss = event.argv[event.argc - 2];

	if (prefixcmp(ss->ptr, WAKE_SUCC_PFIX) == 0)
		return int_after_char(event, event.argc - 2, '=');

	/*
	 * Here we could search through all arguments in case we would find the
	 * success field. Assume that wakeup is successful if no success field
	 * is found. We don't bother doing it because I am not aware of any
	 * kernel with a different format for the success field, or any kernel
	 * that would generate unsuccessful wakeup events.
	 */
	return true;
}

/*
 * Fix me, this doesn't work for negative prio, or if we have the format
 * <PNAME>:<PID> [<PRIO>]<CANT FIND FIELD success> CPU:<CPU>. We would need
 * to implement a param_inside_braces_or_cant() that can handle negative
 * numbers to solve this.
 *
 * Fixing this has low priority because:
 * - traceshark is curently not using this function. We do not consume the prio
 *   value anywhere.
 * - So far, I have only seen positive prio values
 * - This "[<PRIO>]<CANT" thing only happens when an old libtraceevent is used
 *   with a newer perf.
 *
 */
static vtl_always_inline unsigned int
perf_sched_wakeup_prio(const TraceEvent &event)
{
	const int lastidx = event.argc - 1;
	int idx = 0;
	int i;

	if (!prefixcmp(event.argv[lastidx]->ptr, WAKE_CPU_PFIX)) {
		/* libtraceevent output format: newer perf or Fedora */
		const int maxi = event.argc - 3;
		for (i = 0; i <= maxi; i++) {
			idx = event.argc - 2 - i;
			const TString *priostr = event.argv[idx];
			if (is_param_inside_braces(priostr))
				return param_inside_braces(event, idx);
		}
		return ABSURD_INT;
	} else if (!prefixcmp(event.argv[lastidx]->ptr, WAKE_TCPU_PFIX)) {
		/* older perf */
		const int maxi = event.argc - 2;
		for (i = 0; i <= maxi; i++) {
			idx = event.argc - 2 - i;
			const TString *priostr = event.argv[idx];
			if (!prefixcmp(priostr->ptr, WAKE_PRIO_PFIX))
				return uint_after_char(event, idx, '=');
		}
		return ABSURD_INT;
	} else {
		/* Hmmm, this would be a completely unknown format */
		return ABSURD_INT;
	}
}

static vtl_always_inline int perf_sched_wakeup_pid(const TraceEvent &event)
{
	const int lastidx = event.argc - 1;
	int idx = 0;

	if (!prefixcmp(event.argv[lastidx]->ptr, WAKE_CPU_PFIX)) {
		/* libtraceevent output format: newer perf or Fedora */
		for (idx = event.argc - 2; idx >= 1 ; idx--) {
			const TString *priostr = event.argv[idx];
			if (is_param_inside_braces_or_cant(priostr)) {
				idx--;
				break;
			}
		}
		return int_after_char(event, idx, ':');
	} else if (!prefixcmp(event.argv[lastidx]->ptr, WAKE_TCPU_PFIX)) {
		/* older perf */
		for (idx = event.argc - 3; idx >= 0; idx--) {
			const TString *pidstr = event.argv[idx];
			if (!prefixcmp(pidstr->ptr, WAKE_PID_PFIX))
				return int_after_char(event, idx, '=');
		}

		/*
		 * I would not expect this to be succesful, unless we encounter
		 * a previously unknown format. However, try with this string
		 * too. We know that event.argc >= 3 becaus presumably,
		 * the caller has checked this with
		 * perf_sched_wakeup_args_ok()
		 */
		idx = event.argc - 2;
		const TString *pidstr = event.argv[idx];
		if (!prefixcmp(pidstr->ptr, WAKE_PID_PFIX))
			return int_after_char(event, idx, '=');
		return ABSURD_INT;
	} else {
		/* Hmmm, this would be a completely unknown format */
		return ABSURD_INT;
	}
}

static vtl_always_inline const char *
perf_sched_wakeup_name_strdup_(const TraceEvent &event, StringPool<> *pool)
{
	const int lastidx = event.argc - 1;
	int beginidx;
	int endidx;
	const TString *retstr;
	char *c;
	const TString *first;
	const TString *last;
	bool ok;
	char sbuf[TASKNAME_MAXLEN + 1];
	int len = 0;
	TString ts;

	c = &sbuf[0];
	ts.ptr = c;

	if (!prefixcmp(event.argv[lastidx]->ptr, WAKE_CPU_PFIX)) {
		beginidx = 0;

		for (endidx = event.argc - 2; endidx > 0; endidx--) {
			const TString *priostr = event.argv[endidx];
			if (is_param_inside_braces_or_cant(priostr))
				break;
		}
		if (endidx <= 0)
			return NullStr;
		endidx = endidx - 2;

		merge_args_into_cstring(event, beginidx, endidx, c, len,
					TASKNAME_MAXLEN, ok);

		if (!ok)
			return NullStr;

		last = event.argv[endidx + 1];
		copy_tstring_before_char_(last, ':', c, len, TASKNAME_MAXLEN,
					  ok);
		if (!ok)
			return NullStr;

	} else if (!prefixcmp(event.argv[lastidx]->ptr, WAKE_TCPU_PFIX)) {
		beginidx = 1;

		for (endidx = event.argc - 2; endidx > 0; endidx--) {
			const TString *pidstr = event.argv[endidx];
			if (!prefixcmp(pidstr->ptr, WAKE_PID_PFIX))
				break;
		}
		if (endidx <= 0)
			return NullStr;
		endidx--;

		first = event.argv[0];
		copy_tstring_after_char_(first, '=', c, len, TASKNAME_MAXLEN,
					 ok);
		if (!ok)
			return NullStr;

		merge_args_into_cstring_nullterminate(event, beginidx, endidx,
						      c, len, TASKNAME_MAXLEN,
						      ok);
		if (!ok)
			return NullStr;
	} else
		return NullStr;

	ts.len = len;
	retstr = pool->allocString(&ts, 0);
	if (retstr == nullptr)
		return NullStr;

	return retstr->ptr;
}

const char *perf_sched_wakeup_name_strdup(const TraceEvent &event,
					  StringPool<> *pool);

#define perf_sched_process_fork_args_ok(EVENT) (EVENT.argc >= 4)

static vtl_always_inline
int perf_sched_process_fork_childpid(const TraceEvent &event) {
	int i;
	int endidx = event.argc - 1;
	int guessidx = endidx;

	if (likely(prefixcmp(event.argv[guessidx]->ptr, "child_pid=") == 0))
		return int_after_char(event, guessidx, '=');

	for (i = endidx - 1; i > 0; i--) {
		if (prefixcmp(event.argv[i]->ptr, "child_pid=") == 0)
			break;
	}
	if (i < 0)
		return ABSURD_INT;

	return int_after_char(event, i, '=');
}

static vtl_always_inline
int perf_sched_process_fork_parent_pid(const TraceEvent &event) {
	int i;
	int endidx = event.argc - 1;
	int guessidx = 1;

	if (prefixcmp(event.argv[guessidx]->ptr, "pid=") == 0 &&
	    prefixcmp(event.argv[guessidx + 1]->ptr, "child_comm=") == 0)
		return int_after_char(event, guessidx, '=');

	for (i = guessidx; i < endidx - 1; i++) {
		if (prefixcmp(event.argv[i]->ptr, "pid=") == 0 &&
		    prefixcmp(event.argv[i + 1]->ptr, "child_comm=") == 0)
			break;
	}
	if (i >= endidx)
		return ABSURD_INT;

	return int_after_char(event, i, '=');
}

static vtl_always_inline const char *
perf_sched_process_fork_childname_strdup_(const TraceEvent &event,
					  StringPool<> *pool)
{
	int i;
	int beginidx;
	const int endidx = event.argc - 2;
	char *c;
	int len = 0;
	bool ok;
	const TString *first;
	char sbuf[TASKNAME_MAXLEN + 1];
	TString ts;
	const TString *retstr;

	c = &sbuf[0];
	ts.ptr = c;

	for (i = 2; i <= endidx; i++) {
		if (!prefixcmp(event.argv[i - 1]->ptr, "pid=") &&
		    !prefixcmp(event.argv[i]->ptr,     "child_comm="))
			break;
	}
	if (i > endidx)
		return NullStr;
	beginidx = i + 1;

	first = event.argv[i];
	copy_tstring_after_char_(first, '=', c, len, TASKNAME_MAXLEN, ok);
	if (!ok)
		return NullStr;

	merge_args_into_cstring_nullterminate(event, beginidx, endidx, c, len,
					      TASKNAME_MAXLEN, ok);
	if (!ok)
		return NullStr;

	ts.len = len;
	retstr = pool->allocString(&ts, 0);
	if (retstr == nullptr)
		return NullStr;

	return retstr->ptr;
}

const char *perf_sched_process_fork_childname_strdup(const TraceEvent &event,
						     StringPool<> *pool);

/* Normally should be >= 3 but we don't care if the prio argument is missing */
#define perf_sched_process_exit_args_ok(EVENT) (EVENT.argc >= 2)
#define perf_sched_process_exit_pid(EVENT) \
	(int_after_pfix(EVENT, EVENT.argc - 2, EXIT_PID_PFIX))

/*
 * As a first approximation we assume that waking events and wakeup can be
 * parsed by the same code, through all kernel version where traceshark is
 * supposed to work.
 */
#define perf_sched_waking_args_ok(EVENT) (perf_sched_wakeup_args_ok(EVENT))
#define perf_sched_waking_cpu(EVENT) (perf_sched_wakeup_cpu(EVENT))
#define perf_sched_waking_prio(EVENT) (perf_sched_wakeup_prio(EVENT))
#define perf_sched_waking_pid(EVENT) (perf_sched_wakeup_pid(EVENT))
#define perf_sched_waking_name_strdup(EVENT, POOL) \
	perf_sched_wakeup_name_strdup(EVENT, POOL)

#endif /* PERFPARAMS_H*/
