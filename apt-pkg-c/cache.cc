#include <apt-pkg/acquire-item.h>
#include <apt-pkg/algorithms.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/indexfile.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/policy.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/update.h>
#include <apt-pkg/version.h>
#include <fnmatch.h>

// Headers for the cxx bridge
#include "rust-apt/apt-pkg-c/util.h"
#include "rust-apt/src/cache.rs"
#include "rust-apt/src/progress.rs"


/// Create the CacheFile.
std::unique_ptr<PkgCacheFile> pkg_cache_create(rust::Slice<const rust::String> deb_files) {
	std::unique_ptr<PkgCacheFile> cache = std::make_unique<PkgCacheFile>();

	for (auto deb_str : deb_files) {
		std::string deb_string(deb_str.c_str());

		// Add the deb to the cache.
		if (!cache->GetSourceList()->AddVolatileFile(deb_string)) {
			_error->Error(
			"%s", ("Couldn't add '" + deb_string + "' to the cache.").c_str());
			handle_errors();
		}

		handle_errors();
	}

	cache->BuildCaches();
	handle_errors();

	return cache;
}

/// Update the package lists, handle errors and return a Result.
void cache_update(const std::unique_ptr<PkgCacheFile>& cache, DynAcquireProgress& callback) {
	AcqTextStatus progress(callback);

	ListUpdate(progress, *cache->GetSourceList(), pulse_interval(callback));
	handle_errors();
}


/// Get the package list uris. This is the files that are updated with `apt update`.
rust::Vec<SourceFile> source_uris(const std::unique_ptr<PkgCacheFile>& cache) {
	pkgAcquire fetcher;
	rust::Vec<SourceFile> list;

	cache->GetSourceList()->GetIndexes(&fetcher, true);
	pkgAcquire::UriIterator I = fetcher.UriBegin();

	for (; I != fetcher.UriEnd(); ++I) {
		list.push_back(SourceFile{ I->URI, flNotDir(I->Owner->DestFile) });
	}
	return list;
}

/// Helper function to check if the package should be included in the list
static bool include_pkg(const std::unique_ptr<PkgCacheFile>& cache,
pkgCache::PkgIterator pkg,
const PackageSort& sort) {
	// If Virtual Packages is not enabled will check if they're virtual
	if ((sort.virtual_pkgs != Sort::Enable) &&
	// If the package is Virtual it will not have a version list
	// And if disabled then we will exclude this package
	((sort.virtual_pkgs == Sort::Disable && !pkg.VersionList()) ||
	// If reverse then exclude any packages that DO have a version list
	(sort.virtual_pkgs == Sort::Reverse && pkg.VersionList()))) {
		return false;
	}

	if ((sort.upgradable != Sort::Disable) &&
	((sort.upgradable == Sort::Enable && !is_upgradable(cache, pkg)) ||
	(sort.upgradable == Sort::Reverse && is_upgradable(cache, pkg)))) {
		return false;
	}

	if ((sort.installed != Sort::Disable) &&
	((sort.installed == Sort::Enable && !pkg.CurrentVer()) ||
	(sort.installed == Sort::Reverse && pkg.CurrentVer()))) {
		return false;
	}

	if ((sort.auto_installed != Sort::Disable) &&
	((sort.auto_installed == Sort::Enable && !is_auto_installed(cache, pkg)) ||
	(sort.auto_installed == Sort::Reverse && is_auto_installed(cache, pkg)))) {
		return false;
	}

	if ((sort.auto_removable != Sort::Disable) &&
	((sort.auto_removable == Sort::Enable && !is_auto_removable(cache, pkg)) ||
	(sort.auto_removable == Sort::Reverse && is_auto_removable(cache, pkg)))) {
		return false;
	}
	return true;
}

/// Returns a Vector of all the packages in the cache.
rust::Vec<PackagePtr> pkg_list(
const std::unique_ptr<PkgCacheFile>& cache, const PackageSort& sort) {
	rust::Vec<PackagePtr> list;
	pkgCache::PkgIterator pkg;

	for (pkg = cache->GetPkgCache()->PkgBegin(); !pkg.end(); pkg++) {

		if (!include_pkg(cache, pkg, sort)) {
			continue;
		}

		list.push_back(wrap_package(pkg));
	}
	return list;
}

/// Returns a Vector of all the packages in the cache.
GlobResults glob_pkgs(const std::unique_ptr<PkgCacheFile>& cache,
const PackageSort& sort,
rust::Slice<const rust::String> globs) {
	rust::Vec<PackagePtr> list;
	std::unordered_set<std::string> found;

	pkgCache::PkgIterator pkg;
	for (pkg = cache->GetPkgCache()->PkgBegin(); !pkg.end(); pkg++) {

		// First check if the package should even be included
		if (!include_pkg(cache, pkg, sort)) {
			continue;
		}

		// If the package passes sorting, we can now glob pkg name
		for (auto glob : globs) {
			const char* c_glob = glob.c_str();
			if (fnmatch(c_glob, pkg.Name(), FNM_CASEFOLD) == 0) {
				// Add that the glob has matched something
				found.insert(c_glob);
				// Add the package to our list
				list.push_back(wrap_package(pkg));
				break;
			}
		}
	}

	rust::Vec<rust::String> failed_globs;
	for (auto glob : globs) {
		// Check against the matched globs and create a list of non matched
		if (found.find(glob.c_str()) == found.end()) {
			failed_globs.push_back(glob);
		}
	}

	// These must be moved since they contain UniquePtrs
	return GlobResults{ std::move(list), std::move(failed_globs) };
}

/// Return a Vector of all the VersionFiles for a version.
rust::vec<VersionFile> ver_file_list(const VersionPtr& ver) {
	rust::vec<VersionFile> list;

	pkgCache::VerFileIterator v_file = ver.ptr->FileList();

	for (; !v_file.end(); v_file++) {
		list.push_back(wrap_ver_file(v_file));
	}
	return list;
}

/// Return a Vector of all the PackageFiles for a version.
rust::vec<PackageFile> ver_pkg_file_list(const VersionPtr& ver) {
	rust::vec<PackageFile> list;

	pkgCache::VerFileIterator v_file = ver.ptr->FileList();

	for (; !v_file.end(); v_file++) {
		list.push_back(wrap_pkg_file(v_file.File()));
	}
	return list;
}


/// Return a Vector of all the versions of a package.
rust::Vec<VersionPtr> pkg_version_list(const PackagePtr& pkg) {
	rust::Vec<VersionPtr> list;

	for (pkgCache::VerIterator I = pkg.ptr->VersionList(); !I.end(); I++) {
		list.push_back(wrap_version(I));
	}
	return list;
}


/// Return a Vector of all the packages that provide another. steam:i386 provides steam.
rust::Vec<PackagePtr> pkg_provides_list(
const std::unique_ptr<PkgCacheFile>& cache, const PackagePtr& pkg, bool cand_only) {
	pkgCache::PrvIterator provide = pkg.ptr->ProvidesList();
	std::set<std::string> set;
	rust::vec<PackagePtr> list;

	for (; !provide.end(); provide++) {
		pkgCache::PkgIterator pkg = provide.OwnerPkg();
		bool is_cand = (provide.OwnerVer() == cache->GetPolicy()->GetCandidateVer(pkg));
		// If cand_only is true, then we check if ithe package is candidate.
		if (!cand_only || is_cand) {
			// Make sure we do not have duplicate packages.
			if (!set.insert(pkg.FullName()).second) {
				continue;
			}

			list.push_back(wrap_package(pkg));
		}
	}
	return list;
}


/// Return a package by name. Ptr will be NULL if the package doesn't exist.
PackagePtr pkg_cache_find_name(const std::unique_ptr<PkgCacheFile>& cache, rust::string name) {
	return wrap_package(cache->GetPkgCache()->FindPkg(name.c_str()));
}


/// Return a package by name and architecture.
/// Ptr will be NULL if the package doesn't exist.
PackagePtr pkg_cache_find_name_arch(
const std::unique_ptr<PkgCacheFile>& cache, rust::string name, rust::string arch) {
	return wrap_package(cache->GetPkgCache()->FindPkg(name.c_str(), arch.c_str()));
}

/// The path to the PackageFile
rust::string filename(const PackageFile& pkg_file) {
	return handle_null_str(pkg_file.ptr->pkg_file.FileName());
}

/// The Archive of the PackageFile. ex: unstable
rust::string archive(const PackageFile& pkg_file) {
	return handle_null_str(pkg_file.ptr->pkg_file.Archive());
}

/// The Origin of the PackageFile. ex: Debian
rust::string origin(const PackageFile& pkg_file) {
	return handle_null_str(pkg_file.ptr->pkg_file.Origin());
}

/// The Codename of the PackageFile. ex: main, non-free
rust::string codename(const PackageFile& pkg_file) {
	return handle_null_str(pkg_file.ptr->pkg_file.Codename());
}

/// The Label of the PackageFile. ex: Debian
rust::string label(const PackageFile& pkg_file) {
	return handle_null_str(pkg_file.ptr->pkg_file.Label());
}

/// The Hostname of the PackageFile. ex: deb.debian.org
rust::string site(const PackageFile& pkg_file) {
	return handle_null_str(pkg_file.ptr->pkg_file.Site());
}

/// The Component of the PackageFile. ex: sid
rust::string component(const PackageFile& pkg_file) {
	return handle_null_str(pkg_file.ptr->pkg_file.Component());
}

/// The Architecture of the PackageFile. ex: amd64
rust::string arch(const PackageFile& pkg_file) {
	return handle_null_str(pkg_file.ptr->pkg_file.Architecture());
}


/// The Index Type of the PackageFile. Known values are:
///
/// Debian Package Index,
/// Debian Translation Index,
/// Debian dpkg status file,
rust::string index_type(const PackageFile& pkg_file) {
	return handle_null_str(pkg_file.ptr->pkg_file.IndexType());
}

/// The Index of the PackageFile
u_int64_t index(const PackageFile& pkg_file) {
	return pkg_file.ptr->pkg_file.Index();
}

/// Return true if the PackageFile is trusted.
bool pkg_file_is_trusted(const std::unique_ptr<PkgCacheFile>& cache, PackageFile& pkg_file) {
	if (!pkg_file.ptr->index) {
		pkgIndexFile* index;

		if (!cache->GetSourceList()->FindIndex(pkg_file.ptr->pkg_file, index)) {
			_system->FindIndex(pkg_file.ptr->pkg_file, index);
		}
		pkg_file.ptr->index = index;
	}
	return pkg_file.ptr->index->IsTrusted();
}
