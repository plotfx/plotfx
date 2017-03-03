/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *   Copyright (c) 2016 Paul Asmuth, FnordCorp B.V. <paul@asmuth.com>
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnordmetric/types.h>
#include <fnordmetric/sample.h>
#include <fnordmetric/util/return_code.h>
#include <fnordmetric/aggregate.h>
#include <fnordmetric/summarize.h>
#include <fnordmetric/units.h>
#include <fnordmetric/util/option.h>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <mutex>

namespace fnordmetric {
class MetricMap;

using MetricIDType = std::string;

enum MetricKind : uint32_t {
  UNKNOWN           = 0,
  SAMPLE_UINT64     = 1,
  SAMPLE_INT64      = 2,
  SAMPLE_FLOAT64    = 3,
  COUNTER_UINT64    = 4,
  COUNTER_INT64     = 5,
  COUNTER_FLOAT64   = 6,
  MONOTONIC_UINT64  = 7,
  MONOTONIC_INT64   = 8,
  MONOTONIC_FLOAT64 = 9,
  MIN_UINT64        = 10,
  MIN_INT64         = 11,
  MIN_FLOAT64       = 12,
  MAX_UINT64        = 13,
  MAX_INT64         = 14,
  MAX_FLOAT64       = 15,
  AVERAGE_UINT64    = 16,
  AVERAGE_INT64     = 17,
  AVERAGE_FLOAT64   = 18
};

struct TableConfig {
  TableConfig();
  MetricKind kind;
  uint64_t interval;
  uint64_t display_granularity;
  bool is_valid;
  std::string unit_id;
  std::shared_ptr<const UnitConfig> unit_config;
  tval_autoref unit_scale;
  std::vector<GrossSummaryMethod> summarize_gross;
  Option<GroupSummaryMethod> summarize_group;
};

class Metric {
public:

  Metric(
      const std::string& key);

  size_t getTotalBytes() const;
  TimestampType getLastInsertTime();

  std::shared_ptr<const TableConfig> getConfig() const;
  ReturnCode setConfig(std::shared_ptr<const TableConfig> config);

  std::mutex& getInsertLock();

protected:
  std::string key_;
  mutable std::mutex config_mutex_;
  std::shared_ptr<const TableConfig> config_;
  std::shared_ptr<const UnitConfig> unit_config_;
  std::mutex insert_mutex_;
};

class MetricInfo {
public:

  MetricInfo();
  MetricInfo(Metric* metric, std::shared_ptr<MetricMap> metric_map);

  MetricInfo(const MetricInfo& o) = delete;
  MetricInfo(MetricInfo&& o);
  MetricInfo& operator=(const MetricInfo& o) = delete;
  MetricInfo& operator=(MetricInfo&& o);

  std::shared_ptr<const UnitConfig> getUnitConfig() const;
  std::shared_ptr<const TableConfig> getTableConfig() const;

protected:
  Metric* metric_;
  std::shared_ptr<MetricMap> metric_map_;
};

tval_type getMetricDataType(MetricKind t);
GroupSummaryMethod getMetricDefaultGroupSumamry(MetricKind t);

} // namespace fnordmetric

