// SPDX-License_identifier:  LGPL-3.0-or-later

#pragma once

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

	static void Schedule(
	    uint8_t priority, int64_t period_us, Task &&task, bool otherCore = false
	) {
		schedule(priority, period_us, std::forward<Task>(task), otherCore);
	}

	static void Schedule(
	    uint8_t                 priority,
	    int64_t                 period_us,
	    std::function<void()> &&task,
	    bool                    otherCore = false
	) {
		schedule(
		    priority,
		    period_us,
		    [t = std::move(task)](absolute_time_t) {
			    t();
			    return std::nullopt;
		    },
		    otherCore
		);
	}

	static void After(
	    uint8_t priority, int64_t period_us, Task &&task, bool otherCore = false
	) {
		after(priority, period_us, std::forward<Task>(task), otherCore);
	}

	static void After(
	    uint8_t                          priority,
	    int64_t                          period_us,
	    typename std::function<void()> &&task,
	    bool                             otherCore = false
	) {
		after(
		    priority,
		    period_us,
		    [t = std::move(task)](absolute_time_t) {
			    t();
			    return std::nullopt;
		    },
		    otherCore
		);
	}

	static void WorkLoop();

	static void InitWorkLoopOnSecondCore();

private:
	struct TaskData {
		uint8_t         Priority;
		absolute_time_t Next;
		Scheduler::Task Task;
		int64_t         Period;
	};

	struct TaskComparator {
		bool operator()(const TaskData &a, const TaskData &b);
	};

	typedef std::priority_queue<TaskData, std::vector<TaskData>, TaskComparator>
	    TaskQueue;

	static void
	schedule(uint8_t priority, int64_t period_us, Task &&task, bool otherCore);

	static void
	after(uint8_t priority, absolute_time_t at, Task &&task, bool otherCore);

	static TaskQueue d_tasks[2];
};
