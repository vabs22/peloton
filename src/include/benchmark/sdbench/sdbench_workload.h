//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// sdbench_workload.h
//
// Identification: src/include/benchmark/sdbench/sdbench_workload.h
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//


#pragma once

#include "benchmark/sdbench/sdbench_configuration.h"

namespace peloton {
namespace benchmark {
namespace sdbench {

extern configuration state;

void CreateAndLoadTable(LayoutType layout_type);

void RunQuery(const std::vector<oid_t>& tuple_key_attrs,
              const std::vector<oid_t>& index_key_attrs);

void RunInsertTest();

void RunAdaptExperiment();

}  // namespace sdbench
}  // namespace benchmark
}  // namespace peloton
