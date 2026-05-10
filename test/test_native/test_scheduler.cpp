// Unity unit tests for taskscheduler::Task and taskscheduler::Scheduler.
// Runs on PlatformIO's native platform (host); no MCU required.
#include <unity.h>
#include "CriticalTaskScheduler.h"

using namespace taskscheduler;

// ---- Fake clock ---------------------------------------------------------
static unsigned long g_fakeNow = 0;
static unsigned long fakeMillis() { return g_fakeNow; }
extern "C" unsigned long millis() { return g_fakeNow; }

// ---- Counters -----------------------------------------------------------
static int counterA = 0;
static int counterB = 0;
static int counterCritical = 0;
static void taskA() { counterA++; }
static void taskB() { counterB++; }
static void taskCritical() { counterCritical++; }

// ---- Setup / tear-down --------------------------------------------------
void setUp(void)
{
    counterA = counterB = counterCritical = 0;
    g_fakeNow = 0;
}
void tearDown(void) {}

// ---- Task lifecycle -----------------------------------------------------

static void test_task_starts_disabled()
{
    Task t("t", 100, taskA);
    TEST_ASSERT_FALSE(t.isEnabled());
    TEST_ASSERT_FALSE(t.shouldRun(0));
}

static void test_task_enable_schedules_immediate()
{
    g_fakeNow = 500;
    Task t("t", 100, taskA);
    t.enable();
    TEST_ASSERT_TRUE(t.isEnabled());
    TEST_ASSERT_TRUE(t.shouldRun(500));
}

static void test_task_disable_blocks_shouldRun()
{
    g_fakeNow = 500;
    Task t("t", 100, taskA);
    t.enable();
    t.disable();
    TEST_ASSERT_FALSE(t.isEnabled());
    TEST_ASSERT_FALSE(t.shouldRun(1000));
}

static void test_task_enableDelayed_postpones_first_run()
{
    g_fakeNow = 1000;
    Task t("t", 100, taskA);
    t.enableDelayed(250);
    TEST_ASSERT_FALSE(t.shouldRun(1200));
    TEST_ASSERT_TRUE(t.shouldRun(1250));
}

static void test_task_setPeriod_getPeriod()
{
    Task t("t", 100, taskA);
    TEST_ASSERT_EQUAL(100, t.getPeriod());
    t.setPeriod(250);
    TEST_ASSERT_EQUAL(250, t.getPeriod());
}

static void test_task_critical_flag()
{
    Task crit("crit", 100, taskCritical, true);
    Task bg("bg", 100, taskA, false);
    TEST_ASSERT_TRUE(crit.isCritical());
    TEST_ASSERT_FALSE(bg.isCritical());
}

// ---- Task::run stats ----------------------------------------------------

static void test_run_invokes_callback_and_updates_stats()
{
    Task t("t", 100, taskA);
    t.enable();
    g_fakeNow = 0;
    // run() calls now() once for start and once for end. Move time between.
    // We simulate this by calling run() and bumping g_fakeNow inside the
    // callback isn't possible (no closure). Instead drive via two runs.
    t.run(0);
    TEST_ASSERT_EQUAL_INT(1, counterA);
    TEST_ASSERT_EQUAL_UINT32(1, t.stats().runs);
}

static void test_run_tracks_max_execution_time()
{
    // We can't easily mock the elapsed-time-per-run without a closure-aware
    // mock; assert maxExecutionTime is monotonically non-decreasing.
    Task t("t", 100, taskA);
    t.enable();
    t.run(0);
    auto s1 = t.stats();
    t.run(10);
    auto s2 = t.stats();
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(s1.maxExecutionTime, s2.maxExecutionTime);
}

static void test_run_reschedules_non_critical_from_endTime()
{
    Task t("t", 100, taskA, false);
    t.enable();
    g_fakeNow = 5; // both start and end millis() return 5 -> exec=0
    t.run(0);
    // background: nextRunTime = endTime + period = 5 + 100 = 105
    TEST_ASSERT_EQUAL_UINT32(105, t.stats().nextRunTime);
}

static void test_run_reschedules_critical_from_schedulerTime()
{
    Task t("c", 100, taskCritical, true);
    t.enable();
    g_fakeNow = 50;
    t.run(20);
    // critical: nextRunTime = schedulerTime + period = 20 + 100 = 120
    TEST_ASSERT_EQUAL_UINT32(120, t.stats().nextRunTime);
}

static void test_run_does_nothing_with_null_callback()
{
    Task t("t", 100, nullptr);
    t.enable();
    t.run(0);
    TEST_ASSERT_EQUAL_UINT32(0, t.stats().runs);
}

// ---- Scheduler ----------------------------------------------------------

static void test_scheduler_execute_runs_only_earliest_due_task()
{
    g_fakeNow = 50;
    Scheduler sched;
    sched.setTimeProvider(&fakeMillis);
    Task t1("a", 100, taskA);
    Task t2("b", 200, taskB);
    sched.addTask(&t1);
    sched.addTask(&t2);
    sched.enableAll();
    sched.execute();
    TEST_ASSERT_EQUAL_INT(1, counterA + counterB);
}

static void test_scheduler_execute_skips_disabled_tasks()
{
    g_fakeNow = 50;
    Scheduler sched;
    sched.setTimeProvider(&fakeMillis);
    Task t1("a", 100, taskA);
    sched.addTask(&t1); // not enabled
    sched.execute();
    TEST_ASSERT_EQUAL_INT(0, counterA);
}

static void test_scheduler_executeCritical_runs_all_due()
{
    g_fakeNow = 50;
    Scheduler sched;
    sched.setTimeProvider(&fakeMillis);
    Task t1("c1", 100, taskCritical, true);
    Task t2("c2", 100, taskCritical, true);
    sched.addTask(&t1);
    sched.addTask(&t2);
    sched.enableAll();
    sched.executeCritical();
    TEST_ASSERT_EQUAL_INT(2, counterCritical);
}

static void test_scheduler_disableAll_blocks_execution()
{
    g_fakeNow = 50;
    Scheduler sched;
    sched.setTimeProvider(&fakeMillis);
    Task t1("a", 100, taskA);
    Task t2("c", 100, taskCritical, true);
    sched.addTask(&t1);
    sched.addTask(&t2);
    sched.enableAll();
    sched.disableAll();
    sched.execute();
    sched.executeCritical();
    TEST_ASSERT_EQUAL_INT(0, counterA);
    TEST_ASSERT_EQUAL_INT(0, counterCritical);
}

static void test_scheduler_addTask_separates_critical_from_background()
{
    g_fakeNow = 50;
    Scheduler sched;
    sched.setTimeProvider(&fakeMillis);
    Task bg("bg", 100, taskA, false);
    Task crit("crit", 100, taskCritical, true);
    sched.addTask(&bg);
    sched.addTask(&crit);
    TEST_ASSERT_EQUAL_UINT32(1, sched.taskCount());
    TEST_ASSERT_EQUAL_UINT32(1, sched.criticalTaskCount());
    sched.enableAll();
    sched.execute();
    TEST_ASSERT_EQUAL_INT(1, counterA);
    TEST_ASSERT_EQUAL_INT(0, counterCritical);
    counterA = 0;
    sched.executeCritical();
    TEST_ASSERT_EQUAL_INT(0, counterA);
    TEST_ASSERT_EQUAL_INT(1, counterCritical);
}

static void test_scheduler_removeTask()
{
    Scheduler sched;
    Task t1("a", 100, taskA);
    Task t2("b", 100, taskB);
    sched.addTask(&t1);
    sched.addTask(&t2);
    TEST_ASSERT_TRUE(sched.removeTask(&t1));
    TEST_ASSERT_EQUAL_UINT32(1, sched.taskCount());
    TEST_ASSERT_EQUAL_PTR(&t2, sched.taskAt(0));
    TEST_ASSERT_FALSE(sched.removeTask(&t1)); // already gone
}

static void test_scheduler_addTask_full_returns_false()
{
    Scheduler sched;
    Task *all[CRITICALTASKSCHEDULER_MAX_TASKS];
    for (int i = 0; i < CRITICALTASKSCHEDULER_MAX_TASKS; ++i)
    {
        all[i] = new Task("t", 100, taskA);
        TEST_ASSERT_TRUE(sched.addTask(all[i]));
    }
    Task overflow("of", 100, taskA);
    TEST_ASSERT_FALSE(sched.addTask(&overflow));
    for (int i = 0; i < CRITICALTASKSCHEDULER_MAX_TASKS; ++i) delete all[i];
}

// ---- Time provider injection -------------------------------------------

static void test_setTimeProvider_propagates_to_tasks()
{
    Scheduler sched;
    sched.setTimeProvider(&fakeMillis);
    Task t("t", 100, taskA);
    sched.addTask(&t);

    g_fakeNow = 1000;
    t.enable();
    TEST_ASSERT_TRUE(t.shouldRun(1000));

    g_fakeNow = 2000;
    t.enableDelayed(50);
    TEST_ASSERT_FALSE(t.shouldRun(2049));
    TEST_ASSERT_TRUE(t.shouldRun(2050));
}

// ---- Rollover safety ---------------------------------------------------

static void test_shouldRun_handles_millis_rollover()
{
    // Simulate a task scheduled near the end of unsigned long range.
    Task t("t", 100, taskA);
    t.enable();
    g_fakeNow = 0xFFFFFFA0UL; // ~96 ms before wrap
    t.enableDelayed(200);     // _nextRunTime = 0xFFFFFFA0 + 200 = wraps to 0x00000068

    // Before due time (still 50 ms before _nextRunTime), should NOT run, even
    // though after wrap "currentTime < _nextRunTime" naively would be tricky.
    g_fakeNow = 0xFFFFFFA0UL + 150; // wraps: 0x00000038, 50 ms before due
    TEST_ASSERT_FALSE(t.shouldRun(g_fakeNow));

    // At/after due time, should run.
    g_fakeNow = 0xFFFFFFA0UL + 200; // wraps: 0x00000068, exactly due
    TEST_ASSERT_TRUE(t.shouldRun(g_fakeNow));

    g_fakeNow = 0xFFFFFFA0UL + 250; // wraps: 0x000000B8, 50 ms late
    TEST_ASSERT_TRUE(t.shouldRun(g_fakeNow));
}

static void test_scheduler_execute_picks_latest_due_across_rollover()
{
    // Two background tasks: t1 due BEFORE the wrap, t2 due AFTER the wrap.
    // The scheduler must pick t1 (most overdue) rather than naively comparing
    // raw _nextRunTime (which would prefer t2 because its value is small).
    Scheduler sched;
    sched.setTimeProvider(&fakeMillis);
    Task t1("a", 100, taskA);
    Task t2("b", 100, taskB);
    sched.addTask(&t1);
    sched.addTask(&t2);

    g_fakeNow = 0xFFFFFF00UL;
    t1.enable();                // _nextRunTime = 0xFFFFFF00 (most overdue eventually)
    g_fakeNow = 0xFFFFFFFEUL;
    t2.enable();                // _nextRunTime = 0xFFFFFFFE (less overdue)

    // Advance past wrap. Now both are due, t1 is older.
    g_fakeNow = 0x00000010UL;   // wrapped
    sched.execute();
    TEST_ASSERT_EQUAL_INT(1, counterA);
    TEST_ASSERT_EQUAL_INT(0, counterB);
}

// ---- Duplicate registration --------------------------------------------

static void test_addTask_rejects_duplicates()
{
    Scheduler sched;
    Task t("t", 100, taskA);
    TEST_ASSERT_TRUE(sched.addTask(&t));
    TEST_ASSERT_FALSE(sched.addTask(&t));
    TEST_ASSERT_EQUAL_UINT32(1, sched.taskCount());
}

static void test_addTask_rejects_duplicate_critical()
{
    Scheduler sched;
    Task c("c", 100, taskCritical, true);
    TEST_ASSERT_TRUE(sched.addTask(&c));
    TEST_ASSERT_FALSE(sched.addTask(&c));
    TEST_ASSERT_EQUAL_UINT32(1, sched.criticalTaskCount());
}

// ---- main --------------------------------------------------------------

int main(int, char **)
{
    UNITY_BEGIN();

    RUN_TEST(test_task_starts_disabled);
    RUN_TEST(test_task_enable_schedules_immediate);
    RUN_TEST(test_task_disable_blocks_shouldRun);
    RUN_TEST(test_task_enableDelayed_postpones_first_run);
    RUN_TEST(test_task_setPeriod_getPeriod);
    RUN_TEST(test_task_critical_flag);

    RUN_TEST(test_run_invokes_callback_and_updates_stats);
    RUN_TEST(test_run_tracks_max_execution_time);
    RUN_TEST(test_run_reschedules_non_critical_from_endTime);
    RUN_TEST(test_run_reschedules_critical_from_schedulerTime);
    RUN_TEST(test_run_does_nothing_with_null_callback);

    RUN_TEST(test_scheduler_execute_runs_only_earliest_due_task);
    RUN_TEST(test_scheduler_execute_skips_disabled_tasks);
    RUN_TEST(test_scheduler_executeCritical_runs_all_due);
    RUN_TEST(test_scheduler_disableAll_blocks_execution);
    RUN_TEST(test_scheduler_addTask_separates_critical_from_background);
    RUN_TEST(test_scheduler_removeTask);
    RUN_TEST(test_scheduler_addTask_full_returns_false);

    RUN_TEST(test_setTimeProvider_propagates_to_tasks);

    RUN_TEST(test_shouldRun_handles_millis_rollover);
    RUN_TEST(test_scheduler_execute_picks_latest_due_across_rollover);
    RUN_TEST(test_addTask_rejects_duplicates);
    RUN_TEST(test_addTask_rejects_duplicate_critical);

    return UNITY_END();
}
