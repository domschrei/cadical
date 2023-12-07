
#include "internal.hpp"

// Write a LRAT derivation straight to the tracers
// without the detour over "Proof::add_derived_*_clause".
void CaDiCaL::Internal::add_clause_to_proof (uint64_t id) {
  std::vector<int> elits;
  for (int ilit : clause) elits.push_back(externalize (ilit));
  for (auto &tracer : tracers) {
    tracer->add_derived_clause (id, true, elits, lrat_chain);
  }
}

// Attempt to import an incoming unit clause, possibly arising from
// a simplification of an incoming non-unit clause (bool simplified).
// In that case, lrat_chain may contain IDs of this simplification.
void CaDiCaL::Internal::try_import_unit (uint64_t id, int elit, bool simplified) {

  // Do not learn unit clause if marked as witness
  if (external->marked (external->witness, elit)) {
    internal->stats.clauseimport.r_wit++;
    internal->stats.clauseimport.discarded++;
    if (simplified && lrat) lrat_chain.clear ();
    return;
  }
  int ilit = external->internalize (elit);
  auto& f = flags(ilit);
  // Do not import eliminated or substituted literal
  if (f.eliminated () || f.substituted ()) {
    internal->stats.clauseimport.r_el++;
    internal->stats.clauseimport.discarded++;
    if (simplified && lrat) lrat_chain.clear ();
    return;
  }
  // Do not import units which are already fixed
  if (f.fixed ()) {
    internal->stats.clauseimport.r_fx++;
    internal->stats.clauseimport.discarded++;
    if (simplified && lrat) lrat_chain.clear ();
    return;
  }
  // Actually add the unit clause
  uint64_t impclsid = simplified ? next_lrat_id () : id;
  if (simplified) {
    // Clause was simplified: Need to add an LRAT derivation
    if (lrat) {
      assert(!lrat_chain.empty ());
      lrat_chain.push_back (id); // add ID of the "original" incoming clause!
      clause.resize (1, ilit);
      add_clause_to_proof (impclsid);
      lrat_chain.clear ();
    }
    // Re-export the clause in its simplified form
    external->export_learned_unit_clause (impclsid, ilit);
  }
  assign_original_unit (impclsid, ilit);
  internal->stats.clauseimport.imported++;
}

// Attempt to import a single clause.
void CaDiCaL::Internal::handle_incoming_clause (uint64_t id, int glue, std::vector<int>& cls) {

  const size_t size = cls.size ();
  assert (size > 0);
  assert (clause.empty ());
  assert (lrat_chain.empty ());

  // Unit clause?
  if (size == 1) {
    try_import_unit (id, cls[0], false);
    return;
  }

  // Handle non-unit clause
  assert (glue > 0);
  bool reducedSize = false;

  // Analyze clause literals
  bool addClause = true;
  for (size_t i = 0; i < size; i++) {

    int elit = cls[i];
    assert (elit != 0);

    if (external->marked (external->witness, elit)) {
      // Literal marked as witness: Cannot import
      internal->stats.clauseimport.r_wit++;
      addClause = false; break;
    }

    int ilit = external->internalize(elit);

    auto& f = flags (ilit);
    if (f.eliminated () || f.substituted ()) {
      // Literal has been eliminated: do not add this clause.
      internal->stats.clauseimport.r_el++;
      addClause = false; break;
    }
    if (f.fixed ()) {
      // Literal is fixed
      if (val (ilit) == 1) {
        // TRUE: Clause must be omitted.
        internal->stats.clauseimport.r_fx++;
        addClause = false; break;
      } // else: FALSE - literal must be omitted from the clause.
      assert (val (ilit) == -1);
      reducedSize = true;
      if (lrat) {
        // Add the unit clause which shortens this clause to the LRAT chain.
        auto unitidx = vlit (-ilit);
        assert (unitidx < unit_clauses.size ());
        uint64_t uid = unit_clauses[unitidx];
        assert (uid);
        lrat_chain.push_back (uid);
      }
    } else {
      // Can treat literal normally.
      clause.push_back (ilit);
    }
  }

  // Can clause be imported?
  if (!addClause) {
    internal->stats.clauseimport.discarded++;
    clause.clear ();
    lrat_chain.clear ();
    return;
  }

  // Clause can be imported. Which size?
  if (clause.empty ()) {
    // found empty clause -- UNSAT
    assert (reducedSize);
    if (lrat) {
      assert (!lrat_chain.empty ());
      lrat_chain.push_back (id); // add ID of the "original" incoming clause!
      add_clause_to_proof (next_lrat_id ());
      lrat_chain.clear ();
    }
    internal->stats.clauseimport.r_fx++;
    internal->stats.clauseimport.discarded++;
    unsat = true;
    return;
  }
  if (clause.size () == 1) {
    // unit clause due to shortening -- to be handled below
    assert (reducedSize);
    int elit = internal->externalize (clause[0]);
    clause.clear ();
    try_import_unit (id, elit, true);
    return;
  }
  // Handle clause of size >= 2 being learnt
  uint64_t impclsid = reducedSize ? next_lrat_id () : id;
  Clause * res = new_clause (true, glue, false, impclsid);
  if (reducedSize && lrat) {
    assert (!lrat_chain.empty ());
    lrat_chain.push_back (id); // add ID of the "original" incoming clause!
    add_clause_to_proof (impclsid);
    lrat_chain.clear ();
  }
  clause.clear ();
  assert (watching ());
  watch_clause (res);
  internal->stats.clauseimport.imported++;
}


bool CaDiCaL::Internal::importing () {
  return level == 0 && external->learnSource != 0 
      && watching() && external->learnSource->hasNextClause ();
}

// Top-level import function
void CaDiCaL::Internal::import_redundant_clauses (int& res) {
  if (external->learnSource == 0) return;
  if (res != 0) return;

  // Import external clauses.
  while (external->learnSource->hasNextClause ()) {

    // Fetch pointer to 1st literal and size of the clause (plus ID, glue)
    uint64_t id;
    int glue;
    auto cls = external->learnSource->getNextClause (id, glue);
    handle_incoming_clause(id, glue, cls);

    // Stop importing if SAT or UNSAT was found
    if (unsat || res == 20) {
      res = 20;
      return;
    }
    if (satisfied ()) {
      res = 10;
      return;
    }
  }
}
