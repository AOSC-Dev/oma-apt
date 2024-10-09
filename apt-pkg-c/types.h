#pragma once
#include <apt-pkg/indexfile.h>
#include <memory>
#include "rust/cxx.h"

using namespace rust;

template <typename T>
using UniquePtr = std::unique_ptr<T>;


#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

// Forward declarations for progress.rs
struct ItemDesc;
struct PkgAcquire;
struct AcqWorker;
