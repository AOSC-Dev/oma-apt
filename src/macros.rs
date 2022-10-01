#[macro_export]
/// Macro to create the cache, optionally including debs
///
/// Here is an example of the two ways you can use this.
///
/// ```
/// use rust_apt::new_cache;
///
/// let cache = new_cache!().unwrap();
///
/// println!("{}", cache.get("apt").unwrap().name());
///
/// let local_debs = vec![
///     "tests/files/cache/apt.deb",
///     "tests/files/cache/dep-pkg1_0.0.1.deb",
/// ];
///
/// let cache = new_cache!(&local_debs).unwrap();
/// println!("{}", cache.get("apt").unwrap().get_version("5000:1.0.0").unwrap().version());
/// ```
///
/// Returns `Result<rust_apt::cache::Cache, cxx::Exception>`
macro_rules! new_cache {
	() => {{
		Ok($crate::cache::Cache::new())
	} as Result<$crate::cache::Cache, cxx::Exception> };
	($slice:expr) => {{
		$crate::cache::Cache::debs($slice)
	}};
}