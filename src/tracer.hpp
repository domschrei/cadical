#ifndef _tracer_h_INCLUDED
#define _tracer_h_INCLUDED

#include <stdexcept>
#include "observer.hpp" // Alphabetically after 'tracer'.

// Proof tracing to a file (actually 'File') in DRAT format.

namespace CaDiCaL {

class Tracer : public Observer {

  Internal * internal;
  File * file;
  bool binary, lrat, frat;
  void put_binary_zero ();
  void put_binary_lit (int external_lit);
  void put_binary_unsigned (int64_t n);
  void put_binary_signed (int64_t n);
  int64_t added, deleted;

public:

  Tracer (Internal *, File * file, bool binary, bool lrat, bool frat); // own and delete 'file'
  ~Tracer ();

  void add_original_clause (clause_id_t, const vector<int> &);
  void add_derived_clause (clause_id_t, const vector<clause_id_t> *, const vector<int> &){
      throw std::invalid_argument("Tracer add_derived_clauses must have 4 arguments; was only given 3");
  }
  void add_derived_clause (clause_id_t, const vector<clause_id_t> *, const vector<int> &, bool);
  void delete_clause (clause_id_t, const vector<int> &);
  void finalize_clause (clause_id_t, const vector<int> &);
  void add_todo (const vector<int64_t> &);
  bool closed ();
  void close ();
  void flush ();
};

}

#endif
