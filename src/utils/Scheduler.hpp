// SPDX-License_identifier:  LGPL-3.0-or-later

#pragma once

#include "utils/Queue.hpp"
#include "utils/RingBuffer.hpp"
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <pico/time.h>
#include <pico/types.h>
#include <queue>

static constexpr uint8_t         SCHEDULER_DEFAULT_PRIORITY = 100;
static constexpr uint8_t         SCHEDULER_HIGH_PRIORITY    = 50;
static constexpr uint8_t         SCHEDULER_LOW_PRIORITY     = 200;
static constexpr absolute_time_t SCHEDULER_START_NOW = 0xffffffffffffffff;

struct SchedulerOptions {
	uint8_t         Priority = SCHEDULER_DEFAULT_PRIORITY;
	absolute_time_t Start    = SCHEDULER_START_NOW;
	const char     *Name     = "";
};

class Scheduler {
public:
	typedef std::function<std::optional<int64_t>(absolute_time_t)> Task;

	static void Work();

	void Schedule(
	    int64_t period_us, Task &&task, const SchedulerOptions &options = {}
	) {
		schedule(period_us, std::forward<Task>(task), options);
	}

	void Schedule(
	    int64_t                 period_us,
	    std::function<void()> &&task,
	    const SchedulerOptions &options = {}
	) {
		schedule(
		    period_us,
		    [t = std::move(task)](absolute_time_t) {
			    t();
			    return std::nullopt;
		    },
		    options
		);
	}

	void After(
	    absolute_time_t at, Task &&task, const SchedulerOptions &options = {}
	) {
		after(at, std::forward<Task>(task), options);
	}

	void After(
	    absolute_time_t                  at,
	    typename std::function<void()> &&task,
	    const SchedulerOptions          &options = {}
	) {
		after(
		    at,
		    [t = std::move(task)](absolute_time_t) {
			    t();
			    return std::nullopt;
		    },
		    options
		);
	}

	static Scheduler &Get();

	static void WorkLoop();

	static void InitWorkLoopOnCore1(std::function<void()> &&core1Init);

private:
	Scheduler(uint core_idx);

	struct TaskData {
		uint8_t         Priority;
		absolute_time_t Next;
		Scheduler::Task Task;
		int64_t         Period;
#ifndef NDEBUG
		const char *Name = "";
#endif
	};

	class TaskQueue {
	public:
		TaskData       *pop();
		const TaskData *top() const;
		void            push(TaskData *);
		size_t          size() const;

#ifndef NDEBUG
		void debugPrint() const;
#endif
	private:
		std::vector<TaskData *> d_tasks;
	};

	static bool
	compareTask(const TaskData *a, const TaskData *b);

	void
	schedule(int64_t period_us, Task &&task, const SchedulerOptions &options);

	void
	after(absolute_time_t at, Task &&task, const SchedulerOptions &options);

	void addTask(TaskData *ptr);

	void work();

	static Scheduler s_schedulers[2];

	uint      d_coreIdx;
	TaskQueue d_tasks;
};
