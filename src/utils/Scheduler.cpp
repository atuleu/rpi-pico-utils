// SPDX-License_identifier:  LGPL-3.0-or-later

#include "Scheduler.hpp"
#include "utils/Duration.hpp"

#include <algorithm>
#include <pico/multicore.h>
#include <pico/platform/panic.h>
#include <pico/time.h>
#include <pico/types.h>

#include <queue>
#include <utils/Defer.hpp>
#include <utils/internal/debugf.hpp>

Scheduler::Scheduler(uint idx)
    : d_coreIdx{idx} {}

void Scheduler::Work() {
	Get().work();
}

#ifndef NDEBUG
void Scheduler::TaskQueue::debugPrint() const {
	printf(
	    "\n----------------------------------------------------------------\n"
	);
	for (const auto &task : d_tasks) {
		int sec  = task->Next / 1000000;
		int usec = task->Next % 1000000;
		int msec = usec / 10000;

		printf(
		    "{Name: %s, Next: %03d.%02d, Priority:%d}",
		    task->Name,
		    sec,
		    msec,
		    task->Priority
		);
	}
	printf(
	    "\n--------------------------------------------------------------------"
	    "------------\n"
	);
}

#endif

bool Scheduler::compareTask(const TaskData *a, const Scheduler::TaskData *b) {
	if (a->Next == b->Next) {
		return a->Priority > b->Priority;
	}
	return a->Next > b->Next;
}

void Scheduler::TaskQueue::push(TaskData *ptr) {
	d_tasks.push_back(ptr);
	std::push_heap(d_tasks.begin(), d_tasks.end(), Scheduler::compareTask);
}

size_t Scheduler::TaskQueue::size() const {
	return d_tasks.size();
}

Scheduler::TaskData *Scheduler::TaskQueue::pop() {
	std::pop_heap(d_tasks.begin(), d_tasks.end(), Scheduler::compareTask);
	auto res = d_tasks.back();
	d_tasks.pop_back();
	return res;
}

const Scheduler::TaskData *Scheduler::TaskQueue::top() const {
	return d_tasks.front();
}

void Scheduler::work() {

	std::vector<TaskData *> renewed;
	renewed.reserve(d_tasks.size());

	while (d_tasks.size() > 0) {
		auto now = get_absolute_time();
		if (absolute_time_diff_us(now, d_tasks.top()->Next) > 0) {
			break;
		}
		auto task = d_tasks.pop();

		debugf("[scheduler/%d] executing task '%s'\n", d_coreIdx, task->Name);
		auto newPeriod = task->Task(now);
		if (newPeriod.has_value()) {
			task->Period = newPeriod.value();
		}

		if (task->Period < 0) {
			debugf("[scheduler/%d] task '%s' done\n", d_coreIdx, task->Name);
			delete task;

			// one shot task, simply do not put it back
			continue;
		}

		task->Next += task->Period;
		auto nextIn = absolute_time_diff_us(now, task->Next);
		if (task->Period > 0 && nextIn < 0) {
			uint nbOverflow = std::abs(nextIn) / task->Period + 1;
			debugf(
			    "[scheduler/%d] task '%s' has overflow %d time(s)\n",
			    d_coreIdx,
			    task->Name,
			    nbOverflow
			);
			task->Next += nbOverflow * task->Period;
		}

		renewed.push_back(task);
	}

	for (const auto &t : renewed) {
		debugf("[scheduler/%d] rescheduling task '%s'\n", d_coreIdx, t->Name);
		d_tasks.push(t);
	}
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
#ifndef NDEBUG
	    .Name = options.Name,
#endif
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
#ifndef NDEBUG
	    .Name = options.Name,
#endif
	});
}

void Scheduler::addTask(TaskData *ptr) {
	d_tasks.push(ptr);

	debugf("[scheduler/%d] scheduled a new task '%s'\n", d_coreIdx, ptr->Name);
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

void Scheduler::InitWorkLoopOnCore1(std::function<void()> &&initFunction) {
	if (multicore_lockout_victim_is_initialized(1) == true) {
		// debugf("[scheduler/%d] Other core already started\n",
		// get_core_num());
		return;
	}
	s_schedulers[1].After(0, std::move(initFunction), {.Name = "core1/init"});

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
