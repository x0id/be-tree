
#include <stdio.h>
#include "tree.h"
#include "minunit.h"

static int test_one()
{
    struct betree* tree = betree_make();

    betree_add_boolean_variable(tree, "stop", false);

    betree_insert(tree, 1, "stop");

    struct report *report = make_report();
    struct betree_event* event = betree_make_event(tree);
    betree_set_variable(event, 0, betree_make_boolean_variable("stop", false));
    betree_search_with_event(tree, event, report);

    mu_assert(report->matched == 0, "");

    free_report(report);
    betree_free_event(event);

    report = make_report();
    event = betree_make_event(tree);
    betree_set_variable(event, 0, betree_make_boolean_variable("stop", true));
    betree_search_with_event(tree, event, report);

    mu_assert(report->matched == 1, "");
    mu_assert(report->subs[0] == 1, "");

    free_report(report);
    betree_free_event(event);
    betree_free(tree);

    return 0;
}

static void *last_arg;
static void *last_data;
static bool last_result;
static const void *last_context;

static void hook(void *arg, void *data, bool result, const void *context) {
    last_arg = arg;
    last_data = data;
    last_result = result;
    last_context = context;
}

static int test_two()
{
    struct betree* tree = betree_make();

    betree_add_boolean_variable(tree, "stop", false);

    struct betree_sub* sub = betree_make_sub(tree, 1, 0, NULL, "stop");
    mu_assert(sub != NULL, "");
    sub->data = (void *)555;
    mu_assert(betree_insert_sub(tree, sub), "");

    struct report *report = make_report();
    report->cb = hook;
    report->arg = (void *)123;

    struct betree_event* event = betree_make_event(tree);
    struct betree_variable *var = betree_make_boolean_variable("stop", false);
    betree_set_variable(event, 0, var);
    betree_search_with_event(tree, event, report);

    mu_assert(report->matched == 0, "");
    mu_assert(last_arg == (void *)123, "");
    mu_assert(last_data == (void *)555, "");
    mu_assert(last_result == false, "");
    mu_assert(!strcmp((char *)last_context, "stop"), "");

    free_report(report);
    betree_free_event(event);

    report = make_report();
    report->cb = hook;
    report->arg = (void *)456;

    event = betree_make_event(tree);
    var = betree_make_boolean_variable("stop", true);
    betree_set_variable(event, 0, var);
    betree_search_with_event(tree, event, report);

    mu_assert(report->matched == 0, "");
    mu_assert(last_arg == (void *)456, "");
    mu_assert(last_data == (void *)555, "");
    mu_assert(last_result == true, "");
    mu_assert(!strcmp((char *)last_context, "stop"), "");

    free_report(report);
    betree_free_event(event);
    betree_free(tree);

    return 0;
}

static int all_tests()
{
    mu_run_test(test_one);
    mu_run_test(test_two);
    return 0;
}

RUN_TESTS()
