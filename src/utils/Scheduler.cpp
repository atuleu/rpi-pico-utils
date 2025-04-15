// SPDX-License_identifier:  LGPL-3.0-or-later

#include "Scheduler.hpp"
#include "utils/Duration.hpp"

#include <hardware/sync/spin_lock.h>
#include <pico/lock_core.h>
#include <pico/multicore.h>
#include <pico/platform/panic.h>
#include <pico/time.h>
#include <pico/types.h>

#include <utils/Defer.hpp>
#include <utils/internal/debugf.hpp>

Scheduler::Scheduler(uint idx)
    : d_coreIdx{idx} {}

void Scheduler::Work() {
	Get().work();
}

void Scheduler::work() {

	while (d_incoming.empty() == false) {
		TaskData *ptr;
		d_incoming.pop(ptr);
		d_tasks.push(ptr);
	}

	std::vector<TaskData *> renewed;
	renewed.reserve(d_tasks.size());
	while (true) {
		auto now = get_absolute_time();
		if (absolute_time_diff_us(now, d_tasks.top()->Next) > 0) {
			break;
		}
		auto task = d_tasks.top();
		d_tasks.pop();

		auto newPeriod = task->Task(now);
		if (newPeriod.has_value()) {
			task->Period = newPeriod.value();
		}

		if (task->Period < 0) {
			delete task;
			// one shot task, simply do not put it back
			continue;
		}

		task->Next += task->Period;
		if (task->Period > 0) {
			while (absolute_time_diff_us(now, task->Next) < 0) {
				debugf("[scheduler/%d] task overflow", d_coreIdx);
				task->Next += task->Period;
			}
		}

		renewed.push_back(task);
	}

	for (const auto &t : renewed) {
		d_tasks.push(t);
	}
}

bool Scheduler::TaskComparator::operator()(
    const TaskData *a, const TaskData *b
) {
	if (a->Next == b->Next) {
		return a->Priority < b->Priority;
	}

	return a->Next > b->Next;
}

void Scheduler::schedule(
    int64_t period_us, Task &&task, const SchedulerOptions &options
) {
	addTask(new TaskData{
	    .Priority = options.Priority,
	    .Next     = options.Start == SCHEDULER_START_NOW ? get_absolute_time()
	                                                     : options.Start,
	    .Task     = std::move(task),
	    .Period   = period_us,
	});
}

void Scheduler::after(
    absolute_time_t timeout, Task &&task, const SchedulerOptions &options
) {
	addTask(new TaskData{
	    .Priority = options.Priority,
	    .Next     = timeout,
	    .Task     = std::move(task),
	    .Period   = -1,
	});
}

void Scheduler::addTask(TaskData *ptr) {

	if (d_incoming.full() == true) {
		panic("[scheduler/%d]: incoming task overflow", d_coreIdx);
	}
	d_incoming.insert(ptr);

	debugf("[scheduler/%d] scheduled a new task\n", d_coreIdx);
}

void Scheduler::WorkLoop() {
	// debugf("[scheduler/%d] scheduler loop init\n", get_core_num());
	multicore_lockout_victim_init();
	auto &self = Get();
	debugf("[scheduler/%d] scheduler loop rolling\n", self.d_coreIdx);

	while (true) {
		self.work();
	}
}

void Scheduler::InitWorkLoopOnCore1() {
	if (multicore_lockout_victim_is_initialized(1) == true) {
		// debugf("[scheduler/%d] Other core already started\n",
		// get_core_num());
		return;
	}
	multicore_launch_core1(Scheduler::WorkLoop);
	while (multicore_lockout_victim_is_initialized(1) == false) {
		tight_loop_contents();
	}
	// debugf("[scheduler/%d] Other core started and ready\n", get_core_num());
}

Scheduler Scheduler::s_schedulers[2] = {Scheduler(0), Scheduler(1)};

Scheduler &Scheduler::Get() {
	return s_schedulers[get_core_num()];
}

Scheduler &Scheduler::Core1() {
	// if (get_core_num() == 1) {
	// 	panic("you can only schedule from core 0, or from itself");
	// }
	return s_schedulers[1];
}
