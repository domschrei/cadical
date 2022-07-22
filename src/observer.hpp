#ifndef _observer_hpp_INCLUDED
#define _observer_hpp_INCLUDED

namespace CaDiCaL {

// Proof observer class used to act on added, derived or deleted clauses.

class Observer {

public:

  Observer () { }
  virtual ~Observer () { }

  virtual void add_original_clause (clause_id_t, const vector<int> &) { }

  // Notify the observer that a new clause has been derived.
  //
  // glue is ignored if clause is unit
  virtual void add_derived_clause (clause_id_t id, const vector<clause_id_t> *chain, const vector<int> &lits, bool is_imported, int) {
      add_derived_clause(id, chain, lits, is_imported);
  }
  virtual void add_derived_clause (clause_id_t id, const vector<clause_id_t> *chain, const vector<int> &lits, bool) {
      add_derived_clause(id, chain, lits);
  }
  virtual void add_derived_clause (clause_id_t, const vector<clause_id_t> *, const vector<int> &) { }

  // Notify the observer that a clause is not used anymore.
  //
  virtual void delete_clause (clause_id_t, const vector<int> &) { }

  // Notify the observer that a clause is active at the end of processing.
  //
  virtual void finalize_clause (clause_id_t, const vector<int> &) { }

  virtual void add_todo (const vector<int64_t> &) { }

  virtual void flush () { }
};

}

#endif
