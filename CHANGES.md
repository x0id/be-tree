# v1.5.0 (2026-06-25)

## New Features

### 1. Per-Subscription Callback Hooks (`cb` / `cba`)

Enables streaming match results via callbacks instead of collecting matched IDs
into `report->subs`. Activated by setting fields on `struct report` before
calling the existing `betree_search_with_event()`.

#### Per-sub callback (`cb`)

```c
void my_cb(void* arg, void* data, bool result, const void* ctx);
```

Called for every subscription evaluated during search. Parameters:

| Parameter | Description |
|-----------|-------------|
| `arg`     | User context pointer (`report->arg`) |
| `data`    | Opaque per-sub payload (`sub->data`) |
| `result`  | Match result (`true`/`false`) |
| `ctx`     | `(void*)last_var` — variable index that determined the outcome |

When `report->cb` is set, `report->subs` is **not** populated; the caller
tracks matches in their hook.

#### Batch-exclude callback (`cba`)

```c
void my_cba(void* arg, void** data, size_t count, const void* ctx);
```

Called during tree traversal when an entire cdir partition is excluded (the
event doesn't fall into that domain range). Reports all subscriptions in
that subtree at once.

| Parameter | Description |
|-----------|-------------|
| `arg`     | User context pointer (`report->arg`) |
| `data`    | Array of `sub->data` pointers for excluded subs |
| `count`   | Number of elements in the array |
| `ctx`     | `(void*)var_id` — the partitioning variable |

**Requires** `betree_prepare_sub_data(tree)` to be called once after all
subscriptions are inserted.

#### Usage

```c
struct report* report = make_report();
report->cb  = my_cb;
report->cba = my_cba;
report->arg = my_context;

betree_search_with_event(tree, event, report);
```

---

### 2. `last_var` Tracking (Match Attribution)

Each subscription's match result is attributed to the last variable that
determined the outcome. This is tracked as `report->last_var` (type
`betree_var_t`) and passed to the `cb` hook as the `ctx` argument.

Sources of attribution:
- Direct predicate evaluation (compare, equality, bool, set, list, special exprs)
- Short-circuit pass/fail — reports the variable from the short-circuit bitmask
- Memoized results — replays the variable recorded when the predicate was first evaluated

Special values:
- `NIL_VAR` (`(betree_var_t)-1`) — no variable determined the result yet
- `GEO_VAR` (`(betree_var_t)-2`) — result determined by a geo-type expression

---

### 3. `sub->data` — Per-Subscription Opaque Payload

`betree_make_sub()` now returns a **non-const** `struct betree_sub*`, allowing
the caller to attach arbitrary data before insertion:

```c
struct betree_sub* sub = betree_make_sub(tree, id, 0, NULL, expr);
sub->data = my_pointer;   // defaults to (void*)id if not set
betree_insert_sub(tree, sub);
```

This `data` pointer is what gets delivered to `cb` and `cba` hooks.

---

### 4. Variable Ranking

Controls the order in which variables are used for tree partitioning. Higher
rank → the variable appears closer to the root of the partition tree.

New API functions (one per variable type):

```c
void betree_add_ranked_boolean_variable(tree, name, allow_undefined, rank);
void betree_add_ranked_integer_variable(tree, name, allow_undefined, min, max, rank);
void betree_add_ranked_float_variable(tree, name, allow_undefined, min, max, rank);
void betree_add_ranked_string_variable(tree, name, allow_undefined, count, rank);
void betree_add_ranked_integer_list_variable(tree, name, allow_undefined, min, max, rank);
void betree_add_ranked_integer_enum_variable(tree, name, allow_undefined, count, rank);
void betree_add_ranked_string_list_variable(tree, name, allow_undefined, count, rank);
void betree_add_ranked_segments_variable(tree, name, allow_undefined, rank);
void betree_add_ranked_frequency_caps_variable(tree, name, allow_undefined, rank);
```

The existing `betree_add_*_variable()` functions remain unchanged (rank = 0).

Score formula: `score = count * attr_domain_score + 10000 * rank`

---

### 5. Subs Data Preparation (`betree_prepare_sub_data`)

Pre-computes per-cdir arrays of `sub->data` pointers so the `cba` hook can
fire without walking individual nodes at search time.

```c
// Caller allocates subs_data before calling:
struct subs_data sd = { .array = my_array, .count = 0, .limit = total_subs };
tree->subs_data = &sd;
betree_prepare_sub_data(tree);
```

Must be called **once** after all subscriptions are inserted, **before** any
search that uses the `cba` callback.

---

### 6. `print_betree` Debug Visualization

Prints the tree structure to stderr showing partition hierarchy, pnode scores,
sub counts, and slot counts.

```c
void print_betree(const struct betree* betree);
```

---

### 7. `TRACE_LAST_VAR` Compile-Time Debug Tracing

When compiled with `-DTRACE_LAST_VAR`, enables per-sub stderr logging of
`last_var` transitions during evaluation. Tracing is selective — only subs
for which `report->is_trc_cb(arg, sub->data)` returns `true` are traced.

---

## Bug Fixes

- Float domain score going unreasonably high — guard changed from `diff == 0` to `diff < 1`
- `dyn_arr.c` — abort on `brealloc` failure instead of silent corruption

---

## Removed

- **`src/clone.c` / `src/clone.h`** — AST node cloning (290 lines). Dead code, removed.

---

## Build / Infrastructure

- **Makefile**: proper lex/yacc dependency tracking; selective test execution
  via `TESTS=` filter (e.g. `TESTS=*cb* make test`); test binaries output to
  `build/tests/`; fixed `$^` → `$<` in `.o` compilation rules.
- **Dockerfile + `docker.sh`**: Debian bookworm container for reproducible
  builds (build-essential, libgsl, valgrind, flex, bison).
- **`tests/runtests.sh`**: respects `TESTS` environment variable.
- **New test**: `tests/betree_search_cb_tests.c` — exercises callback API.
