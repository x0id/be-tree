
#include <stdio.h>
#include "tree.h"
#include "minunit.h"

int test_one()
{
    struct betree* tree = betree_make();

    betree_add_boolean_variable(tree, "stop", false);

    betree_insert(tree, 1, "stop");

    struct betree_event* event = betree_make_event(tree);
    betree_set_variable(event, 0, betree_make_boolean_variable("stop", false));

    struct report *report = make_report();

    betree_search_with_event(tree, event, report);
    fprintf(stderr, "evaluated: %lu\n", report->evaluated);
    fprintf(stderr, "matched: %lu\n", report->matched);
    fprintf(stderr, "memoized: %lu\n", report->memoized);
    fprintf(stderr, "shorted: %lu\n", report->shorted);
    for (size_t i=0; i<report->matched; i++)
        fprintf(stderr, "id: %lu\n", report->subs[i]);

    free_report(report);
    betree_free_event(event);
    betree_free(tree);

    return 0;
}

int all_tests()
{
    mu_run_test(test_one);
    return 0;
}

RUN_TESTS()
