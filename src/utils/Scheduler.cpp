// SPDX-License_identifier:  LGPL-3.0-or-later

#include "Scheduler.hpp"

#include <hardware/sync/spin_lock.h>
#include <pico/lock_core.h>
#include <pico/multicore.h>
#include <pico/time.h>
#include <pico/types.h>

#include <utils/internal/debugf.hpp>

Scheduler::TaskQueue Scheduler::d_tasks[2];

void Scheduler::Work() {
	auto	             &tasks = d_tasks[get_core_num()];
	std::vector<TaskData> renewed;
	renewed.reserve(tasks.size());
	while (true) {
		auto now = get_absolute_time();
		if (absolute_time_diff_us(now, tasks.top().Next) > 0) {
			break;
		}

		auto task = std::move(tasks.top());
		tasks.pop();

		auto newPeriod = task.Task(now);
		if (newPeriod.has_value()) {
			task.Period = newPeriod.value();
		}

		if (task.Period < 0) {
			// one shot task, simply ignore it
			continue;
		}

		task.Next = task.Period + task.Next;
		if (absolute_time_diff_us(now, task.Next) < 0 && task.Period > 0) {
			debugf("[scheduler] task overflow");
			task.Next = now;
		}

		renewed.emplace_back(std::move(task));
	}
	for (auto &t : renewed) {
		tasks.emplace(std::move(t));
	}
}

bool Scheduler::TaskComparator::operator()(
    const TaskData &a, const TaskData &b
) {
	if (a.Next == b.Next) {
		return a.Priority < b.Priority;
	}

	return a.Next > b.Next;
}

void Scheduler::schedule(
    uint8_t priority, int64_t period_us, Task &&task, bool otherCore
) {
	uint core = get_core_num();
	if (otherCore == true) {
		core = 1 - core;
	}

	d_tasks[core].emplace(TaskData{
	    .Priority = priority,
	    .Next     = get_absolute_time(),
	    .Task     = std::move(task),
	    .Period   = period_us,
	});
}

void Scheduler::after(
    uint8_t priority, absolute_time_t timeout, Task &&task, bool otherCore
) {
	uint core = get_core_num();
	if (otherCore == true) {
		core = 1 - core;
	}

	d_tasks[core].emplace(TaskData{
	    .Priority = priority,
	    .Next     = timeout,
	    .Task     = std::move(task),
	    .Period   = -1,
	});
}

void Scheduler::WorkLoop() {
	multicore_lockout_victim_init();
	while (true) {
		Work();
	}
}

void Scheduler::InitWorkLoopOnSecondCore() {
	if (multicore_lockout_victim_is_initialized(1) == true) {
		return;
	}
	multicore_launch_core1(Scheduler::WorkLoop);
	while (multicore_lockout_victim_is_initialized(1) == false) {
		tight_loop_contents();
	}
}
