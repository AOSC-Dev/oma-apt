#include <sstream>

#include <assert.h>

#include <apt-pkg/pkgcache.h>
#include <apt-pkg/prettyprinters.h>
#include <apt-pkg/cachefile.h>

struct PCache {
    // Owned by us.
    pkgCacheFile *cache_file;

    // Borrowed from cache_file.
    pkgCache *cache;
};

struct PPkgIterator {
    // Owned by us.
    pkgCache::PkgIterator iterator;

    // Borrowed from PCache.
    pkgCache *cache;
};

struct PVerIterator {
    // Owned by us.
    pkgCache::VerIterator iterator;

    // Borrowed from PCache.
    pkgCache::PkgIterator *pkg;
};

extern "C" {
    void init_config_system();

    PCache *pkg_cache_create();

    PPkgIterator *pkg_cache_pkg_iter(PCache *cache);
    PPkgIterator *pkg_cache_find_name(PCache *cache, const char *name);
    PPkgIterator *pkg_cache_find_name_arch(PCache *cache, const char *name, const char *arch);
    void pkg_iter_release(PPkgIterator *iterator);

    void pkg_iter_next(PPkgIterator *iterator);
    bool pkg_iter_end(PPkgIterator *iterator);

    const char *pkg_iter_name(PPkgIterator *iterator);
    const char *pkg_iter_arch(PPkgIterator *iterator);
    const char *pkg_iter_current_version(PPkgIterator *iterator);

    // freed by caller
    char *pkg_iter_pretty(PCache *cache, PPkgIterator *iterator);

    PVerIterator *pkg_iter_ver_iter(PPkgIterator *iterator);
    void ver_iter_release(PVerIterator *iterator);

    void ver_iter_next(PVerIterator *iterator);
    bool ver_iter_end(PVerIterator *iterator);

    const char *ver_iter_version(PVerIterator *iterator);
    const char *ver_iter_section(PVerIterator *iterator);
    const char *ver_iter_source_package(PVerIterator *iterator);
    const char *ver_iter_source_version(PVerIterator *iterator);
    const char *ver_iter_arch(PVerIterator *iterator);
}

void init_config_system() {
    pkgInitConfig(*_config);
    pkgInitSystem(*_config, _system);
}

PCache *pkg_cache_create() {
    pkgCacheFile *cache_file = new pkgCacheFile();
    pkgCache *cache = cache_file->GetPkgCache();

    PCache *ret = new PCache();
    ret->cache_file = cache_file;
    ret->cache = cache;

    return ret;
}

// TODO: we don't expose this so we always leak the wrapper.
void pkg_cache_release(PCache *cache) {
    // TODO: is cache->cache cleaned up with cache->cache_file?
    delete cache->cache_file;
    delete cache;
}

PPkgIterator *pkg_cache_pkg_iter(PCache *cache) {
    PPkgIterator *wrapper = new PPkgIterator();
    wrapper->iterator = cache->cache->PkgBegin();
    wrapper->cache = cache->cache;
    return wrapper;
}

PPkgIterator *pkg_cache_find_name(PCache *cache, const char *name) {
    PPkgIterator *wrapper = new PPkgIterator();
    wrapper->iterator = cache->cache->FindPkg(name);
    wrapper->cache = cache->cache;
    return wrapper;
}

PPkgIterator *pkg_cache_find_name_arch(PCache *cache, const char *name, const char *arch) {
    PPkgIterator *wrapper = new PPkgIterator();
    wrapper->iterator = cache->cache->FindPkg(name, arch);
    wrapper->cache = cache->cache;
    return wrapper;
}

void pkg_iter_release(PPkgIterator *wrapper) {
    delete wrapper;
}

void pkg_iter_next(PPkgIterator *wrapper) {
    ++wrapper->iterator;
}

bool pkg_iter_end(PPkgIterator *wrapper) {
    return wrapper->cache->PkgEnd() == wrapper->iterator;
}

const char *pkg_iter_name(PPkgIterator *wrapper) {
    return wrapper->iterator.Name();
}

const char *pkg_iter_arch(PPkgIterator *wrapper) {
    return wrapper->iterator.Arch();
}

const char *pkg_iter_current_version(PPkgIterator *wrapper) {
    return wrapper->iterator.CurVersion();
}

char *pkg_iter_pretty(PCache *cache, PPkgIterator *wrapper) {
    assert(cache);
    assert(wrapper);
    std::stringstream ss;
    ss << APT::PrettyPkg(cache->cache_file->GetDepCache(), wrapper->iterator);
    return strdup(ss.str().c_str());
}

PVerIterator *pkg_iter_ver_iter(PPkgIterator *wrapper) {
    PVerIterator *new_wrapper = new PVerIterator();
    new_wrapper->iterator = wrapper->iterator.VersionList();
    new_wrapper->pkg = &wrapper->iterator;
    return new_wrapper;
}

void ver_iter_release(PVerIterator *wrapper) {
    delete wrapper;
}

void ver_iter_next(PVerIterator *wrapper) {
    ++wrapper->iterator;
}

bool ver_iter_end(PVerIterator *wrapper) {
    return wrapper->iterator.end();
}


const char *ver_iter_version(PVerIterator *wrapper) {
    return wrapper->iterator.VerStr();
}

const char *ver_iter_section(PVerIterator *wrapper) {
   return wrapper->iterator.Section();
}

const char *ver_iter_source_package(PVerIterator *wrapper) {
    return wrapper->iterator.SourcePkgName();
}

const char *ver_iter_source_version(PVerIterator *wrapper) {
    return wrapper->iterator.SourceVerStr();
}

const char *ver_iter_arch(PVerIterator *wrapper) {
    return wrapper->iterator.Arch();
}