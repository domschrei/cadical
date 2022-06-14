#include "internal.hpp"

/*------------------------------------------------------------------------*/

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// See corresponding header file 'cadical.hpp' (!) for more information.
//
// Again, to avoid confusion, note that, 'cadical.hpp' is the header file of
// this file 'solver.cpp', since we want to call the application and main
// file 'cadical.cpp', while at the same time using 'cadical.hpp' as the
// main header file of the library (and not 'solver.hpp').

/*------------------------------------------------------------------------*/
#ifdef LOGGING

// Needs to be kept in sync with the color schemes used in 'logging.cpp'.
//
#define api_code blue_code              // API call color
#define log_code magenta_code           // standard/default logging color
#define emph_code bright_magenta_code   // emphasized logging color

#endif
/*------------------------------------------------------------------------*/

// Log state transitions.

#define STATE(S) \
do { \
  assert (is_power_of_two (S)); \
  if (_state == S) break; \
  _state = S; \
  LOG ("API enters state %s" #S "%s", \
    tout.emph_code (), tout.normal_code ()); \
} while (0)

void Solver::transition_to_unknown_state () {
  if (state () == CONFIGURING) {
    LOG ("API leaves state %sCONFIGURING%s",
      tout.emph_code (), tout.normal_code ());
    if (internal->opts.check &&
        internal->opts.checkproof) {
      internal->check ();
    }
  } else if (state () == SATISFIED) {
    LOG ("API leaves state %sSATISFIED%s",
      tout.emph_code (), tout.normal_code ());
    external->reset_assumptions ();
  } else if (state () == UNSATISFIED) {
    LOG ("API leaves state %sUNSATISFIED%s",
      tout.emph_code (), tout.normal_code ());
    external->reset_assumptions ();
  }
  if (state () != UNKNOWN) STATE (UNKNOWN);
}

/*------------------------------------------------------------------------*/
#ifdef LOGGING
/*------------------------------------------------------------------------*/

// The following logging code is useful for debugging mostly (or trying to
// understand what the solver is actually doing).  It needs to be enabled
// during configuration using the '-l' option for './configure', which
// forces 'LOGGING' to be defined during compilation.  This includes all the
// logging code, which then still needs to enabled during run-time with the
// '-l' or 'log' option.

static void
log_api_call (Internal * internal,
  const char * name, const char * suffix) {
  Logger::log (internal, "API call %s'%s ()'%s %s",
    tout.api_code (), name, tout.log_code (), suffix);
}

static void
log_api_call (Internal * internal,
  const char * name, int arg, const char * suffix) {
  Logger::log (internal, "API call %s'%s (%d)'%s %s",
    tout.api_code (), name, arg, tout.log_code (), suffix);
}

static void
log_api_call (Internal * internal,
  const char * name, const char * arg, const char * suffix) {
  Logger::log (internal, "API call %s'%s (\"%s\")'%s %s",
    tout.api_code (), name, arg, tout.log_code (), suffix);
}

static void
log_api_call (Internal * internal,
  const char * name, const char * a1, int a2, const char * s) {
  Logger::log (internal, "API call %s'%s (\"%s\", %d)'%s %s",
    tout.api_code (), name, a1, a2, tout.log_code (), s);
}

/*------------------------------------------------------------------------*/

// We factored out API call begin/end logging and use overloaded functions.

static void
log_api_call_begin (Internal * internal, const char * name) {
  Logger::log_empty_line (internal);
  log_api_call (internal, name, "started");
}

static void
log_api_call_begin (Internal * internal, const char * name, int arg) {
  Logger::log_empty_line (internal);
  log_api_call (internal, name, arg, "started");
}

static void
log_api_call_begin (Internal * internal,
                    const char * name, const char * arg) {
  Logger::log_empty_line (internal);
  log_api_call (internal, name, arg, "started");
}

static void
log_api_call_begin (Internal * internal, const char * name,
                    const char * arg1, int arg2) {
  Logger::log_empty_line (internal);
  log_api_call (internal, name, arg1, arg2, "started");
}

/*------------------------------------------------------------------------*/

static void
log_api_call_end (Internal * internal, const char * name)
{
  log_api_call (internal, name, "succeeded");
}

static void
log_api_call_end (Internal * internal, const char * name, int lit) {
  log_api_call (internal, name, lit, "succeeded");
}

static void
log_api_call_end (Internal * internal,
                    const char * name, const char * arg) {
  Logger::log_empty_line (internal);
  log_api_call (internal, name, arg, "succeeded");
}

static void
log_api_call_end (Internal * internal, const char * name,
                  const char * arg,
                  bool res)
{
  log_api_call (internal, name, arg, res ? "succeeded" : "failed");
}

static void
log_api_call_end (Internal * internal, const char * name,
                  const char * arg, int val,
                  bool res)
{
  log_api_call (internal, name, arg, val, res ? "succeeded" : "failed");
}

static void
log_api_call_returns (Internal * internal, const char * name, bool res) {
  log_api_call (internal, name,
    res ? "returns 'true'" : "returns 'false'");
}

static void
log_api_call_returns (Internal * internal, const char * name, int res) {
  char fmt[32];
  sprintf (fmt, "returns '%d'", res);
  log_api_call (internal, name, fmt);
}

static void
log_api_call_returns (Internal * internal, const char * name, int64_t res) {
  char fmt[32];
  sprintf (fmt, "returns '%" PRId64 "'", res);
  log_api_call (internal, name, fmt);
}

static void
log_api_call_returns (Internal * internal,
                      const char * name, int lit, int res) {
  char fmt[32];
  sprintf (fmt, "returns '%d'", res);
  log_api_call (internal, name, lit, fmt);
}

static void
log_api_call_returns (Internal * internal,
                      const char * name, const char * arg, bool res) {
  log_api_call (internal, name, arg,
    res ? "returns 'true'" : "returns 'false'");
}

static void
log_api_call_returns (Internal * internal,
                      const char * name, int lit, bool res) {
  log_api_call (internal, name, lit,
    res ? "returns 'true'" : "returns 'false'");
}

static void
log_api_call_returns (Internal * internal,
  const char * name, const char * arg, const char * res) {
  Logger::log (internal, "API call %s'%s (\"%s\")'%s returns '%s'",
    tout.api_code (), name, arg, tout.log_code (), res ? res : "<null>");
}

static void
log_api_call_returns (Internal * internal,
  const char * name, const char * arg1, int arg2, const char * res) {
  Logger::log (internal, "API call %s'%s (\"%s\", %d)'%s returns '%s'",
    tout.api_code (), name, arg1, arg2, tout.log_code (),
    res ? res : "<null>");
}

/*------------------------------------------------------------------------*/

#define LOG_API_CALL_BEGIN(...) \
do { \
  if (!internal->opts.log) break; \
  log_api_call_begin (internal, __VA_ARGS__); \
} while (0)

#define LOG_API_CALL_END(...) \
do { \
  if (!internal->opts.log) break; \
  log_api_call_end (internal, __VA_ARGS__); \
} while (0)

#define LOG_API_CALL_RETURNS(...) \
do { \
  if (!internal->opts.log) break; \
  log_api_call_returns (internal, __VA_ARGS__); \
} while (0)

/*------------------------------------------------------------------------*/
#else   // end of 'then' part of 'ifdef LOGGING'
/*------------------------------------------------------------------------*/

#define LOG_API_CALL_BEGIN(...) do { } while (0)
#define LOG_API_CALL_END(...) do { } while (0)
#define LOG_API_CALL_RETURNS(...) do { } while (0)

/*------------------------------------------------------------------------*/
#endif  // end of 'else' part of 'ifdef LOGGING'
/*------------------------------------------------------------------------*/

#define TRACE(...) \
do { \
  if ((this == 0)) break; \
  if ((internal == 0)) break; \
  LOG_API_CALL_BEGIN (__VA_ARGS__); \
  if (!trace_api_file) break; \
  trace_api_call (__VA_ARGS__); \
} while (0)

void Solver::trace_api_call (const char * s0) const {
  assert (trace_api_file);
  LOG ("TRACE %s", s0);
  fprintf (trace_api_file, "%s\n", s0);
  fflush (trace_api_file);
}

void Solver::trace_api_call (const char * s0, int i1) const {
  assert (trace_api_file);
  LOG ("TRACE %s %d", s0, i1);
  fprintf (trace_api_file, "%s %d\n", s0, i1);
  fflush (trace_api_file);
}

void
Solver::trace_api_call (const char * s0, const char * s1, int i2) const {
  assert (trace_api_file);
  LOG ("TRACE %s %s %d", s0, s1, i2);
  fprintf (trace_api_file, "%s %s %d\n", s0, s1, i2);
  fflush (trace_api_file);
}

/*------------------------------------------------------------------------*/

// The global 'tracing_api' flag is used to ensure that only one solver
// traces to a file.  Otherwise the method to use an environment variable to
// point to the trace file is bogus, since those different solver instances
// would all write to the same file producing garbage.  A more sophisticated
// solution would use a different mechanism to tell the solver to which file
// to trace to, but in our experience it is quite convenient to get traces
// out of applications which use the solver as library by just setting an
// environment variable without requiring to change any application code.
//
static bool tracing_api_calls_through_environment_variable_method;

Solver::Solver () {

  const char * path = getenv ("CADICAL_API_TRACE");
  if (!path) path = getenv ("CADICALAPITRACE");
  if (path) {
    if (tracing_api_calls_through_environment_variable_method)
      FATAL ("can not trace API calls of two solver instances "
        "using environment variable 'CADICAL_API_TRACE'");
    if (!(trace_api_file = fopen (path, "w")))
      FATAL ("failed to open file '%s' to trace API calls "
        "using environment variable 'CADICAL_API_TRACE'", path);
    close_trace_api_file = true;
    tracing_api_calls_through_environment_variable_method = true;
  } else {
    tracing_api_calls_through_environment_variable_method = false;
    close_trace_api_file = false;
    trace_api_file = 0;
  }

  _state = INITIALIZING;
  internal = new Internal ();
  TRACE ("init");
  external = new External (internal);
  STATE (CONFIGURING);
  if (tracing_api_calls_through_environment_variable_method)
    message ("tracing API calls to '%s'", path);
}

Solver::~Solver () {

  TRACE ("reset");
  REQUIRE_VALID_OR_SOLVING_STATE ();
  STATE (DELETING);

#ifdef LOGGING
  //
  // After deleting 'internal' logging does not work anymore.
  //
  bool logging = internal->opts.log;
  int level = internal->level;
  string prefix = internal->prefix;
#endif

  delete internal;
  delete external;

  if (close_trace_api_file) {
    close_trace_api_file = false;
    assert (trace_api_file);
    assert (tracing_api_calls_through_environment_variable_method);
    fclose (trace_api_file);
    tracing_api_calls_through_environment_variable_method = false;
  }

#ifdef LOGGING
  //
  // Need to log success of this API call manually.
  //
  if (logging) {
    printf ("%s%sLOG %s%d%s API call %s'reset ()'%s succeeded%s\n",
      prefix.c_str (),
      tout.log_code (),
      tout.emph_code (),
      level,
      tout.log_code (),
      tout.api_code (),
      tout.log_code (),
      tout.normal_code ());
    fflush (stdout);
  }
#endif
}

/*------------------------------------------------------------------------*/

int Solver::vars () {
  TRACE ("vars");
  REQUIRE_VALID_OR_SOLVING_STATE ();
  int res = external->max_var;
  LOG_API_CALL_RETURNS ("vars", res);
  return res;
}

void Solver::reserve (int min_max_var) {
  TRACE ("reserve", min_max_var);
  REQUIRE_VALID_STATE ();
  transition_to_unknown_state ();
  external->reset_extended ();
  external->init (min_max_var);
  LOG_API_CALL_END ("reserve", min_max_var);
}

/*------------------------------------------------------------------------*/

void Solver::trace_api_calls (FILE * file) {
  LOG_API_CALL_BEGIN ("trace_api_calls");
  REQUIRE_VALID_STATE ();
  REQUIRE (file != 0, "invalid zero file argument");
  REQUIRE (!tracing_api_calls_through_environment_variable_method,
    "already tracing API calls "
    "using environment variable 'CADICAL_API_TRACE'");
  REQUIRE (!trace_api_file, "called twice");
  trace_api_file = file;
  LOG_API_CALL_END ("trace_api_calls");
  trace_api_call ("init");
}

/*------------------------------------------------------------------------*/

bool Solver::is_valid_option (const char * name) {
  return Options::has (name);
}

bool Solver::is_valid_long_option (const char * arg) {
  string name;
  int tmp;
  return Options::parse_long_option (arg, name, tmp);
}

int Solver::get (const char * arg) {
  REQUIRE_VALID_OR_SOLVING_STATE ();
  return internal->opts.get (arg);
}

void Solver::set_total_instances (int val) {
  TRACE ("set_total_instances", val);
  REQUIRE_VALID_STATE ();
  internal->total_instances = val;
  LOG_API_CALL_END ("set_total_instances", val);
}

void Solver::set_instance_num (int val) {
  TRACE ("set_instance_num", val);
  REQUIRE_VALID_STATE ();
  internal->instance_num = val;
  LOG_API_CALL_END ("set_instance_num", val);
}

bool Solver::set (const char * arg, int val) {
  TRACE ("set", arg, val);
  REQUIRE_VALID_STATE ();
  if (strcmp (arg, "log") &&
      strcmp (arg, "quiet") &&
      strcmp (arg, "verbose")) {
    REQUIRE (state () == CONFIGURING,
      "can only set option 'set (\"%s\", %d)' right after initialization",
      arg, val);
  }
  bool res = internal->opts.set (arg, val);
  LOG_API_CALL_END ("set", arg, val, res);
  return res;
}

bool Solver::set_long_option (const char * arg) {
  LOG_API_CALL_BEGIN ("set", arg);
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
    "can only set option '%s' right after initialization", arg);
  bool res;
  if (arg[0] != '-' || arg[1] != '-') res = false;
  else {
    int val;
    string name;
    res = Options::parse_long_option (arg, name, val);
    if (res) set (name.c_str (), val);
  }
  LOG_API_CALL_END ("set", arg, res);
  return res;
}

void Solver::optimize (int arg) {
  LOG_API_CALL_BEGIN ("optimize", arg);
  REQUIRE_VALID_STATE ();
  internal->opts.optimize (arg);
  LOG_API_CALL_END ("optimize", arg);
}

bool Solver::limit (const char * arg, int val) {
  TRACE ("limit", arg, val);
  REQUIRE_VALID_STATE ();
  bool res = internal->limit (arg, val);
  LOG_API_CALL_END ("limit", arg, val, res);
  return res;
}

bool Solver::is_valid_limit (const char * arg) {
  return Internal::is_valid_limit (arg);
}

void Solver::prefix (const char * str) {
  LOG_API_CALL_BEGIN ("prefix", str);
  REQUIRE_VALID_STATE ();
  internal->prefix = str;
  LOG_API_CALL_END ("prefix", str);
}

bool Solver::is_valid_configuration (const char * name) {
  return Config::has (name);
}

bool Solver::configure (const char * name) {
  LOG_API_CALL_BEGIN ("config", name);
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
    "can only set configuration '%s' right after initialization", name);
  bool res = Config::set (*this, name);
  LOG_API_CALL_END ("config", name, res);
  return res;
}

/*===== IPASIR BEGIN =====================================================*/

void Solver::add (int lit) {
  TRACE ("add", lit);
  REQUIRE_VALID_STATE ();
  if (lit) REQUIRE_VALID_LIT (lit);
  transition_to_unknown_state ();
  external->add (lit);
  if (lit) STATE (ADDING);
  else     STATE (UNKNOWN);
  LOG_API_CALL_END ("add", lit);
}

void Solver::assume (int lit) {
  TRACE ("assume", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  transition_to_unknown_state ();
  external->assume (lit);
  LOG_API_CALL_END ("assume", lit);
}

/*------------------------------------------------------------------------*/

int Solver::call_external_solve_and_check_results () {
  transition_to_unknown_state ();
  assert (state () & READY);
  STATE (SOLVING);
  int res = external->solve ();
       if (res == 10) STATE (SATISFIED);
  else if (res == 20) STATE (UNSATISFIED);
  else                STATE (UNKNOWN);
#if 0 // EXPENSIVE ALTERNATIVE ASSUMPTION CHECKING
  // This checks that the set of failed assumptions form a core using the
  // external 'copy (...)' function to copy the solver, which can be trusted
  // less, since it involves copying the extension stack too.  The
  // 'External::check_assumptions_failing' is a better alternative and can
  // be enabled by options too.  We keep this code though to have an
  // alternative failed assumption checking available for debugging.
  //
  if (res == 20 && !external->assumptions.empty ()) {
    Solver checker;
    checker.prefix ("checker ");
    copy (checker);
    for (const auto & lit : external->assumptions)
      if (failed (lit))
        checker.add (lit), checker.add (0);
    if (checker.solve () != 20)
      FATAL ("copying assumption checker failed");
  }
#endif
  if (!res) external->reset_assumptions ();
  return res;
}

int Solver::solve () {
  TRACE ("solve");
  REQUIRE_VALID_STATE ();
  REQUIRE (state () != ADDING,
    "clause incomplete (terminating zero not added)");
  int res = call_external_solve_and_check_results ();
  LOG_API_CALL_RETURNS ("solve", res);
  return res;
}

int Solver::simplify (int rounds) {
  TRACE ("simplify", rounds);
  REQUIRE_VALID_STATE ();
  REQUIRE (rounds >= 0,
    "negative number of simplification rounds '%d'", rounds);
  REQUIRE (state () != ADDING,
    "clause incomplete (terminating zero not added)");
  internal->limit ("conflicts", 0);
  internal->limit ("preprocessing", rounds);
  int res = call_external_solve_and_check_results ();
  LOG_API_CALL_RETURNS ("simplify", rounds, res);
  return res;
}

/*------------------------------------------------------------------------*/

int Solver::val (int lit) {
  TRACE ("val", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  REQUIRE (state () == SATISFIED,
    "can only get value in satisfied state");
  int res = external->ival (lit);
  LOG_API_CALL_RETURNS ("val", lit, res);
  return res;
}

bool Solver::failed (int lit) {
  TRACE ("failed", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  REQUIRE (state () == UNSATISFIED,
    "can only get failed assumptions in unsatisfied state");
  bool res = external->failed (lit);
  LOG_API_CALL_RETURNS ("failed", lit, res);
  return res;
}

int Solver::fixed (int lit) const {
  TRACE ("fixed", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  int res = external->fixed (lit);
  LOG_API_CALL_RETURNS ("fixed", lit, res);
  return res;
}

/*------------------------------------------------------------------------*/

void Solver::terminate () {
  LOG_API_CALL_BEGIN ("terminate");
  REQUIRE_VALID_OR_SOLVING_STATE ();
  external->terminate ();
  LOG_API_CALL_END ("terminate");
}

void Solver::connect_terminator (Terminator * terminator) {
  LOG_API_CALL_BEGIN ("connect_terminator");
  REQUIRE_VALID_STATE ();
  REQUIRE (terminator, "can not connect zero terminator");
#ifdef LOGGING
  if (external->terminator)
    LOG ("connecting new terminator (disconnecting previous one)");
  else
    LOG ("connecting new terminator (no previous one)");
#endif
  external->terminator = terminator;
  LOG_API_CALL_END ("connect_terminator");
}

void Solver::disconnect_terminator () {
  LOG_API_CALL_BEGIN ("disconnect_terminator");
  REQUIRE_VALID_STATE ();
#ifdef LOGGING
    if (external->terminator)
      LOG ("disconnecting previous terminator");
    else
      LOG ("ignoring to disconnect terminator (no previous one)");
#endif
  external->terminator = 0;
  LOG_API_CALL_END ("disconnect_terminator");
}

/*===== IPASIR END =======================================================*/

int Solver::active () const {
  TRACE ("active");
  REQUIRE_VALID_STATE ();
  int res = internal->active ();
  LOG_API_CALL_RETURNS ("active", res);
  return res;
}

int64_t Solver::redundant () const {
  TRACE ("redundant");
  REQUIRE_VALID_STATE ();
  int64_t res = internal->redundant ();
  LOG_API_CALL_RETURNS ("redundant", res);
  return res;
}

int64_t Solver::irredundant () const {
  TRACE ("irredundant");
  REQUIRE_VALID_STATE ();
  int64_t res = internal->irredundant ();
  LOG_API_CALL_RETURNS ("irredundant", res);
  return res;
}

/*------------------------------------------------------------------------*/

void Solver::freeze (int lit) {
  TRACE ("freeze", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  external->freeze (lit);
  LOG_API_CALL_END ("freeze", lit);
}

void Solver::melt (int lit) {
  TRACE ("melt", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  REQUIRE (external->frozen (lit),
    "can not melt completely melted literal '%d'", lit);
  external->melt (lit);
  LOG_API_CALL_END ("melt", lit);
}

bool Solver::frozen (int lit) const {
  TRACE ("frozen", lit);
  REQUIRE_VALID_STATE ();
  REQUIRE_VALID_LIT (lit);
  bool res = external->frozen (lit);
  LOG_API_CALL_RETURNS ("frozen", lit, res);
  return res;
}

/*------------------------------------------------------------------------*/

bool Solver::trace_proof (FILE * external_file, const char * name) {
  LOG_API_CALL_BEGIN ("trace_proof", name);
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
    "can only start proof tracing to '%s' right after initialization",
    name);
  REQUIRE (!internal->tracer, "already tracing proof");
  File * internal_file = File::write (internal, external_file, name);
  assert (internal_file);
  internal->trace (internal_file);
  LOG_API_CALL_RETURNS ("trace_proof", name, true);
  return true;
}

bool Solver::trace_proof (const char * path) {
  LOG_API_CALL_BEGIN ("trace_proof", path);
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
    "can only start proof tracing to '%s' right after initialization",
    path);
  REQUIRE (!internal->tracer, "already tracing proof");
  File * internal_file = File::write (internal, path);
  bool res = (internal_file != 0);
  internal->trace (internal_file);
  LOG_API_CALL_RETURNS ("trace_proof", path, res);
  return res;
}

void Solver::flush_proof_trace () {
  LOG_API_CALL_BEGIN ("flush_proof_trace");
  REQUIRE_VALID_STATE ();
  REQUIRE (internal->tracer, "proof is not traced");
  REQUIRE (!internal->tracer->closed (), "proof trace already closed");
  internal->flush_trace ();
  LOG_API_CALL_END ("flush_proof_trace");
}

void Solver::close_proof_trace () {
  LOG_API_CALL_BEGIN ("close_proof_trace");
  REQUIRE_VALID_STATE ();
  REQUIRE (internal->tracer, "proof is not traced");
  REQUIRE (!internal->tracer->closed (), "proof trace already closed");
  internal->close_trace ();
  LOG_API_CALL_END ("close_proof_trace");
}

/*------------------------------------------------------------------------*/

void Solver::build (FILE * file, const char * prefix) {

  assert (file == stdout || file == stderr);

  Terminal * terminal;

  if (file == stdout) terminal = &tout;
  else if (file == stderr) terminal = &terr;
  else terminal = 0;

  const char * v = CaDiCaL::version ();
  const char * i = identifier ();
  const char * c = compiler ();
  const char * b = date ();
  const char * f = flags ();

  assert (v);

  fputs (prefix, file);
  if (terminal) terminal->magenta ();
  fputs ("Version ", file);
  if (terminal) terminal->normal ();
  fputs (v, file);
  if (i) {
    if (terminal) terminal->magenta ();
    fputc (' ', file);
    fputs (i, file);
    if (terminal) terminal->normal ();
  }
  fputc ('\n', file);

  if (c) {
    fputs (prefix, file);
    if (terminal) terminal->magenta ();
    fputs (c, file);
    if (f) {
      fputc (' ', file);
      fputs (f, file);
    }
    if (terminal) terminal->normal ();
    fputc ('\n', file);
  }

  if (b) {
    fputs (prefix, file);
    if (terminal) terminal->magenta ();
    fputs (b, file);
    if (terminal) terminal->normal ();
    fputc ('\n', file);
  }

  fflush (file);
}

const char * Solver::version () { return CaDiCaL::version (); }

const char * Solver::signature () { return CaDiCaL::signature (); }

void Solver::options () {
  REQUIRE_VALID_STATE ();
  internal->opts.print ();
}

void Solver::usage () { Options::usage (); }

void Solver::configurations () { Config::usage (); }

void Solver::statistics () {
  if (state () == DELETING) return;
  TRACE ("stats");
  REQUIRE_VALID_OR_SOLVING_STATE ();
  internal->print_stats ();
  LOG_API_CALL_END ("stats");
}

/*------------------------------------------------------------------------*/

const char * Solver::read_dimacs (File * file, int & vars, int strict) {
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
    "can only read DIMACS file right after initialization");
  Parser * parser = new Parser (this, file);
  const char * err = parser->parse_dimacs (vars, strict);
  delete parser;
  return err;
}

const char *
Solver::read_dimacs (FILE * external_file,
                     const char * name, int & vars, int strict) {
  LOG_API_CALL_BEGIN ("read_dimacs", name);
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
    "can only read DIMACS file right after initialization");
  File * file = File::read (internal, external_file, name);
  assert (file);
  const char * err = read_dimacs (file, vars, strict);
  delete file;
  LOG_API_CALL_RETURNS ("read_dimacs", name, err);
  return err;
}

const char *
Solver::read_dimacs (const char * path, int & vars, int strict) {
  LOG_API_CALL_BEGIN ("read_dimacs", path);
  REQUIRE_VALID_STATE ();
  REQUIRE (state () == CONFIGURING,
    "can only read DIMACS file right after initialization");
  File * file = File::read (internal, path);
  if (!file)
    return internal->error_message.init (
             "failed to read DIMACS file '%s'", path);
  const char * err = read_dimacs (file, vars, strict);
  delete file;
  LOG_API_CALL_RETURNS ("read_dimacs", path, err);
  return err;
}

const char * Solver::read_solution (const char * path) {
  LOG_API_CALL_BEGIN ("solution", path);
  REQUIRE_VALID_STATE ();
  File * file = File::read (internal, path);
  if (!file) return internal->error_message.init (
                      "failed to read solution file '%s'", path);
  Parser * parser = new Parser (this, file);
  const char * err = parser->parse_solution ();
  delete parser;
  delete file;
  if (!err) external->check_assignment (&External::sol);
  LOG_API_CALL_RETURNS ("read_solution", path, err);
  return err;
}

/*------------------------------------------------------------------------*/

void Solver::dump_cnf () {
  TRACE ("dump");
  REQUIRE_INITIALIZED ();
  internal->dump ();
  LOG_API_CALL_END ("dump");
}

/*------------------------------------------------------------------------*/

bool Solver::traverse_clauses (ClauseIterator & it) const {
  LOG_API_CALL_BEGIN ("traverse_clauses");
  REQUIRE_VALID_STATE ();
  bool res = external->traverse_all_frozen_units_as_clauses (it) &&
             internal->traverse_clauses (it);
  LOG_API_CALL_RETURNS ("traverse_clauses", res);
  return res;
}

bool Solver::traverse_witnesses_backward (WitnessIterator & it) const {
  LOG_API_CALL_BEGIN ("traverse_witnesses_backward");
  REQUIRE_VALID_STATE ();
  bool res = external->traverse_all_non_frozen_units_as_witnesses (it) &&
             external->traverse_witnesses_backward (it);
  LOG_API_CALL_RETURNS ("traverse_witnesses_backward", res);
  return res;
}

bool Solver::traverse_witnesses_forward (WitnessIterator & it) const {
  LOG_API_CALL_BEGIN ("traverse_witnesses_forward");
  REQUIRE_VALID_STATE ();
  bool res = external->traverse_witnesses_forward (it) &&
             external->traverse_all_non_frozen_units_as_witnesses (it);
  LOG_API_CALL_RETURNS ("traverse_witnesses_forward", res);
  return res;
}

/*------------------------------------------------------------------------*/

class ClauseCounter : public ClauseIterator {
public:
  int vars;
  int64_t clauses;
  ClauseCounter () : vars (0), clauses (0) { }
  bool clause (const vector<int> & c) {
    for (const auto & lit : c) {
      assert (lit != INT_MIN);
      int idx = abs (lit);
      if (idx > vars) vars = idx;
    }
    clauses++;
    return true;
  }
};

class ClauseWriter : public ClauseIterator {
  File * file;
public:
  ClauseWriter (File * f) : file (f) { }
  bool clause (const vector<int> & c) {
    for (const auto & lit : c) {
      if (!file->put (lit)) return false;
      if (!file->put (' ')) return false;
    }
    return file->put ("0\n");
  }
};

const char * Solver::write_dimacs (const char * path, int min_max_var) {
  LOG_API_CALL_BEGIN ("write_dimacs", path, min_max_var);
  REQUIRE_VALID_STATE ();
  ClauseCounter counter;
  (void) traverse_clauses (counter);
  LOG ("found maximal variable %d and %" PRId64 " clauses",
    counter.vars, counter.clauses);
#ifndef QUIET
  const double start = internal->time ();
#endif
  File * file = File::write (internal, path);
  const char * res = 0;
  if (file) {
    int actual_max_vars = max (min_max_var, counter.vars);
    MSG ("writing %s'p cnf %d %" PRId64 "'%s header",
      tout.green_code (), actual_max_vars, counter.clauses,
      tout.normal_code ());
    file->put ("p cnf ");
    file->put (actual_max_vars);
    file->put (' ');
    file->put (counter.clauses);
    file->put ('\n');
    ClauseWriter writer (file);
    if (!traverse_clauses (writer))
      res = internal->error_message.init (
              "writing to DIMACS file '%s' failed", path);
    delete file;
  } else res = internal->error_message.init (
                 "failed to open DIMACS file '%s' for writing", path);
#ifndef QUIET
  if (!res) {
    const double end = internal->time ();
    MSG ("wrote %" PRId64 " clauses in %.2f seconds %s time",
      counter.clauses, end - start,
      internal->opts.realtime ? "real" : "process");
  }
#endif
  LOG_API_CALL_RETURNS ("write_dimacs", path, min_max_var, res);
  return res;
}

/*------------------------------------------------------------------------*/

struct WitnessWriter : public WitnessIterator {
  File * file;
  int64_t witnesses;
  WitnessWriter (File * f) : file (f), witnesses (0) { }
  bool write (const vector<int> & a) {
    for (const auto & lit : a) {
      if (!file->put (lit)) return false;
      if (!file->put (' ')) return false;
    }
    return file->put ('0');
  }
  bool witness (const vector<int> & c, const vector<int> & w) {
    if (!write (c)) return false;
    if (!file->put (' ')) return false;
    if (!write (w)) return false;
    if (!file->put ('\n')) return false;
    witnesses++;
    return true;
  }
};

const char * Solver::write_extension (const char * path) {
  LOG_API_CALL_BEGIN ("write_extension", path);
  REQUIRE_VALID_STATE ();
  const char * res = 0;
#ifndef QUIET
  const double start = internal->time ();
#endif
  File * file = File::write (internal, path);
  WitnessWriter writer (file);
  if (file) {
    if (!traverse_witnesses_backward (writer))
      res = internal->error_message.init (
              "writing to DIMACS file '%s' failed", path);
    delete file;
  } else res = internal->error_message.init (
                 "failed to open extension file '%s' for writing", path);
#ifndef QUIET
  if (!res) {
    const double end = internal->time ();
    MSG ("wrote %" PRId64 " witnesses in %.2f seconds %s time",
      writer.witnesses, end - start,
      internal->opts.realtime ? "real" : "process");
  }
#endif
  LOG_API_CALL_RETURNS ("write_extension", path, res);
  return res;
}

/*------------------------------------------------------------------------*/

struct ClauseCopier : public ClauseIterator {
  Solver & dst;
public:
  ClauseCopier (Solver & d) : dst (d) { }
  bool clause (const vector<int> & c) {
    for (const auto & lit : c)
      dst.add (lit);
    dst.add (0);
    return true;
  }
};

struct WitnessCopier : public WitnessIterator {
  External * dst;
public:
  WitnessCopier (External * d) : dst (d) { }
  bool witness (const vector<int> & c, const vector<int> & w) {
    dst->push_external_clause_and_witness_on_extension_stack (c, w);
    return true;
  }
};

void Solver::copy (Solver & other) const {
  ClauseCopier clause_copier (other);
  traverse_clauses (clause_copier);
  WitnessCopier witness_copier (other.external);
  traverse_witnesses_forward (witness_copier);
}

/*------------------------------------------------------------------------*/

void Solver::section (const char * title) {
  if (state () == DELETING) return;
#ifdef QUIET
  (void) title;
#endif
  REQUIRE_INITIALIZED ();
  SECTION (title);
}

void Solver::message (const char * fmt, ...) {
  if (state () == DELETING) return;
#ifdef QUIET
  (void) fmt;
#else
  REQUIRE_INITIALIZED ();
  va_list ap;
  va_start (ap, fmt);
  internal->vmessage (fmt, ap);
  va_end (ap);
#endif
}

void Solver::message () {
  if (state () == DELETING) return;
  REQUIRE_INITIALIZED ();
#ifndef QUIET
  internal->message ();
#endif
}

void Solver::verbose (int level, const char * fmt, ...) {
  if (state () == DELETING) return;
  REQUIRE_VALID_OR_SOLVING_STATE ();
#ifdef QUIET
  (void) level;
  (void) fmt;
#else
  va_list ap;
  va_start (ap, fmt);
  internal->vverbose (level, fmt, ap);
  va_end (ap);
#endif
}

void Solver::error (const char * fmt, ...) {
  if (state () == DELETING) return;
  REQUIRE_INITIALIZED ();
  va_list ap;
  va_start (ap, fmt);
  internal->verror (fmt, ap);
  va_end (ap);
}

}
