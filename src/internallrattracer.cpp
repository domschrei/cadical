
#include "internal.hpp"
#include "internallrattracer.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits.h>

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

InternalLratTracer::InternalLratTracer (Internal *i, LratCallbackProduceClause cbProduce, LratCallbackImportClause cbImport, LratCallbackDeleteClauses cbDel) :
  internal(i), added(0), deleted(0), latest_id(0), cb_produce(cbProduce), cb_import(cbImport), cb_delete(cbDel) {}

void InternalLratTracer::connect_internal (Internal *i) {
  internal = i;
  LOG ("LRAT TRACER connected to internal");
}

InternalLratTracer::~InternalLratTracer () {
  LOG ("LRAT TRACER delete");
}

/*------------------------------------------------------------------------*/

void InternalLratTracer::lrat_add_clause (const uint64_t id, bool redundant,
                                  const vector<int> &clause,
                                  const vector<uint64_t> &chain) {

  // sanity check
  if (id <= latest_id) {
    printf("ERROR - added import ID %lu out of order (prev: %lu)!\n", id, latest_id);
    abort();
  }

  if (delete_ids.size ()) {
    bool ok = cb_delete (delete_ids.data (), delete_ids.size ());
    if (!ok) abort ();
    delete_ids.clear ();
  }
  latest_id = id;

  if (clause.size () == 1) {
    // Remember the ID of this unit clause as an *external* literal
    // so that internal variable domain compacting does not destroy the mapping.
    internal->register_lrat_id_of_unit_elit (id, clause[0]);
  }

  bool export_clause = internal->opts.signsharedcls
    && (redundant || clause.size () == 1)
    && internal->external->learner
    && internal->external->learner->learning (clause.size ());

  auto clauseToExport = &clause;
  int sigSize = 16;
  uint8_t sigData[sigSize];
  if (export_clause) {
    auto sorted = new std::vector<int>(clause.data (), clause.data () + clause.size ());
    std::sort(sorted->begin(), sorted->end());
    clauseToExport = sorted;
  }

  bool ok = cb_produce (id, clauseToExport->data (), clauseToExport->size (), chain.data (), chain.size (),
    export_clause ? sigData : nullptr, sigSize);
  if (!ok) abort ();
  internal->stats.produced_cls++;

  // Export the clause whose derivation you just output.
  if (export_clause) {

    // Export clause with signature
    if (clauseToExport->size () == 0)
      internal->external->export_learned_empty_clause ();
    else if (clauseToExport->size () == 1)
      internal->external->export_learned_external_unit_clause (id, clauseToExport->at(0), sigData, sigSize);
    else {
      auto& glue = internal->last_glue;
      if (!glue || glue > clauseToExport->size ()) glue = clauseToExport->size ();
      internal->external->export_learned_external_large_clause (id, *clauseToExport, internal->last_glue, sigData, sigSize);
      glue = 0;
    }

    internal->stats.signed_produced_cls++;
    delete clauseToExport;
  }
}

void InternalLratTracer::lrat_delete_clause (uint64_t id) {
  if (!internal->opts.lratdeletelines) return;
  delete_ids.push_back (id); // pushing off deletion for later
}

/*------------------------------------------------------------------------*/

void InternalLratTracer::add_original_clause_with_signature (uint64_t id,
    const vector<int> & clause, const std::vector<uint8_t>& signature) {
  bool ok = cb_import (id, clause.data (), clause.size (), signature.data (), signature.size ());
  if (!ok) abort();
}

void InternalLratTracer::add_derived_clause (uint64_t id, bool redundant,
                                     const vector<int> &clause,
                                     const vector<uint64_t> &chain) {
  LOG ("LRAT TRACER tracing addition of derived clause");
  lrat_add_clause (id, redundant, clause, chain);
#ifndef QUIET
  added++;
#endif
}

void InternalLratTracer::delete_clause (uint64_t id, bool, const vector<int> &) {
  LOG ("LRAT TRACER tracing deletion of clause");
  lrat_delete_clause (id);
#ifndef QUIET
  deleted++;
#endif
}

void InternalLratTracer::begin_proof (uint64_t id) {
  LOG ("LRAT TRACER tracing begin of proof");
  latest_id = id;
}

/*------------------------------------------------------------------------*/

bool InternalLratTracer::closed () { return false; }
#ifndef QUIET
void InternalLratTracer::print_statistics () {}
#endif
void InternalLratTracer::close (bool /*print*/) {
  printf("[CaDiCaL] produced=%lu produced_signed=%lu incoming=%lu incoming_validated=%lu\n",
    internal->stats.produced_cls, internal->stats.signed_produced_cls,
    internal->stats.incoming_cls, internal->stats.validated_incoming_cls);
}
void InternalLratTracer::flush (bool /*print*/) {}

} // namespace CaDiCaL
