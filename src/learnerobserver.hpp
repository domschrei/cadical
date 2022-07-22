#ifndef _learnerobserver_h_INCLUDED
#define _learnerobserver_h_INCLUDED

#include "observer.hpp" // Alphabetically after 'learner'.

// Exporting clauses to an external learner

namespace CaDiCaL {

class LearnerObserver : public Observer {

  External * external;

public:

  LearnerObserver (External *);
  ~LearnerObserver ();

  void add_original_clause (clause_id_t, const vector<int> &);
  void add_derived_clause (clause_id_t, const vector<clause_id_t> *, const vector<int> &){
      throw std::invalid_argument("LearnerObserver add_derived_clauses must have 5 arguments; was only given 3");
  }
  void add_derived_clause (clause_id_t, const vector<clause_id_t> *, const vector<int> &, bool){
      throw std::invalid_argument("LearnerObserver add_derived_clauses must have 5 arguments; was only given 4");
  }
  void add_derived_clause (clause_id_t, const vector<clause_id_t> *, const vector<int> &, bool, int);
  void delete_clause (clause_id_t, const vector<int> &);
  void finalize_clause (clause_id_t, const vector<int> &);
  void add_todo (const vector<int64_t> &);
  bool closed ();
  void close ();
  void flush ();
};

}

#endif
