#include "internal.hpp"

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Compacting removes holes generated by inactive variables (fixed,
// eliminated, substituted or pure) by mapping active variables indices down
// to a contiguous interval of indices.

/*------------------------------------------------------------------------*/

bool Internal::compacting () {
  if (level)
    return false;
  if (!opts.compact)
    return false;
  if (stats.conflicts < lim.compact)
    return false;
  int inactive = max_var - active ();
  assert (inactive >= 0);
  if (!inactive)
    return false;
  if (inactive < opts.compactmin)
    return false;
  return inactive >= (1e-3 * opts.compactlim) * max_var;
}

/*------------------------------------------------------------------------*/

struct Mapper {

  Internal *internal;
  int new_max_var;             // New 'max_var' after compacting.
  int *table;                  // Old variable index to new literal map.
  int first_fixed;             // First fixed variable index.
  int map_first_fixed;         // Mapped literal of first fixed variable.
  signed char first_fixed_val; // Value of first fixed variable.
  size_t new_vsize;

  /*----------------------------------------------------------------------*/
  // We produce a compacting garbage collector like map of old 'src' to
  // new 'dst' variables.  Inactive variables are just skipped except for
  // fixed ones which will be mapped to the first fixed variable (in the
  // appropriate phase).  This avoids to handle the case 'fixed value'
  // separately as it is done in Lingeling, where fixed variables are
  // mapped to the internal variable '1'.
  //
  Mapper (Internal *i)
      : internal (i), new_max_var (0), first_fixed (0), map_first_fixed (0),
        first_fixed_val (0) {
    table = new int[internal->max_var + 1u];
    clear_n (table, internal->max_var + 1u);

    assert (!internal->level);

    for (auto src : internal->vars) {
      const Flags &f = internal->flags (src);
      if (f.active ())
        table[src] = ++new_max_var;
      else if (f.fixed () && !first_fixed)
        table[first_fixed = src] = map_first_fixed = ++new_max_var;
    }

    first_fixed_val = first_fixed ? internal->val (first_fixed) : 0;
    new_vsize = new_max_var + 1u;
  }

  ~Mapper () { delete[] table; }

  /*----------------------------------------------------------------------*/
  // Map old variable indices.  A result of zero means not mapped.
  //
  int map_idx (int src) {
    assert (0 < src);
    assert (src <= internal->max_var);
    const int res = table[src];
    assert (res <= new_max_var);
    return res;
  }

  /*----------------------------------------------------------------------*/
  // The 'map_idx' above is just a look-up into the 'table'.  Here we have
  // to care about signedness of 'src', and in addition that fixed variables
  // have all to be mapped to the first fixed variable 'first_fixed'.
  //
  int map_lit (int src) {
    int res = map_idx (abs (src));
    if (!res) {
      const signed char tmp = internal->val (src);
      if (tmp) {
        assert (first_fixed);
        res = map_first_fixed;
        if (tmp != first_fixed_val)
          res = -res;
      }
    } else if ((src) < 0)
      res = -res;
    assert (abs (res) <= new_max_var);
    return res;
  }

  /*----------------------------------------------------------------------*/
  // Map positive variable indices in vector.
  //
  template <class T> void map_vector (vector<T> &v) {
    for (auto src : internal->vars) {
      const int dst = map_idx (src);
      if (!dst)
        continue;
      assert (0 < dst);
      assert (dst <= src);
      v[dst] = v[src];
    }
    v.resize (new_vsize);
    shrink_vector (v);
  }

  /*----------------------------------------------------------------------*/
  // Map positive and negative variable indices in two-sided vector.
  //
  template <class T> void map2_vector (vector<T> &v) {
    for (auto src : internal->vars) {
      const int dst = map_idx (src);
      if (!dst)
        continue;
      assert (0 < dst);
      assert (dst <= src);
      v[2 * dst] = v[2 * src];
      v[2 * dst + 1] = v[2 * src + 1];
    }
    v.resize (2 * new_vsize);
    shrink_vector (v);
  }

  /*----------------------------------------------------------------------*/
  // Map a vector of literals, flush inactive literals, then resize and
  // shrink it to fit the new size after flushing.
  //
  void map_flush_and_shrink_lits (vector<int> &v) {
    const auto end = v.end ();
    auto j = v.begin (), i = j;
    for (; i != end; i++) {
      const int src = *i;
      int dst = map_idx (abs (src));
      assert (abs (dst) <= abs (src));
      if (!dst)
        continue;
      if (src < 0)
        dst = -dst;
      *j++ = dst;
    }
    v.resize (j - v.begin ());
    shrink_vector (v);
  }
};

/*------------------------------------------------------------------------*/

static signed char *ignore_clang_analyze_memory_leak_warning;

void Internal::compact () {

  START (compact);

  assert (active () < max_var);

  stats.compacts++;

  assert (!level);
  assert (!unsat);
  assert (!conflict);
  assert (clause.empty ());
  assert (levels.empty ());
  assert (analyzed.empty ());
  assert (minimized.empty ());
  assert (control.size () == 1);
  assert (propagated == trail.size ());

  garbage_collection ();

  Mapper mapper (this);

  if (mapper.first_fixed)
    LOG ("found first fixed %d",
         sign (mapper.first_fixed_val) * mapper.first_fixed);
  else
    LOG ("no variable fixed");

  if (!assumptions.empty ()) {
    assert (!external->assumptions.empty ());
    LOG ("temporarily reset internal assumptions");
    reset_assumptions ();
  }

  const bool is_constraint = !constraint.empty ();
  if (is_constraint) {
    assert (!external->constraint.empty ());
    LOG ("temporarily reset internal constraint");
    reset_constraint ();
  }

  /*======================================================================*/
  // In this first part we only map stuff without reallocation / shrinking.
  /*======================================================================*/

  // Flush the external indices.  This has to occur before we map 'vals'.
  // Also fixes external units.
  //
  for (auto eidx : external->vars) {
    int src = external->e2i[eidx];
    if (!src) {
      continue;
    }
    assert (eidx > 0);
    assert (external->ext_units.size () >= (size_t) 2 * eidx + 1);
    uint64_t id1 = external->ext_units[2 * eidx];
    uint64_t id2 = external->ext_units[2 * eidx + 1];
    assert (!id1 || !id2);
    if (!id1 && !id2) {
      uint64_t new_id1 = unit_clauses[2 * src];
      uint64_t new_id2 = unit_clauses[2 * src + 1];
      external->ext_units[2 * eidx] = new_id1;
      external->ext_units[2 * eidx + 1] = new_id2;
    }
    int dst = mapper.map_lit (src);
    LOG ("compact %" PRId64
         " maps external %d to internal %d from internal %d",
         stats.compacts, eidx, dst, src);
    external->e2i[eidx] = dst;
  }

  // Delete garbage units. Needs to occur before resizing unit_clauses
  //
  for (auto src : internal->vars) {
    const int dst = mapper.map_idx (src);
    assert (dst <= src);
    const signed char tmp = internal->val (src);
    if (!dst && !tmp) {
      unit_clauses[2 * src] = 0;
      unit_clauses[2 * src + 1] = 0;
      continue;
    }
    if (!tmp || src == mapper.first_fixed) {
      assert (0 < dst);
      if (dst == src)
        continue;
      assert (!unit_clauses[2 * dst] && !unit_clauses[2 * dst + 1]);
      unit_clauses[2 * dst] = unit_clauses[2 * src];
      unit_clauses[2 * dst + 1] = unit_clauses[2 * src + 1];
      unit_clauses[2 * src] = 0;
      unit_clauses[2 * src + 1] = 0;
      continue;
    }
    uint64_t id = unit_clauses[2 * src];
    int lit = src;
    if (!id) {
      id = unit_clauses[2 * src + 1];
      lit = -lit;
    }
    unit_clauses[2 * src] = 0;
    unit_clauses[2 * src + 1] = 0;
    assert (!opts.lrat || id);
  }
  unit_clauses.resize (2 * mapper.new_vsize);
  shrink_vector (unit_clauses);

  // Map the literals in all clauses.
  //
  for (const auto &c : clauses) {
    assert (!c->garbage);
    for (auto &src : *c) {
      assert (!val (src));
      int dst;
      dst = mapper.map_lit (src);
      assert (dst || c->garbage);
      src = dst;
    }
  }

  // Map the blocking literals in all watches.
  //
  if (!wtab.empty ())
    for (auto lit : lits)
      for (auto &w : watches (lit))
        w.blit = mapper.map_lit (w.blit);

  // We first flush inactive variables and map the links in the queue.  This
  // has to be done before we map the actual links data structure 'links'.
  {
    int prev = 0, mapped_prev = 0, next;
    for (int idx = queue.first; idx; idx = next) {
      next = links[idx].next;
      if (idx == mapper.first_fixed)
        continue;
      const int dst = mapper.map_idx (idx);
      if (!dst)
        continue;
      assert (active (idx));
      if (prev)
        links[prev].next = dst;
      else
        queue.first = dst;
      links[idx].prev = mapped_prev;
      mapped_prev = dst;
      prev = idx;
    }
    if (prev)
      links[prev].next = 0;
    else
      queue.first = 0;
    queue.unassigned = queue.last = mapped_prev;
  }

  /*======================================================================*/
  // In the second part we map, flush and shrink arrays.
  /*======================================================================*/

  assert (trail.size () == num_assigned);
  mapper.map_flush_and_shrink_lits (trail);
  propagated = trail.size ();
  num_assigned = trail.size ();
  if (mapper.first_fixed) {
    assert (trail.size () == 1);
    var (mapper.first_fixed).trail = 0; // before mapping 'vtab'
  } else
    assert (trail.empty ());

  if (opts.reimply) {
    if (notify_trail.size () != notified)
      notify_assignments ();
    mapper.map_flush_and_shrink_lits (notify_trail);
    notified = notify_trail.size ();
  }

  if (!probes.empty ())
    mapper.map_flush_and_shrink_lits (probes);

  /*======================================================================*/
  // In the third part we map stuff and also reallocate memory.
  /*======================================================================*/

  // Now we continue in reverse order of allocated bytes, e.g., see
  // 'Internal::enlarge' which reallocates in order of allocated bytes.

  mapper.map_vector (ftab);
  mapper.map_vector (parents);
  mapper.map_vector (marks);
  mapper.map_vector (phases.saved);
  mapper.map_vector (phases.forced);
  mapper.map_vector (phases.target);
  mapper.map_vector (phases.best);
  mapper.map_vector (phases.prev);
  mapper.map_vector (phases.min);

  // Special code for 'frozentab'.
  //
  for (auto src : vars) {
    const int dst = abs (mapper.map_lit (src));
    if (!dst)
      continue;
    if (src == dst)
      continue;
    assert (dst < src);
    frozentab[dst] += frozentab[src];
    frozentab[src] = 0;
  }
  frozentab.resize (mapper.new_vsize);
  shrink_vector (frozentab);

  // Special code for 'relevanttab'.
  //
  for (auto src : vars) {
    const int dst = abs (mapper.map_lit (src));
    if (!dst)
      continue;
    if (src == dst)
      continue;
    assert (dst < src);

    relevanttab[dst] += relevanttab[src];
    relevanttab[src] = 0;
  }
  relevanttab.resize (mapper.new_vsize);
  shrink_vector (relevanttab);

  /*----------------------------------------------------------------------*/

  if (!external->assumptions.empty ()) {

    for (const auto &elit : external->assumptions) {
      assert (elit);
      assert (elit != INT_MIN);
      int eidx = abs (elit);
      assert (eidx <= external->max_var);
      int ilit = external->e2i[eidx];
      assert (ilit); // Because we froze all!!!
      if (elit < 0)
        ilit = -ilit;
      assume (ilit);
    }

    PHASE ("compact", stats.compacts, "reassumed %zd external assumptions",
           external->assumptions.size ());
  }

  // Special case for 'val' as for 'val' we trade branch less code for
  // memory and always allocated an [-maxvar,...,maxvar] array.
  {
    signed char *new_vals = new signed char[2 * mapper.new_vsize];
    ignore_clang_analyze_memory_leak_warning = new_vals;
    new_vals += mapper.new_vsize;
    for (auto src : vars)
      new_vals[-mapper.map_idx (src)] = vals[-src];
    for (auto src : vars)
      new_vals[mapper.map_idx (src)] = vals[src];
    new_vals[0] = 0;
    vals -= vsize;
    delete[] vals;
    vals = new_vals;
  }

  // 'constrain' uses 'val', so this code has to be after remapping that
  if (is_constraint) {
    assert (!level);
    assert (!external->constraint.back ());
    for (auto elit : external->constraint) {
      assert (elit != INT_MIN);
      int eidx = abs (elit);
      assert (eidx <= external->max_var);
      int ilit = external->e2i[eidx];
      assert (!ilit == !elit);
      if (elit < 0)
        ilit = -ilit;
      LOG ("re adding lit external %d internal %d to constraint", elit,
           ilit);
      constrain (ilit);
    }
    PHASE ("compact", stats.compacts,
           "added %zd external literals to constraint",
           external->constraint.size () - 1);
  }

  mapper.map_vector (i2e);
  mapper.map2_vector (ptab);
  mapper.map_vector (btab);
  mapper.map_vector (gtab);
  mapper.map_vector (links);
  mapper.map_vector (vtab);
  if (!ntab.empty ())
    mapper.map2_vector (ntab);
  if (!wtab.empty ())
    mapper.map2_vector (wtab);
  if (!otab.empty ())
    mapper.map2_vector (otab);
  if (!big.empty ())
    mapper.map2_vector (big);

  /*======================================================================*/
  // In the fourth part we map the binary heap for scores.
  /*======================================================================*/

  // The simplest way to map a binary heap is to get all elements from the
  // heap and reinsert them.  This could be slightly improved in terms of
  // speed if we add a 'flush (int * map)' function to 'Heap', but that is
  // pretty complicated and would require that the 'Heap' knows that mapped
  // elements with 'zero' destination should be flushed.

  vector<int> saved;
  assert (saved.empty ());
  if (!scores.empty ()) {
    while (!scores.empty ()) {
      auto lim = std::min(scores.size (), 2048UL);
      for (size_t i = 0; i < lim; i++) {
        const int src = scores.front ();
        scores.pop_front ();
        const int dst = mapper.map_idx (src);
        if (!dst)
          continue;
        if (src == mapper.first_fixed)
          continue;
        saved.push_back (dst);
      }
      if (opts.lrat && internal->termination_forced) return;
    }
    scores.erase ();
  }
  mapper.map_vector (stab);
  if (!saved.empty ()) {
    for (const auto idx : saved)
      scores.push_back (idx);
    scores.shrink ();
  }

  /*----------------------------------------------------------------------*/

  PHASE ("compact", stats.compacts,
         "reducing internal variables from %d to %d", max_var,
         mapper.new_max_var);

  /*----------------------------------------------------------------------*/

  // Need to adjust the target and best assigned counters too.

  size_t new_target_assigned = 0, new_best_assigned = 0;

  for (auto idx : Range (mapper.new_max_var)) {
    if (phases.target[idx])
      new_target_assigned++;
    if (phases.best[idx])
      new_best_assigned++;
  }

  LOG ("reset target assigned from %zd to %zd", target_assigned,
       new_target_assigned);
  LOG ("reset best assigned from %zd to %zd", best_assigned,
       new_best_assigned);

  target_assigned = new_target_assigned;
  best_assigned = new_best_assigned;
  no_conflict_until = 0;
  notified = 0;

  INIT_EMA (averages.current.trail.fast, opts.ematrailfast);
  INIT_EMA (averages.current.trail.slow, opts.ematrailslow);

  /*----------------------------------------------------------------------*/

  max_var = mapper.new_max_var;
  vsize = mapper.new_vsize;

  stats.unused = 0;
  stats.inactive = stats.now.fixed = mapper.first_fixed ? 1 : 0;
  stats.now.substituted = stats.now.eliminated = stats.now.pure = 0;

  check_var_stats ();

  int64_t delta = opts.compactint * (stats.compacts + 1);
  lim.compact = stats.conflicts + delta;

  PHASE ("compact", stats.compacts,
         "new compact limit %" PRId64 " after %" PRId64 " conflicts",
         lim.compact, delta);

  STOP (compact);
}

} // namespace CaDiCaL
