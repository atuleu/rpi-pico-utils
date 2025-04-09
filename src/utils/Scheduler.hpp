// SPDX-License_identifier:  LGPL-3.0-or-later

#pragma once

#include "utils/Queue.hpp"
#include <optional>
#include <pico/time.h>

#include <functional>
#include <memory>
#include <queue>
#include <type_traits>
#include <variant>

class Scheduler {
public:
	typedef std::function<std::optional<int64_t>(absolute_time_t)> Task;

	static void Work();

	void Schedule(uint8_t priority, int64_t period_us, Task &&task) {
		schedule(priority, period_us, std::forward<Task>(task));
	}

	void Schedule(
	    uint8_t priority, int64_t period_us, std::function<void()> &&task
	) {
		schedule(priority, period_us, [t = std::move(task)](absolute_time_t) {
			t();
			return std::nullopt;
		});
	}

	void After(uint8_t priority, int64_t period_us, Task &&task) {
		after(priority, period_us, std::forward<Task>(task));
	}

	void After(
	    uint8_t                          priority,
	    int64_t                          period_us,
	    typename std::function<void()> &&task
	) {
		after(priority, period_us, [t = std::move(task)](absolute_time_t) {
			t();
			return std::nullopt;
		});
	}

	static Scheduler &Get();
	static Scheduler &Core1();

	static void WorkLoop();

	static void InitWorkLoopOnCore1();

private:
	Scheduler(uint core_idx);

	struct TaskData {
		uint8_t         Priority;
		absolute_time_t Next;
		Scheduler::Task Task;
		int64_t         Period;
	};

	struct TaskComparator {
		bool operator()(const TaskData *a, const TaskData *b);
	};

	typedef std::
	    priority_queue<TaskData *, std::vector<TaskData *>, TaskComparator>
	        TaskQueue;

	void schedule(uint8_t priority, int64_t period_us, Task &&task);

	void after(uint8_t priority, absolute_time_t at, Task &&task);

	void addTask(TaskData *ptr);

	void work();

	static Scheduler s_schedulers[2];

	uint                         d_coreIdx;
	TaskQueue                    d_tasks;
	Queue<TaskData *, 16, false> d_incoming;
};
