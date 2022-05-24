use std::collections::hash_map::DefaultHasher;
use std::fmt;
use std::hash::{Hash, Hasher};

impl fmt::Display for apt::SourceFile {
	fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
		write!(f, "Source< Uri: {}, Filename: {}>", self.uri, self.filename)?;
		Ok(())
	}
}

impl apt::Provider {
	pub fn hash(&self) -> u64 {
		let mut s = DefaultHasher::new();
		self.name.hash(&mut s);
		s.finish()
	}
}

#[cxx::bridge]
pub mod apt {

	/// Struct representing a Source File.
	///
	/// uri = `http://deb.volian.org/volian/dists/scar/InRelease`
	///
	/// filename = `deb.volian.org_volian_dists_scar_InRelease`
	#[derive(Debug)]
	struct SourceFile {
		uri: String,
		filename: String,
	}

	#[derive(Debug)]
	// Simple struct for getting provider list
	struct Provider {
		name: String,
		ptr: *mut PkgIterator,
	}

	unsafe extern "C++" {
		type PCache;
		type PkgIterator;
		type PkgFileIterator;
		type VerIterator;
		type VerFileIterator;
		type DepIterator;
		type VerFileParser;
		type PkgRecords;
		type PkgIndexFile;
		type DescIterator;
		type PkgDepCache;
		include!("rust-apt/apt-pkg-c/apt-pkg.h");

		/// Main Initializers for APT
		pub fn init_config_system();

		pub fn pkg_cache_create() -> *mut PCache;
		pub unsafe fn pkg_records_create(pcache: *mut PCache) -> *mut PkgRecords;
		pub unsafe fn depcache_create(pcache: *mut PCache) -> *mut PkgDepCache;

		pub unsafe fn pkg_cache_release(cache: *mut PCache);
		pub unsafe fn pkg_records_release(records: *mut PkgRecords);

		pub unsafe fn source_uris(pcache: *mut PCache) -> Vec<SourceFile>;
		// pub unsafe fn pkg_cache_compare_versions(
		// 	cache: *mut PCache,
		// 	left: *const c_char,
		// 	right: *const c_char,
		// ) -> i32;

		/// Iterator Creators
		pub unsafe fn pkg_begin(cache: *mut PCache) -> *mut PkgIterator;
		pub unsafe fn pkg_clone(iterator: *mut PkgIterator) -> *mut PkgIterator;

		pub unsafe fn ver_clone(iterator: *mut VerIterator) -> *mut VerIterator;
		pub unsafe fn ver_file(iterator: *mut VerIterator) -> *mut VerFileIterator;
		pub unsafe fn ver_file_clone(iterator: *mut VerFileIterator) -> *mut VerFileIterator;

		pub unsafe fn pkg_current_version(iterator: *mut PkgIterator) -> *mut VerIterator;
		pub unsafe fn pkg_candidate_version(
			cache: *mut PCache,
			iterator: *mut PkgIterator,
		) -> *mut VerIterator;
		pub unsafe fn pkg_version_list(pkg: *mut PkgIterator) -> *mut VerIterator;

		pub unsafe fn ver_pkg_file(iterator: *mut VerFileIterator) -> *mut PkgFileIterator;
		pub unsafe fn ver_desc_file(iterator: *mut VerIterator) -> *mut DescIterator;
		pub unsafe fn pkg_index_file(
			pcache: *mut PCache,
			pkg_file: *mut PkgFileIterator,
		) -> *mut PkgIndexFile;

		pub unsafe fn pkg_cache_find_name(cache: *mut PCache, name: String) -> *mut PkgIterator;
		pub unsafe fn pkg_cache_find_name_arch(
			cache: *mut PCache,
			name: String,
			arch: String,
		) -> *mut PkgIterator;

		/// Iterator Manipulation
		pub unsafe fn pkg_next(iterator: *mut PkgIterator);
		pub unsafe fn pkg_end(iterator: *mut PkgIterator) -> bool;
		pub unsafe fn pkg_release(iterator: *mut PkgIterator);

		pub unsafe fn ver_next(iterator: *mut VerIterator);
		pub unsafe fn ver_end(iterator: *mut VerIterator) -> bool;
		pub unsafe fn ver_release(iterator: *mut VerIterator);

		pub unsafe fn ver_file_next(iterator: *mut VerFileIterator);
		pub unsafe fn ver_file_end(iterator: *mut VerFileIterator) -> bool;
		pub unsafe fn ver_file_release(iterator: *mut VerFileIterator);

		pub unsafe fn pkg_index_file_release(iterator: *mut PkgIndexFile);
		pub unsafe fn pkg_file_release(iterator: *mut PkgFileIterator);
		pub unsafe fn ver_desc_release(iterator: *mut DescIterator);

		/// Information Accessors
		pub unsafe fn pkg_is_upgradable(
			depcache: *mut PkgDepCache,
			iterator: *mut PkgIterator,
		) -> bool;
		pub unsafe fn pkg_is_auto_installed(
			depcache: *mut PkgDepCache,
			wrapper: *mut PkgIterator,
		) -> bool;
		pub unsafe fn pkg_is_garbage(depcache: *mut PkgDepCache, wrapper: *mut PkgIterator)
			-> bool;
		pub unsafe fn pkg_marked_install(
			depcache: *mut PkgDepCache,
			wrapper: *mut PkgIterator,
		) -> bool;
		pub unsafe fn pkg_marked_upgrade(
			depcache: *mut PkgDepCache,
			wrapper: *mut PkgIterator,
		) -> bool;
		pub unsafe fn pkg_marked_delete(
			depcache: *mut PkgDepCache,
			wrapper: *mut PkgIterator,
		) -> bool;
		pub unsafe fn pkg_marked_keep(
			depcache: *mut PkgDepCache,
			wrapper: *mut PkgIterator,
		) -> bool;
		pub unsafe fn pkg_marked_downgrade(
			depcache: *mut PkgDepCache,
			wrapper: *mut PkgIterator,
		) -> bool;
		pub unsafe fn pkg_marked_reinstall(
			depcache: *mut PkgDepCache,
			wrapper: *mut PkgIterator,
		) -> bool;
		pub unsafe fn pkg_is_now_broken(
			depcache: *mut PkgDepCache,
			wrapper: *mut PkgIterator,
		) -> bool;
		pub unsafe fn pkg_is_inst_broken(
			depcache: *mut PkgDepCache,
			wrapper: *mut PkgIterator,
		) -> bool;
		pub unsafe fn pkg_is_installed(iterator: *mut PkgIterator) -> bool;
		pub unsafe fn pkg_has_versions(iterator: *mut PkgIterator) -> bool;
		pub unsafe fn pkg_has_provides(iterator: *mut PkgIterator) -> bool;
		pub unsafe fn pkg_provides_list(
			cache: *mut PCache,
			iterator: *mut PkgIterator,
			cand_only: bool,
		) -> Vec<Provider>;
		pub unsafe fn get_fullname(iterator: *mut PkgIterator, pretty: bool) -> String;
		// pub unsafe fn pkg_name(iterator: *mut PkgIterator) -> String;
		pub unsafe fn pkg_arch(iterator: *mut PkgIterator) -> String;
		pub unsafe fn pkg_id(iterator: *mut PkgIterator) -> i32;
		pub unsafe fn pkg_current_state(iterator: *mut PkgIterator) -> i32;
		pub unsafe fn pkg_inst_state(iterator: *mut PkgIterator) -> i32;
		pub unsafe fn pkg_selected_state(iterator: *mut PkgIterator) -> i32;
		pub unsafe fn pkg_essential(iterator: *mut PkgIterator) -> bool;

		pub unsafe fn ver_arch(iterator: *mut VerIterator) -> String;
		pub unsafe fn ver_str(iterator: *mut VerIterator) -> String;
		pub unsafe fn ver_section(iterator: *mut VerIterator) -> String;
		pub unsafe fn ver_priority_str(iterator: *mut VerIterator) -> String;
		pub unsafe fn ver_priority(cache: *mut PCache, iterator: *mut VerIterator) -> i32;
		// pub unsafe fn ver_source_package(iterator: *mut VerIterator) -> *const
		// c_char; pub unsafe fn ver_source_version(iterator: *mut VerIterator) ->
		// *const c_char;
		pub unsafe fn ver_name(iterator: *mut VerIterator) -> String;
		pub unsafe fn ver_size(iterator: *mut VerIterator) -> i32;
		pub unsafe fn ver_installed_size(iterator: *mut VerIterator) -> i32;
		pub unsafe fn ver_downloadable(iterator: *mut VerIterator) -> bool;
		pub unsafe fn ver_id(iterator: *mut VerIterator) -> i32;
		pub unsafe fn ver_installed(iterator: *mut VerIterator) -> bool;

		/// Package Records Management
		pub unsafe fn ver_file_lookup(records: *mut PkgRecords, iterator: *mut VerFileIterator);
		pub unsafe fn desc_file_lookup(records: *mut PkgRecords, iterator: *mut DescIterator);
		pub unsafe fn ver_uri(records: *mut PkgRecords, index_file: *mut PkgIndexFile) -> String;
		pub unsafe fn long_desc(records: *mut PkgRecords) -> String;
		pub unsafe fn short_desc(records: *mut PkgRecords) -> String;
		pub unsafe fn hash_find(records: *mut PkgRecords, hash_type: String) -> String;
		// pub unsafe fn long_desc(
		// 	cache: *mut PCache,
		// 	records: *mut PkgRecords,
		// 	iterator: *mut PkgIterator,
		// ) -> String;

		// Unused Functions
		// They may be used in the future
		// pub unsafe fn validate(iterator: *mut VerIterator, depcache: *mut PCache) ->
		// bool; pub unsafe fn ver_iter_dep_iter(iterator: *mut VerIterator) -> *mut
		// DepIterator; pub unsafe fn dep_iter_release(iterator: *mut DepIterator);

		// pub unsafe fn dep_iter_next(iterator: *mut DepIterator);
		// pub unsafe fn dep_iter_end(iterator: *mut DepIterator) -> bool;

		// pub fn dep_iter_target_pkg(iterator: *mut DepIterator) -> *mut PkgIterator;
		// pub fn dep_iter_target_ver(iterator: *mut DepIterator) -> *const c_char;
		// pub fn dep_iter_comp_type(iterator: *mut DepIterator) -> *const c_char;
		// pub fn dep_iter_dep_type(iterator: *mut DepIterator) -> *const c_char;

		// pub fn ver_file_parser_short_desc(parser: VerFileParser) -> *mut c_char;
		// pub fn ver_file_parser_long_desc(parser: VerFileParser) -> *mut c_char;

		// pub fn ver_file_parser_maintainer(parser: VerFileParser) -> *mut c_char;
		// pub fn ver_file_parser_homepage(parser: VerFileParser) -> *mut c_char;

		// pub unsafe fn pkg_file_iter_next(iterator: *mut PkgFileIterator);
		// pub unsafe fn pkg_file_iter_end(iterator: *mut PkgFileIterator) -> bool;

		// pub unsafe fn pkg_file_iter_file_name(iterator: *mut PkgFileIterator) ->
		// *const c_char; pub unsafe fn pkg_file_iter_archive(iterator: *mut
		// PkgFileIterator) -> *const c_char; pub unsafe fn
		// pkg_file_iter_version(iterator: *mut PkgFileIterator) -> *const c_char;
		// pub unsafe fn pkg_file_iter_origin(iterator: *mut PkgFileIterator) -> *const
		// c_char; pub unsafe fn pkg_file_iter_codename(iterator: *mut PkgFileIterator)
		// -> *const c_char; pub unsafe fn pkg_file_iter_label(iterator: *mut
		// PkgFileIterator) -> *const c_char; pub unsafe fn pkg_file_iter_site(iterator:
		// *mut PkgFileIterator) -> *const c_char; pub unsafe fn
		// pkg_file_iter_component(iterator: *mut PkgFileIterator) -> *const c_char; pub
		// unsafe fn pkg_file_iter_architecture(iterator: *mut PkgFileIterator) ->
		// *const c_char; pub unsafe fn pkg_file_iter_index_type(iterator: *mut
		// PkgFileIterator) -> *const c_char;
	}
}
