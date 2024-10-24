
#ifndef _internallrattracer_h_INCLUDED
#define _internallrattracer_h_INCLUDED

#include "onthefly_checking.hpp"

namespace CaDiCaL {

class InternalLratTracer : public FileTracer {

  Internal *internal;

#ifndef QUIET
  int64_t added, deleted;
#endif
  uint64_t latest_id;
  vector<uint64_t> delete_ids;

  LratCallbackProduceClause cb_produce;
  LratCallbackImportClause cb_import;
  LratCallbackDeleteClauses cb_delete;

  // support LRAT
  void lrat_add_clause (const uint64_t, bool, const vector<int> &,
                        const vector<uint64_t> &);
  void lrat_delete_clause (uint64_t);

public:
  InternalLratTracer (Internal *, LratCallbackProduceClause cbProduce, LratCallbackImportClause cbImport, LratCallbackDeleteClauses cbDel);
  ~InternalLratTracer ();

  void connect_internal (Internal *i) override;
  void begin_proof (uint64_t) override;

  void add_original_clause_with_signature (uint64_t id, const vector<int> & clause, const vector<uint8_t>& signature) override;
  void add_original_clause (uint64_t /*id*/, bool /*redundant*/, const vector<int> & /*clause*/, bool /*restore*/) override {};

  void add_derived_clause (uint64_t, bool, const vector<int> &,
                           const vector<uint64_t> &) override;

  void delete_clause (uint64_t, bool, const vector<int> &) override;

  void finalize_clause (uint64_t, const vector<int> &) override {} // skip

#ifndef QUIET
  void print_statistics ();
#endif
  bool closed () override;
  void close (bool) override;
  void flush (bool) override;
};

} // namespace CaDiCaL

#endif
