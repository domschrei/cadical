Adding FRAT style proofs to CaDiCaL
===============================================================================

This is a modification of the CaDiCaL SAT solver (<http://fmv.jku.at/cadical>)
which adds support for FRAT proofs.

Use `./configure && make` to configure and build `cadical` and the library
`libcadical.a` in the default `build` sub-directory.  The header file of
the library is [`src/cadical.hpp`](src/cadical.hpp) and includes an example
for API usage.

See [`BUILD.md`](BUILD.md) for options and more details related to the build
process and [`test/README.md`](test/README.md) for testing the library and
the solver.

The solver has the following usage `cadical [ dimacs [ proof ] ]`.
See `cadical -h` for more options.

To enable FRAT proof output, run `cadical dimacs.cnf proof.frat --lrat=true`.

## Implementation notes

The diff is visible [here](https://github.com/arminbiere/cadical/compare/master...digama0:master). The changes break down into the following parts:

* Clause IDs were previously only enabled if you compile with the `LOGGING` preprocessor flag. Now they are required.

* Clause IDs are passed around in lots of places, changing many function signatures to include a `int64_t id` argument.

* A `chain` global variable is used to hold on to the proof of the clause that is about to be added. The way this variable gets filled depends on the proof step and is the most "creative" part of the modification.

* On code paths that do not provide proofs because I wasn't able to see an easy way to do it or I couldn't figure out the code, there is a call to the `PROOF_TODO (proof, reason, todo_id)` macro. The `reason` is a short description of the code path and appears only in logs, while the `todo_id` is a number that appears in a `t <todo_id> 0` step that is emitted as part of the FRAT proof. These steps can be analyzed by the [FRAT toolchain](https://github.com/digama0/frat) to find out which code paths are most common / worth more thorough investigation.

* Unit literals are also clauses in the FRAT format, but CaDiCaL treats them separately. These also need IDs, and this is the function of the `unit_id` field in `Var`, which is the ID of the clause that proves or disproves the literal, if it is fixed, and `0` otherwise.

* There are some new functions to `finalize` a clause. The main function is `Internal::finalize ()`. This is done after a proof of `unsat` is found, and just dumps the list of all clauses that are not yet deleted at the point that the `unsat` is found.

* The actual formatting of the FRAT file on disk is performed by [`tracer.cpp`](src/tracer.cpp). This implementation supports both ASCII and binary versions of the FRAT format; the binary version is default and the ASCII version is enabled with `--lrat=true --binary=false`.

* There are also some changes to `drat-trim.c`, which is used by the test harness, to make sure that it does not fail with exit code 0.