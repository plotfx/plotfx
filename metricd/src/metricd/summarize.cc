/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2016 Paul Asmuth, FnordCorp B.V. <paul@asmuth.com>
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <string.h>
#include <map>
#include <metricd/summarize.h>

namespace fnordmetric {

std::string getGrossSummaryName(GrossSummaryMethod method) {
  switch (method) {
    case GrossSummaryMethod::SUM: return "sum";
    case GrossSummaryMethod::MAX: return "max";
    case GrossSummaryMethod::MIN: return "min";
    case GrossSummaryMethod::TREND: return "trend";
  }
}

bool getGrossSummaryFromName(
    GrossSummaryMethod* method,
    const std::string& name) {
  static const std::map<std::string, GrossSummaryMethod> method_map = {
    { "sum", GrossSummaryMethod::SUM },
    { "max", GrossSummaryMethod::MAX },
    { "min", GrossSummaryMethod::MIN },
    { "trend", GrossSummaryMethod::TREND }
  };

  auto iter = method_map.find(name);
  if (iter == method_map.end()) {
    return false;
  } else {
    *method = iter->second;
    return true;
  }
}

ReturnCode summarizeTimeseries(
    GrossSummaryMethod method,
    const DataFrame* frame,
    tval_autoref* result) {
  switch (method) {
    case GrossSummaryMethod::SUM:
        return summarizeTimeseries_SUM(frame, result);
  }
}

ReturnCode summarizeTimeseries_SUM(
    const DataFrame* frame,
    tval_autoref* result) {
  tval_autoref acc(frame->getType());

  for (size_t i = 0; i < frame->getSize(); ++i) {
    tval_add(
        acc.val.type,
        acc.val.data,
        acc.val.len,
        frame->getData(i),
        acc.val.len);
  }

  *result = std::move(acc);
  return ReturnCode::success();
}

SumGroupSummary::SumGroupSummary(tval_type val_type) : acc_(val_type) {}

void SumGroupSummary::addValue(
    tval_type value_type,
    const void* value,
    size_t value_len) {
  if (value_type == tval_type::NIL) {
    return;
  }

  assert(value_type == acc_.val.type);
  assert(value_len == acc_.val.len);
  tval_add(acc_.val.type, acc_.val.data, acc_.val.len, value, value_len);
}

void SumGroupSummary::getValue(
    tval_type value_type,
    void* value,
    size_t value_len) {
  assert(value_type == acc_.val.type);
  assert(value_len == acc_.val.len);
  memcpy(value, acc_.val.data, acc_.val.len);
}

void SumGroupSummary::reset() {
  tval_zero(acc_.val.type, acc_.val.data, acc_.val.len);
}

} // namespace fnordmetric

