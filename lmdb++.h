/* This is free and unencumbered software released into the public domain. */

#ifndef LMDBXX_H
#define LMDBXX_H

/**
 * <lmdb++.h> - C++11 wrapper for LMDB.
 *
 * @author Arto Bendiken <arto@bendiken.net>
 * @see https://sourceforge.net/projects/lmdbxx/
 */

#ifndef __cplusplus
#error "<lmdb++.h> requires a C++ compiler"
#endif

#if __cplusplus < 201103L
#error "<lmdb++.h> requires a C++11 compiler (CXXFLAGS='-std=c++11')"
#endif

////////////////////////////////////////////////////////////////////////////////

#include <lmdb.h>      /* for MDB_*, mdb_*() */

#if 1
#include <cassert>     /* for assert() */
#endif
#include <cstddef>     /* for std::size_t */
#include <cstdio>      /* for std::snprintf() */
#include <cstring>     /* for std::strlen() */
#include <stdexcept>   /* for std::runtime_error */
#include <string>      /* for std::string */
#include <type_traits> /* for std::is_pod<> */

namespace lmdb {
  using mode = mdb_mode_t;
}

////////////////////////////////////////////////////////////////////////////////
/* Error Handling */

namespace lmdb {
  class error;
  class logic_error;
  class fatal_error;
  class runtime_error;
  class key_exist_error;
  class not_found_error;
  class corrupted_error;
  class panic_error;
}

/**
 * Base class for LMDB exception conditions.
 *
 * @see http://symas.com/mdb/doc/group__errors.html
 */
class lmdb::error : public std::runtime_error {
protected:
  const int _code;

public:
  /**
   * Throws an error based on the given LMDB return code.
   */
  [[noreturn]] static inline void raise(const char* origin, int rc);

  /**
   * Constructor.
   */
  error(const char* const origin,
        const int rc) noexcept
    : runtime_error{origin},
      _code{rc} {}

  /**
   * Returns the underlying LMDB error code.
   */
  int code() const noexcept {
    return _code;
  }

  /**
   * Returns the origin of the LMDB error.
   */
  const char* origin() const noexcept {
    return runtime_error::what();
  }

  /**
   * Returns the underlying LMDB error code.
   */
  virtual const char* what() const noexcept {
    static thread_local char buffer[1024];
    std::snprintf(buffer, sizeof(buffer),
      "%s: %s", origin(), ::mdb_strerror(code()));
    return buffer;
  }
};

/**
 * Base class for logic error conditions.
 */
class lmdb::logic_error : public lmdb::error {
public:
  using error::error;
};

/**
 * Base class for fatal error conditions.
 */
class lmdb::fatal_error : public lmdb::error {
public:
  using error::error;
};

/**
 * Base class for runtime error conditions.
 */
class lmdb::runtime_error : public lmdb::error {
public:
  using error::error;
};

/**
 * Exception class for `MDB_KEYEXIST` errors.
 *
 * @see http://symas.com/mdb/doc/group__errors.html#ga05dc5bbcc7da81a7345bd8676e8e0e3b
 */
class lmdb::key_exist_error final : public lmdb::runtime_error {
public:
  using runtime_error::runtime_error;
};

/**
 * Exception class for `MDB_NOTFOUND` errors.
 *
 * @see http://symas.com/mdb/doc/group__errors.html#gabeb52e4c4be21b329e31c4add1b71926
 */
class lmdb::not_found_error final : public lmdb::runtime_error {
public:
  using runtime_error::runtime_error;
};

/**
 * Exception class for `MDB_CORRUPTED` errors.
 *
 * @see http://symas.com/mdb/doc/group__errors.html#gaf8148bf1b85f58e264e57194bafb03ef
 */
class lmdb::corrupted_error final : public lmdb::fatal_error {
public:
  using fatal_error::fatal_error;
};

/**
 * Exception class for `MDB_PANIC` errors.
 *
 * @see http://symas.com/mdb/doc/group__errors.html#gae37b9aedcb3767faba3de8c1cf6d3473
 */
class lmdb::panic_error final : public lmdb::fatal_error {
public:
  using fatal_error::fatal_error;
};

inline void
lmdb::error::raise(const char* const origin,
                   const int rc) {
  switch (rc) {
    case MDB_KEYEXIST:  throw key_exist_error{origin, rc};
    case MDB_NOTFOUND:  throw not_found_error{origin, rc};
    case MDB_CORRUPTED: throw corrupted_error{origin, rc};
    case MDB_PANIC:     throw panic_error{origin, rc};
    default: throw lmdb::runtime_error{origin, rc};
  }
}

////////////////////////////////////////////////////////////////////////////////
/* Procedural Interface: Metadata */

namespace lmdb {
  // TODO: mdb_version()
  // TODO: mdb_strerror()
}

////////////////////////////////////////////////////////////////////////////////
/* Procedural Interface: Environment */

namespace lmdb {
  static inline void env_create(MDB_env** env);
  static inline void env_open(MDB_env* env,
    const char* path, unsigned int flags, mode mode);
  // TODO: mdb_env_copy()
  // TODO: mdb_env_copyfd()
  // TODO: mdb_env_copy2()
  // TODO: mdb_env_copyfd2()
  // TODO: mdb_env_stat()
  // TODO: mdb_env_info()
  static inline void env_sync(MDB_env* env, bool force);
  static inline void env_close(MDB_env* env) noexcept;
  static inline void env_set_flags(MDB_env* env, unsigned int flags, bool onoff);
  // TODO: mdb_env_get_flags()
  // TODO: mdb_env_get_path()
  // TODO: mdb_env_get_fd()
  static inline void env_set_map_size(MDB_env* env, std::size_t size);
  static inline void env_set_max_readers(MDB_env* env, unsigned int count);
  // TODO: mdb_env_get_maxreaders()
  static inline void env_set_max_dbs(MDB_env* env, MDB_dbi count);
  // TODO: mdb_env_get_maxkeysize()
  // TODO: mdb_env_set_userctx()
  // TODO: mdb_env_get_userctx()
  // TODO: mdb_env_set_assert()
  // TODO: mdb_reader_list()
  // TODO: mdb_reader_check()
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_create(MDB_env** env) {
  const int rc = ::mdb_env_create(env);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_create", rc);
  }
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_open(MDB_env* env,
               const char* const path,
               const unsigned int flags,
               const mode mode) {
  const int rc = ::mdb_env_open(env, path, flags, mode);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_open", rc);
  }
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_sync(MDB_env* env,
               const bool force = true) {
  const int rc = ::mdb_env_sync(env, force);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_sync", rc);
  }
}

/**
 * @see http://symas.com/mdb/doc/group__mdb.html#ga4366c43ada8874588b6a62fbda2d1e95
 */
static inline void
lmdb::env_close(MDB_env* env) noexcept {
  ::mdb_env_close(env);
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_set_flags(MDB_env* env,
                    const unsigned int flags,
                    const bool onoff = true) {
  const int rc = ::mdb_env_set_flags(env, flags, onoff ? 1 : 0);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_set_flags", rc);
  }
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_set_map_size(MDB_env* env,
                       const std::size_t size) {
  const int rc = ::mdb_env_set_mapsize(env, size);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_set_mapsize", rc);
  }
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_set_max_readers(MDB_env* env,
                          const unsigned int count) {
  const int rc = ::mdb_env_set_maxreaders(env, count);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_set_maxreaders", rc);
  }
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_set_max_dbs(MDB_env* env,
                      const MDB_dbi count) {
  const int rc = ::mdb_env_set_maxdbs(env, count);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_set_maxdbs", rc);
  }
}

////////////////////////////////////////////////////////////////////////////////
/* Procedural Interface: Transactions */

namespace lmdb {
  static inline void txn_begin(
    MDB_env* env, MDB_txn* parent, unsigned int flags, MDB_txn** txn);
  static inline MDB_env* txn_env(MDB_txn* const txn) noexcept;
  static inline void txn_commit(MDB_txn* txn);
  static inline void txn_abort(MDB_txn* txn) noexcept;
  static inline void txn_reset(MDB_txn* txn) noexcept;
  static inline void txn_renew(MDB_txn* txn);
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#gad7ea55da06b77513609efebd44b26920
 */
static inline void
lmdb::txn_begin(MDB_env* const env,
                MDB_txn* const parent,
                const unsigned int flags,
                MDB_txn** txn) {
  const int rc = ::mdb_txn_begin(env, parent, flags, txn);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_txn_begin", rc);
  }
}

/**
 * @see http://symas.com/mdb/doc/group__mdb.html#gaeb17735b8aaa2938a78a45cab85c06a0
 */
static inline MDB_env*
lmdb::txn_env(MDB_txn* const txn) noexcept {
  return ::mdb_txn_env(txn);
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#ga846fbd6f46105617ac9f4d76476f6597
 */
static inline void
lmdb::txn_commit(MDB_txn* const txn) {
  const int rc = ::mdb_txn_commit(txn);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_txn_commit", rc);
  }
}

/**
 * @see http://symas.com/mdb/doc/group__mdb.html#ga73a5938ae4c3239ee11efa07eb22b882
 */
static inline void
lmdb::txn_abort(MDB_txn* const txn) noexcept {
  ::mdb_txn_abort(txn);
}

/**
 * @see http://symas.com/mdb/doc/group__mdb.html#ga02b06706f8a66249769503c4e88c56cd
 */
static inline void
lmdb::txn_reset(MDB_txn* const txn) noexcept {
  ::mdb_txn_reset(txn);
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#ga6c6f917959517ede1c504cf7c720ce6d
 */
static inline void
lmdb::txn_renew(MDB_txn* const txn) {
  const int rc = ::mdb_txn_renew(txn);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_txn_renew", rc);
  }
}

////////////////////////////////////////////////////////////////////////////////
/* Procedural Interface: Databases */

namespace lmdb {
  static inline void dbi_open(
    MDB_txn* txn, const char* name, unsigned int flags, MDB_dbi* dbi);
  static inline void dbi_stat(MDB_txn* txn, MDB_dbi dbi, MDB_stat* stat);
  static inline void dbi_flags(MDB_txn* txn, MDB_dbi dbi, unsigned int* flags);
  static inline void dbi_close(MDB_env* env, MDB_dbi dbi) noexcept;
  // TODO: mdb_drop()
  // TODO: mdb_set_compare()
  // TODO: mdb_set_dupsort()
  // TODO: mdb_set_relfunc()
  // TODO: mdb_set_relctx()
  static inline bool dbi_get(MDB_txn* txn, MDB_dbi dbi, MDB_val* key, MDB_val* data);
  // TODO: mdb_put()
  // TODO: mdb_del()
  // TODO: mdb_cmp()
  // TODO: mdb_dcmp()
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#gac08cad5b096925642ca359a6d6f0562a
 */
static inline void
lmdb::dbi_open(MDB_txn* const txn,
               const char* const name,
               const unsigned int flags,
               MDB_dbi* const dbi) {
  const int rc = ::mdb_dbi_open(txn, name, flags, dbi);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_dbi_open", rc);
  }
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#gae6c1069febe94299769dbdd032fadef6
 */
static inline void
lmdb::dbi_stat(MDB_txn* const txn,
               const MDB_dbi dbi,
               MDB_stat* const result) {
  const int rc = ::mdb_stat(txn, dbi, result);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_stat", rc);
  }
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#ga95ba4cb721035478a8705e57b91ae4d4
 */
static inline void
lmdb::dbi_flags(MDB_txn* const txn,
                const MDB_dbi dbi,
                unsigned int* const flags) {
  const int rc = ::mdb_dbi_flags(txn, dbi, flags);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_dbi_flags", rc);
  }
}

/**
 * @see http://symas.com/mdb/doc/group__mdb.html#ga52dd98d0c542378370cd6b712ff961b5
 */
static inline void
lmdb::dbi_close(MDB_env* const env,
                const MDB_dbi dbi) noexcept {
  ::mdb_dbi_close(env, dbi);
}

/**
 * @see http://symas.com/mdb/doc/group__mdb.html#ga8bf10cd91d3f3a83a34d04ce6b07992d
 */
static inline bool
lmdb::dbi_get(MDB_txn* const txn,
              const MDB_dbi dbi,
              MDB_val* const key,
              MDB_val* const data) {
  const int rc = ::mdb_get(txn, dbi, key, data);
  if (rc != MDB_SUCCESS && rc != MDB_NOTFOUND) {
    error::raise("mdb_get", rc);
  }
  return (rc == MDB_SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////
/* Procedural Interface: Cursors */

namespace lmdb {
  static inline void cursor_open(MDB_txn* txn, MDB_dbi dbi, MDB_cursor** cursor);
  static inline void cursor_close(MDB_cursor* cursor) noexcept;
  static inline void cursor_renew(MDB_txn* txn, MDB_cursor* cursor);
  static inline MDB_txn* cursor_txn(MDB_cursor* cursor) noexcept;
  static inline MDB_dbi cursor_dbi(MDB_cursor* cursor) noexcept;
  static inline bool cursor_get(MDB_cursor* cursor, MDB_val* key, MDB_val* data, MDB_cursor_op op);
  static inline void cursor_put(MDB_cursor* cursor, MDB_val* key, MDB_val* data, unsigned int flags);
  static inline void cursor_del(MDB_cursor* cursor, unsigned int flags);
  static inline void cursor_count(MDB_cursor* cursor, std::size_t& count);
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#ga9ff5d7bd42557fd5ee235dc1d62613aa
 */
static inline void
lmdb::cursor_open(MDB_txn* const txn,
                  const MDB_dbi dbi,
                  MDB_cursor** const cursor) {
  const int rc = ::mdb_cursor_open(txn, dbi, cursor);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_cursor_open", rc);
  }
}

/**
 * @see http://symas.com/mdb/doc/group__mdb.html#gad685f5d73c052715c7bd859cc4c05188
 */
static inline void
lmdb::cursor_close(MDB_cursor* const cursor) noexcept {
  ::mdb_cursor_close(cursor);
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#gac8b57befb68793070c85ea813df481af
 */
static inline void
lmdb::cursor_renew(MDB_txn* const txn,
                   MDB_cursor* const cursor) {
  const int rc = ::mdb_cursor_renew(txn, cursor);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_cursor_renew", rc);
  }
}

/**
 * @see http://symas.com/mdb/doc/group__mdb.html#ga7bf0d458f7f36b5232fcb368ebda79e0
 */
static inline MDB_txn*
lmdb::cursor_txn(MDB_cursor* const cursor) noexcept {
  return ::mdb_cursor_txn(cursor);
}

/**
 * @see http://symas.com/mdb/doc/group__mdb.html#ga2f7092cf70ee816fb3d2c3267a732372
 */
static inline MDB_dbi
lmdb::cursor_dbi(MDB_cursor* const cursor) noexcept {
  return ::mdb_cursor_dbi(cursor);
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#ga48df35fb102536b32dfbb801a47b4cb0
 */
static inline bool
lmdb::cursor_get(MDB_cursor* const cursor,
                 MDB_val* const key,
                 MDB_val* const data,
                 const MDB_cursor_op op) {
  const int rc = ::mdb_cursor_get(cursor, key, data, op);
  if (rc != MDB_SUCCESS && rc != MDB_NOTFOUND) {
    error::raise("mdb_cursor_get", rc);
  }
  return (rc == MDB_SUCCESS);
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#ga1f83ccb40011837ff37cc32be01ad91e
 */
static inline void
lmdb::cursor_put(MDB_cursor* const cursor,
                 MDB_val* const key,
                 MDB_val* const data,
                 const unsigned int flags = 0) {
  const int rc = ::mdb_cursor_put(cursor, key, data, flags);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_cursor_put", rc);
  }
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#ga26a52d3efcfd72e5bf6bd6960bf75f95
 */
static inline void
lmdb::cursor_del(MDB_cursor* const cursor,
                 const unsigned int flags = 0) {
  const int rc = ::mdb_cursor_del(cursor, flags);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_cursor_del", rc);
  }
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#ga4041fd1e1862c6b7d5f10590b86ffbe2
 */
static inline void
lmdb::cursor_count(MDB_cursor* const cursor,
                   std::size_t& count) {
  const int rc = ::mdb_cursor_count(cursor, &count);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_cursor_count", rc);
  }
}

////////////////////////////////////////////////////////////////////////////////
/* Resource Interface: Environment */

namespace lmdb {
  class env;
}

/**
 * Resource class for `MDB_env*` handles.
 *
 * @see http://symas.com/mdb/doc/group__internal.html#structMDB__env
 */
class lmdb::env {
protected:
  MDB_env* _handle{nullptr};

public:
  static constexpr unsigned int default_flags = 0;
  static constexpr mode default_mode = 0644; /* -rw-r--r-- */

  /**
   * Creates a new LMDB environment.
   *
   * @param flags
   * @throws lmdb::error on failure
   */
  static env create(const unsigned int flags = default_flags) {
    MDB_env* handle{nullptr};
    lmdb::env_create(&handle);
#if 1
    assert(handle != nullptr);
#endif
    if (flags) {
      try {
        lmdb::env_set_flags(handle, flags);
      }
      catch (const lmdb::error&) {
        lmdb::env_close(handle);
        throw;
      }
    }
    return env{handle};
  }

  /**
   * Constructor.
   *
   * @param handle a valid `MDB_env*` handle
   */
  env(MDB_env* const handle) noexcept
    : _handle{handle} {}

  /**
   * Destructor.
   */
  ~env() noexcept {
    try { close(); } catch (...) {}
  }

  /**
   * Returns the underlying `MDB_env*` handle.
   */
  operator MDB_env*() const noexcept {
    return _handle;
  }

  /**
   * Returns the underlying `MDB_env*` handle.
   */
  MDB_env* handle() const noexcept {
    return _handle;
  }

  /**
   * Flushes data buffers to disk.
   *
   * @param force
   * @throws lmdb::error on failure
   */
  void sync(const bool force = true) {
    lmdb::env_sync(handle(), force);
  }

  /**
   * Closes this environment, releasing the memory map.
   *
   * @note this method is idempotent
   * @post `handle() == nullptr`
   */
  void close() noexcept {
    if (handle()) {
      lmdb::env_close(handle());
      _handle = nullptr;
    }
  }

  /**
   * Opens this environment.
   *
   * @param path
   * @param flags
   * @param mode
   * @throws lmdb::error on failure
   */
  env& open(const char* const path,
            const unsigned int flags = default_flags,
            const mode mode = default_mode) {
    lmdb::env_open(handle(), path, flags, mode);
    return *this;
  }

  /**
   * @param flags
   * @param onoff
   * @throws lmdb::error on failure
   */
  env& set_flags(const unsigned int flags,
                 const bool onoff = true) {
    lmdb::env_set_flags(handle(), flags, onoff);
    return *this;
  }

  /**
   * @param size
   * @throws lmdb::error on failure
   */
  env& set_map_size(const std::size_t size) {
    lmdb::env_set_map_size(handle(), size);
    return *this;
  }

  /**
   * @param count
   * @throws lmdb::error on failure
   */
  env& set_max_readers(const unsigned int count) {
    lmdb::env_set_max_readers(handle(), count);
    return *this;
  }

  /**
   * @param count
   * @throws lmdb::error on failure
   */
  env& set_max_dbs(const MDB_dbi count) {
    lmdb::env_set_max_dbs(handle(), count);
    return *this;
  }
};

////////////////////////////////////////////////////////////////////////////////
/* Resource Interface: Transactions */

namespace lmdb {
  class txn;
}

/**
 * Resource class for `MDB_txn*` handles.
 *
 * @see http://symas.com/mdb/doc/group__internal.html#structMDB__txn
 */
class lmdb::txn {
protected:
  MDB_txn* _handle{nullptr};

public:
  static constexpr unsigned int default_flags = 0;

  /**
   * Creates a new LMDB transaction.
   *
   * @param env the environment handle
   * @param parent
   * @param flags
   * @throws lmdb::error on failure
   */
  static txn begin(MDB_env* const env,
                   MDB_txn* const parent = nullptr,
                   const unsigned int flags = default_flags) {
    MDB_txn* handle{nullptr};
    lmdb::txn_begin(env, parent, flags, &handle);
#if 1
    assert(handle != nullptr);
#endif
    return txn{handle};
  }

  /**
   * Constructor.
   *
   * @param handle a valid `MDB_txn*` handle
   */
  txn(MDB_txn* const handle) noexcept
    : _handle{handle} {}

  /**
   * Destructor.
   */
  ~txn() noexcept {
    if (_handle) {
      // TODO
      _handle = nullptr;
    }
  }

  /**
   * Returns the underlying `MDB_txn*` handle.
   */
  operator MDB_txn*() const noexcept {
    return _handle;
  }

  /**
   * Returns the underlying `MDB_txn*` handle.
   */
  MDB_txn* handle() const noexcept {
    return _handle;
  }

  /**
   * Returns the transaction's `MDB_env*` handle.
   */
  MDB_env* env() const noexcept {
    return lmdb::txn_env(handle());
  }

  /**
   * Commits this transaction.
   *
   * @throws lmdb::error on failure
   * @post `handle() == nullptr`
   */
  void commit() {
    lmdb::txn_commit(_handle);
    _handle = nullptr;
  }

  /**
   * Aborts this transaction.
   *
   * @post `handle() == nullptr`
   */
  void abort() noexcept {
    lmdb::txn_abort(_handle);
    _handle = nullptr;
  }

  /**
   * Resets this read-only transaction.
   */
  void reset() noexcept {
    lmdb::txn_reset(_handle);
  }

  /**
   * Renews this read-only transaction.
   *
   * @throws lmdb::error on failure
   */
  void renew() {
    lmdb::txn_renew(_handle);
  }
};

////////////////////////////////////////////////////////////////////////////////
/* Resource Interface: Databases */

namespace lmdb {
  class dbi;
}

/**
 * Resource class for `MDB_dbi` handles.
 *
 * @see http://symas.com/mdb/doc/group__mdb.html#gadbe68a06c448dfb62da16443d251a78b
 */
class lmdb::dbi {
protected:
  const MDB_dbi _handle;

public:
  static constexpr unsigned int default_flags = 0;

  /**
   * Opens a database handle.
   *
   * @param txn the transaction handle
   * @param name
   * @param flags
   * @throws lmdb::error on failure
   */
  static dbi
  open(MDB_txn* const txn,
       const char* const name = nullptr,
       const unsigned int flags = default_flags) {
    MDB_dbi handle{};
    lmdb::dbi_open(txn, name, flags, &handle);
    return dbi{handle};
  }

  /**
   * Constructor.
   *
   * @param handle a valid `MDB_dbi` handle
   */
  dbi(const MDB_dbi handle) noexcept
    : _handle{handle} {}

  /**
   * Destructor.
   */
  ~dbi() noexcept {
    if (_handle) {
      /* No need to call close() here. */
    }
  }

  /**
   * Returns the underlying `MDB_dbi` handle.
   */
  operator MDB_dbi() const noexcept {
    return _handle;
  }

  /**
   * Returns the underlying `MDB_dbi` handle.
   */
  MDB_dbi handle() const noexcept {
    return _handle;
  }

  /**
   * Returns statistics for this database.
   *
   * @param txn a transaction handle
   * @throws lmdb::error on failure
   */
  MDB_stat stat(MDB_txn* const txn) const {
    MDB_stat result;
    lmdb::dbi_stat(txn, handle(), &result);
    return result;
  }

  /**
   * Retrieves the flags for this database handle.
   *
   * @param txn a transaction handle
   * @throws lmdb::error on failure
   */
  unsigned int flags(MDB_txn* const txn) const {
    unsigned int result{};
    lmdb::dbi_flags(txn, handle(), &result);
    return result;
  }

  /**
   * Returns the number of records in this database.
   *
   * @param txn a transaction handle
   * @throws lmdb::error on failure
   */
  std::size_t size(MDB_txn* const txn) const {
    return stat(txn).ms_entries;
  }

  /**
   * Retrieves a key from this database.
   *
   * @param txn a transaction handle
   * @throws lmdb::error on failure
   */
  template<typename K>
  bool get(MDB_txn* const txn,
           const K& k) const {
    MDB_val key, val{};
    key.mv_size = sizeof(K);
    key.mv_data = const_cast<void*>(reinterpret_cast<const void*>(&k));
    return lmdb::dbi_get(txn, handle(), &key, &val);
  }

  /**
   * Retrieves a key/value pair from this database.
   *
   * @param txn a transaction handle
   * @throws lmdb::error on failure
   */
  template<typename K, typename V>
  bool get(MDB_txn* const txn,
           const K& k,
           V& v) const {
    MDB_val key, val{};
    key.mv_size = sizeof(K);
    key.mv_data = const_cast<void*>(reinterpret_cast<const void*>(&k));
    const bool result = lmdb::dbi_get(txn, handle(), &key, &val);
    if (result) {
      v = *reinterpret_cast<const V*>(val.mv_data);
    }
    return result;
  }
};

////////////////////////////////////////////////////////////////////////////////
/* Resource Interface: Cursors */

namespace lmdb {
  class cursor;
}

/**
 * Resource class for `MDB_cursor*` handles.
 *
 * @see http://symas.com/mdb/doc/group__internal.html#structMDB__cursor
 */
class lmdb::cursor {
protected:
  MDB_cursor* _handle;

public:
  static constexpr unsigned int default_flags = 0;

  /**
   * Creates an LMDB cursor.
   *
   * @param txn the transaction handle
   * @param dbi the database handle
   * @throws lmdb::error on failure
   */
  static cursor
  open(MDB_txn* const txn,
       const MDB_dbi dbi) {
    MDB_cursor* handle{};
    lmdb::cursor_open(txn, dbi, &handle);
#if 1
    assert(handle != nullptr);
#endif
    return cursor{handle};
  }

  /**
   * Constructor.
   *
   * @param handle a valid `MDB_cursor*` handle
   */
  cursor(MDB_cursor* const handle) noexcept
    : _handle{handle} {}

  /**
   * Destructor.
   */
  ~cursor() noexcept {
    try { close(); } catch (...) {}
  }

  /**
   * Returns the underlying `MDB_cursor*` handle.
   */
  operator MDB_cursor*() const noexcept {
    return _handle;
  }

  /**
   * Returns the underlying `MDB_cursor*` handle.
   */
  MDB_cursor* handle() const noexcept {
    return _handle;
  }

  /**
   * Closes this cursor.
   *
   * @note this method is idempotent
   * @post `handle() == nullptr`
   */
  void close() noexcept {
    if (handle()) {
      lmdb::cursor_close(handle());
      _handle = nullptr;
    }
  }

  /**
   * Renews this cursor.
   *
   * @param txn the transaction scope
   * @throws lmdb::error on failure
   */
  void renew(MDB_txn* const txn) {
    lmdb::cursor_renew(txn, handle());
  }

  /**
   * Returns the cursor's transaction handle.
   */
  MDB_txn* txn() const noexcept {
    return lmdb::cursor_txn(handle());
  }

  /**
   * Returns the cursor's database handle.
   */
  MDB_dbi dbi() const noexcept {
    return lmdb::cursor_dbi(handle());
  }

  /**
   * Retrieves a key from the database.
   *
   * @param key
   * @param op
   * @throws lmdb::error on failure
   */
  bool get(MDB_val* const key,
           const MDB_cursor_op op) {
    return get(key, nullptr, op);
  }

  /**
   * Retrieves a key/value pair from the database.
   *
   * @param key
   * @param data (may be `nullptr`)
   * @param op
   * @throws lmdb::error on failure
   */
  bool get(MDB_val* const key,
           MDB_val* const data,
           const MDB_cursor_op op) {
    return lmdb::cursor_get(handle(), key, data, op);
  }

  /**
   * Positions this cursor at the given key.
   *
   * @param k
   * @throws lmdb::error on failure
   */
  template<typename K>
  bool find(const K& k) {
    MDB_val key, val{};
    key.mv_size = sizeof(K);
    key.mv_data = const_cast<void*>(reinterpret_cast<const void*>(&k));
    return get(&key, &val, MDB_SET);
  }
};

////////////////////////////////////////////////////////////////////////////////
/* Resource Interface: Values */

namespace lmdb {
  class val;
}

/**
 * Wrapper class for `MDB_val` structures.
 *
 * @see http://symas.com/mdb/doc/group__mdb.html#structMDB__val
 */
class lmdb::val {
protected:
  MDB_val _val;

public:
  /**
   * Default constructor.
   */
  val() noexcept = default;

  /**
   * Constructor.
   */
  val(const std::string& data) noexcept
    : val{data.data(), data.size()} {}

  /**
   * Constructor.
   */
  val(const char* const data) noexcept
    : val{data, std::strlen(data)} {}

  /**
   * Constructor.
   */
  val(const char* const data,
      const std::size_t size) noexcept
    : _val{size, const_cast<char*>(data)} {}

  /**
   * Destructor.
   */
  ~val() noexcept = default;

  /**
   * Returns an `MDB_val*` pointer.
   */
  operator MDB_val*() noexcept {
    return &_val;
  }

  /**
   * Returns an `MDB_val*` pointer.
   */
  operator const MDB_val*() const noexcept {
    return &_val;
  }
};

static_assert(std::is_pod<lmdb::val>::value, "lmdb::val must be a POD type");
static_assert(sizeof(lmdb::val) == sizeof(MDB_val), "sizeof(lmdb::val) != sizeof(MDB_val)");

////////////////////////////////////////////////////////////////////////////////

#endif /* LMDBXX_H */
