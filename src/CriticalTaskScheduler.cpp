// CriticalTaskScheduler - Implementation.
// SPDX-License-Identifier: MIT
#include "CriticalTaskScheduler.h"

namespace taskscheduler {

unsigned long defaultTimeProvider()
{
    return millis();
}

// ---------------- Task ----------------

Task::Task(const char *name, unsigned long periodMs, TaskCallback cb, bool critical)
    : _name(name ? name : ""),
      _period(periodMs),
      _callback(cb),
      _critical(critical),
      _enabled(false),
      _nextRunTime(0),
      _lastExecutionTime(0),
      _maxExecutionTime(0),
      _totalTime(0),
      _runs(0),
      _timeProvider(nullptr)
{
}

void Task::enable()
{
    _nextRunTime = now();
    _enabled = true;
}

void Task::disable()
{
    _enabled = false;
}

void Task::enableDelayed(unsigned long delayMs)
{
    _nextRunTime = now() + delayMs;
    _enabled = true;
}

bool Task::shouldRun(unsigned long currentTime) const
{
    return _enabled && (currentTime >= _nextRunTime);
}

void Task::run(unsigned long schedulerTime)
{
    if (!_callback)
    {
        return;
    }

    const unsigned long startTime = now();
    _callback();
    const unsigned long endTime = now();

    if (_critical)
    {
        _nextRunTime = schedulerTime + _period;
    }
    else
    {
        _nextRunTime = endTime + _period;
    }

    _lastExecutionTime = endTime - startTime;
    if (_lastExecutionTime > _maxExecutionTime)
    {
        _maxExecutionTime = _lastExecutionTime;
    }
    _totalTime += _lastExecutionTime;
    _runs++;
}

TaskStats Task::stats() const
{
    TaskStats s;
    s.name = _name;
    s.critical = _critical;
    s.enabled = _enabled;
    s.period = _period;
    s.runs = _runs;
    s.lastExecutionTime = _lastExecutionTime;
    s.maxExecutionTime = _maxExecutionTime;
    s.totalTime = _totalTime;
    s.avgExecutionTime = _runs ? (static_cast<float>(_totalTime) / static_cast<float>(_runs)) : 0.0f;
    s.nextRunTime = _nextRunTime;
    return s;
}

// ---------------- Scheduler ----------------

Scheduler::Scheduler()
    : _count(0), _criticalCount(0), _now(&defaultTimeProvider)
{
    for (size_t i = 0; i < TASKSCHEDULER_MAX_TASKS; ++i)
    {
        _tasks[i] = nullptr;
        _criticalTasks[i] = nullptr;
    }
}

bool Scheduler::addTask(Task *task)
{
    if (!task)
    {
        return false;
    }

    task->useTimeProvider(_now);

    if (task->isCritical())
    {
        if (_criticalCount >= TASKSCHEDULER_MAX_TASKS)
        {
            return false;
        }
        _criticalTasks[_criticalCount++] = task;
    }
    else
    {
        if (_count >= TASKSCHEDULER_MAX_TASKS)
        {
            return false;
        }
        _tasks[_count++] = task;
    }
    return true;
}

bool Scheduler::removeTask(Task *task)
{
    if (!task)
    {
        return false;
    }

    Task **bucket = task->isCritical() ? _criticalTasks : _tasks;
    size_t &count = task->isCritical() ? _criticalCount : _count;

    for (size_t i = 0; i < count; ++i)
    {
        if (bucket[i] == task)
        {
            for (size_t j = i + 1; j < count; ++j)
            {
                bucket[j - 1] = bucket[j];
            }
            count--;
            bucket[count] = nullptr;
            return true;
        }
    }
    return false;
}

void Scheduler::execute()
{
    const unsigned long nowMs = _now();
    Task *taskToRun = nullptr;
    unsigned long minNextRunTime = static_cast<unsigned long>(-1);

    for (size_t i = 0; i < _count; ++i)
    {
        Task *t = _tasks[i];
        if (t->shouldRun(nowMs) && t->_nextRunTime < minNextRunTime)
        {
            taskToRun = t;
            minNextRunTime = t->_nextRunTime;
        }
    }

    if (taskToRun)
    {
        taskToRun->run(nowMs);
    }
}

void Scheduler::executeCritical()
{
    const unsigned long nowMs = _now();
    for (size_t i = 0; i < _criticalCount; ++i)
    {
        Task *t = _criticalTasks[i];
        if (t->shouldRun(nowMs))
        {
            t->run(nowMs);
        }
    }
}

void Scheduler::enableAll()
{
    for (size_t i = 0; i < _count; ++i)
    {
        _tasks[i]->enable();
    }
    for (size_t i = 0; i < _criticalCount; ++i)
    {
        _criticalTasks[i]->enable();
    }
}

void Scheduler::disableAll()
{
    for (size_t i = 0; i < _count; ++i)
    {
        _tasks[i]->disable();
    }
    for (size_t i = 0; i < _criticalCount; ++i)
    {
        _criticalTasks[i]->disable();
    }
}

static void printOne(Print &out, const Task *task)
{
    const TaskStats s = task->stats();
    out.print(s.critical ? "*" : " ");
    out.print(s.name);
    out.print(": dt=");
    out.print(s.lastExecutionTime);
    out.print(" avg=");
    out.print(s.avgExecutionTime, 2);
    out.print(" max=");
    out.print(s.maxExecutionTime);
    out.print(" sum=");
    out.print(s.totalTime);
    out.print(" runs=");
    out.print(s.runs);
    out.print(" period=");
    out.print(s.period);
    out.print(" enabled=");
    out.println(s.enabled ? 1 : 0);
}

void Scheduler::printStats(Print &out) const
{
    for (size_t i = 0; i < _criticalCount; ++i)
    {
        printOne(out, _criticalTasks[i]);
    }
    for (size_t i = 0; i < _count; ++i)
    {
        printOne(out, _tasks[i]);
    }
}

void Scheduler::setTimeProvider(TimeProvider tp)
{
    _now = tp ? tp : &defaultTimeProvider;
    for (size_t i = 0; i < _count; ++i)
    {
        _tasks[i]->useTimeProvider(_now);
    }
    for (size_t i = 0; i < _criticalCount; ++i)
    {
        _criticalTasks[i]->useTimeProvider(_now);
    }
}

// ---------------- FreeRTOSCriticalRunner ----------------

#if TASKSCHEDULER_HAS_FREERTOS

FreeRTOSCriticalRunner::FreeRTOSCriticalRunner(Scheduler &sched,
                                               uint32_t stackSize,
                                               UBaseType_t priority,
                                               uint32_t tickMs)
    : _sched(sched), _stackSize(stackSize), _priority(priority),
      _tickMs(tickMs ? tickMs : 1), _handle(nullptr)
{
}

void FreeRTOSCriticalRunner::runnerEntry(void *param)
{
    FreeRTOSCriticalRunner *self = static_cast<FreeRTOSCriticalRunner *>(param);
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t freq = pdMS_TO_TICKS(self->_tickMs);
    for (;;)
    {
        vTaskDelayUntil(&lastWakeTime, freq);
        self->_sched.executeCritical();
    }
}

bool FreeRTOSCriticalRunner::start()
{
    if (_handle != nullptr)
    {
        return true;
    }
    BaseType_t res = xTaskCreate(&FreeRTOSCriticalRunner::runnerEntry,
                                 "tsCritical",
                                 _stackSize,
                                 this,
                                 _priority,
                                 &_handle);
    if (res != pdPASS)
    {
        _handle = nullptr;
        return false;
    }
    return true;
}

void FreeRTOSCriticalRunner::stop()
{
    if (_handle != nullptr)
    {
        vTaskDelete(_handle);
        _handle = nullptr;
    }
}

uint32_t FreeRTOSCriticalRunner::getFreeStack() const
{
    if (!_handle)
    {
        return 0;
    }
    return static_cast<uint32_t>(uxTaskGetStackHighWaterMark(_handle));
}

#endif // TASKSCHEDULER_HAS_FREERTOS

} // namespace taskscheduler
