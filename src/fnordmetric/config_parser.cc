/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2016 Paul Asmuth, FnordCorp B.V. <paul@asmuth.com>
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <regex>
#include "fnordmetric/config_parser.h"
#include "fnordmetric/util/format.h"

namespace fnordmetric {

ConfigParser::ConfigParser(
    const char* input,
    size_t input_len) :
    input_(input),
    input_cur_(input_),
    input_end_(input_ + input_len),
    has_token_(false),
    has_error_(false) {}

ReturnCode ConfigParser::parse(ConfigList* config) {
  TokenType ttype;
  std::string tbuf;

  /* a file consists of a list of top-level definitions */
  while (getToken(&ttype, &tbuf)) {

    /* parse the "table" definition */
    if (ttype == T_STRING && tbuf == "table") {
      consumeToken();
      if (parseTableDefinition(config)) {
        continue;
      } else {
        break;
      }
    }

    /* parse the "unit" definition */
    if (ttype == T_STRING && tbuf == "unit") {
      consumeToken();
      if (parseUnitDefinition(config)) {
        continue;
      } else {
        break;
      }
    }

    /* parse the "fetch_http" definition */
    if (ttype == T_STRING && tbuf == "fetch_http") {
      consumeToken();
      if (parseFetchHTTPDefinition(config)) {
        continue;
      } else {
        break;
      }
    }

    if (ttype == T_ENDLINE) {
      consumeToken();
      continue;
    }

    setError(
        StringUtil::format(
            "invalid token; got: $0, expected one of: metric",
            printToken(ttype, tbuf)));

    break;
  }

  if (has_error_) {
    return ReturnCode::error(
        "EPARSE",
        StringUtil::format(
            "<$0:$1> $2",
            error_lineno_,
            error_colno_,
            error_msg_));
  } else {
    return ReturnCode::success();
  }
}

bool ConfigParser::parseTableDefinition(ConfigList* config) {
  std::string metric_name;
  if (!expectAndConsumeString(&metric_name)) {
    return false;
  }

  if (!expectAndConsumeToken(T_LCBRACE)) {
    return false;
  }

  TableConfig table_config;

  TokenType ttype;
  std::string tbuf;
  while (getToken(&ttype, &tbuf)) {
    if (ttype == T_RCBRACE) {
      break;
    }

    if (ttype == T_ENDLINE) {
      consumeToken();
      continue;
    }

    /* parse the "interval" stanza */
    if (ttype == T_STRING && tbuf == "interval") {
      consumeToken();
      if (!parseTableDefinitionIntervalStanza(&table_config)) {
        return false;
      }
      continue;
    }

    /* parse the "label" stanza */
    if (ttype == T_STRING && tbuf == "label") {
      consumeToken();
      if (!parseTableDefinitionLabelStanza(&table_config)) {
        return false;
      }
      continue;
    }

    /* parse the "measure" stanza */
    if (ttype == T_STRING && tbuf == "measure") {
      consumeToken();
      if (!parseTableDefinitionMeasureStanza(&table_config)) {
        return false;
      }
      continue;
    }

    setError(
        StringUtil::format(
            "invalid token: $0",
            printToken(ttype, tbuf)));
    return false;
  }

  if (!expectAndConsumeToken(T_RCBRACE)) {
    return false;
  }

  config->addTableConfig(metric_name, table_config);
  return true;
}

bool ConfigParser::parseTableDefinitionIntervalStanza(
    TableConfig* table_config) {
  TokenType ttype;
  std::string tbuf;
  if (!getToken(&ttype, &tbuf) || ttype != T_STRING) {
    setError("interval requires an argument");
    return false;
  }

  consumeToken();

  uint64_t interval = 0;
  auto rc = parseDuration(tbuf, &interval);
  if (!rc.isSuccess()) {
    setError(
        StringUtil::format(
            "invalid value for interval '$0': $1",
            printToken(ttype, tbuf),
            rc.getMessage()));

    return false;
  }

  table_config->interval = interval;
  return true;
}

bool ConfigParser::parseTableDefinitionLabelStanza(
    TableConfig* table_config) {
  TokenType column_name_type;
  std::string column_name;
  if (!getToken(&column_name_type, &column_name) || column_name_type != T_STRING) {
    setError("label requires one or more arguments");
    return false;
  }

  consumeToken();

  table_config->labels.emplace_back(LabelConfig(column_name));
  return true;
}

bool ConfigParser::parseTableDefinitionMeasureStanza(
    TableConfig* table_config) {
  TokenType column_name_type;
  std::string column_name;
  if (!getToken(&column_name_type, &column_name) || column_name_type != T_STRING) {
    setError("measure requires two arguments");
    return false;
  }

  consumeToken();

  TokenType aggregation_type;
  std::string aggregation;
  if (!getToken(&aggregation_type, &aggregation) || aggregation_type != T_STRING) {
    setError("measure requires two arguments");
    return false;
  }

  consumeToken();

  table_config->labels.emplace_back(LabelConfig(column_name));
  return true;
}

bool ConfigParser::parseUnitDefinition(ConfigList* config) {
  UnitConfig unit_config;
  if (!expectAndConsumeString(&unit_config.unit_id)) {
    return false;
  }

  if (!expectAndConsumeToken(T_LCBRACE)) {
    return false;
  }


  TokenType ttype;
  std::string tbuf;
  while (getToken(&ttype, &tbuf)) {
    if (ttype == T_RCBRACE) {
      break;
    }

    if (ttype == T_ENDLINE) {
      consumeToken();
      continue;
    }

    /* parse the "unit_desc" stanza */
    if (ttype == T_STRING && tbuf == "unit_desc") {
      consumeToken();
      if (!parseUnitDefinitionDescriptionStanza(&unit_config)) {
        return false;
      }
      continue;
    }

    /* parse the "unit_name" stanza */
    if (ttype == T_STRING && tbuf == "unit_name") {
      consumeToken();
      if (!parseUnitDefinitionNameStanza(&unit_config)) {
        return false;
      }
      continue;
    }

    setError(
        StringUtil::format(
            "invalid token: $0, expected one of: unit_desc, unit_name",
            printToken(ttype, tbuf)));
    return false;
  }

  if (!expectAndConsumeToken(T_RCBRACE)) {
    return false;
  }

  config->addUnitConfig(std::move(unit_config));
  return true;
}

bool ConfigParser::parseUnitDefinitionDescriptionStanza(
    UnitConfig* unit_config) {
  TokenType ttype;
  std::string tbuf;
  if (!getToken(&ttype, &tbuf) || ttype != T_STRING) {
    setError("unit_desc requires an argument");
    return false;
  }

  unit_config->description = tbuf;
  consumeToken();
  return true;
}

bool ConfigParser::parseUnitDefinitionNameStanza(
    UnitConfig* unit_config) {
  static const std::string kArgError =
      "unit_name requires 5 arguments: unit_name <name> <factor> <singular> <plural> <symbol>";

  UnitNameConfig unc;

  TokenType name_type;
  std::string unit_name;
  if ((!getToken(&name_type, &unit_name) || name_type != T_STRING))  {
    setError(kArgError);
    return false;
  }
  consumeToken();

  TokenType factor_type;
  if ((!getToken(&factor_type, &unc.factor) || factor_type != T_STRING))  {
    setError(kArgError);
    return false;
  }
  consumeToken();

  TokenType singular_type;
  if ((!getToken(&singular_type, &unc.singular) || singular_type != T_STRING))  {
    setError(kArgError);
    return false;
  }
  consumeToken();

  TokenType plural_type;
  std::string plural_buf;
  if ((!getToken(&plural_type, &unc.plural) || plural_type != T_STRING))  {
    setError(kArgError);
    return false;
  }
  consumeToken();

  TokenType symbol_type;
  std::string symbol_buf;
  if ((!getToken(&symbol_type, &unc.symbol) || symbol_type != T_STRING))  {
    setError(kArgError);
    return false;
  }
  consumeToken();

  unit_config->names.emplace(unit_name, std::move(unc));
  return true;
}

bool ConfigParser::parseRewriteStanza(
    SensorConfig* sensor_config) {
  TokenType ttype;
  std::string regex_str;
  if (!getToken(&ttype, &regex_str) || ttype != T_STRING) {
    setError("metric_id_rewrite requires two arguments");
    return false;
  }

  consumeToken();

  std::string replace_str;
  if (!getToken(&ttype, &replace_str) || ttype != T_STRING) {
    setError("metric_id_rewrite requires two arguments");
    return false;
  }

  std::regex regex;
  try {
    regex = std::regex(regex_str);
  } catch (const std::exception& e) {
    setError(StringUtil::format("invalid regex: $0", e.what()));
    return false;
  }

  consumeToken();

  sensor_config->metric_id_rewrite_enabled = true;
  sensor_config->metric_id_rewrite_regex = regex;
  sensor_config->metric_id_rewrite_replace = replace_str;

  return true;
}

bool ConfigParser::parseFetchHTTPDefinition(ConfigList* config) {
  std::unique_ptr<HTTPSensorConfig> sensor_config(new HTTPSensorConfig());
  if (!expectAndConsumeString(&sensor_config->sensor_id)) {
    return false;
  }

  if (!expectAndConsumeToken(T_LCBRACE)) {
    return false;
  }


  TokenType ttype;
  std::string tbuf;
  while (getToken(&ttype, &tbuf)) {
    if (ttype == T_RCBRACE) {
      break;
    }

    if (ttype == T_ENDLINE) {
      consumeToken();
      continue;
    }

    /* parse the "http_url" stanza */
    if (ttype == T_STRING && tbuf == "http_url") {
      consumeToken();
      if (!parseFetchHTTPDefinitionURLStanza(sensor_config.get())) {
        return false;
      }
      continue;
    }

    /* parse the "metric_id_rewrite" stanza */
    if (ttype == T_STRING && tbuf == "metric_id_rewrite") {
      consumeToken();
      if (!parseRewriteStanza(sensor_config.get())) {
        return false;
      }
      continue;
    }

    setError(
        StringUtil::format(
            "invalid token: $0, expected one of: unit_desc, unit_name",
            printToken(ttype, tbuf)));
    return false;
  }

  if (!expectAndConsumeToken(T_RCBRACE)) {
    return false;
  }

  config->addSensorConfig(std::move(sensor_config));
  return true;
}

bool ConfigParser::parseFetchHTTPDefinitionURLStanza(
    HTTPSensorConfig* sensor_config) {
  TokenType ttype;
  std::string tbuf;
  if (!getToken(&ttype, &tbuf) || ttype != T_STRING) {
    setError("http_url requires an argument");
    return false;
  }

  sensor_config->http_url = tbuf;
  consumeToken();
  return true;
}

bool ConfigParser::getToken(
    TokenType* ttype,
    std::string* tbuf) {
  const char* tbuf_cstr = nullptr;
  size_t tbuf_len = 0;

  tbuf->clear();

  bool ret = getToken(ttype, &tbuf_cstr, &tbuf_len);
  if (tbuf_cstr) {
    tbuf->append(tbuf_cstr, tbuf_len);
  }
  return ret;
}

bool ConfigParser::getToken(
    TokenType* ttype,
    const char** tbuf,
    size_t* tbuf_len) {
  char quote_char = 0;

  if (has_token_) {
    goto return_token;
  }

  /* skip whitespace */
  while (input_cur_ < input_end_) {
    if (*input_cur_ == ' ' ||
        *input_cur_ == '\t' ||
        *input_cur_ == '\r') {
      input_cur_++;
    } else {
      break;
    }
  }

  if (input_cur_ >= input_end_) {
    return false;
  }

  /* single character tokens */
  switch (*input_cur_) {

    case '\n': {
      token_type_ = T_ENDLINE;
      input_cur_++;
      goto return_token;
    }

    case ',': {
      token_type_ = T_COMMA;
      input_cur_++;
      goto return_token;
    }

    case '(': {
      token_type_ = T_LPAREN;
      input_cur_++;
      goto return_token;
    }

    case ')': {
      token_type_ = T_RPAREN;
      input_cur_++;
      goto return_token;
    }

    case '{': {
      token_type_ = T_LCBRACE;
      input_cur_++;
      goto return_token;
    }

    case '}': {
      token_type_ = T_RCBRACE;
      input_cur_++;
      goto return_token;
    }

    /* quoted strings */
    case '"':
    case '\'':
      quote_char = *input_cur_;
      input_cur_++;
      break;

    /* unquoted strings */
    default:
      break;
  }

  /* [un]quoted strings */
  token_type_ = T_STRING;

  if (quote_char) {
    bool escaped = false;
    bool eof = false;
    for (; !eof && input_cur_ < input_end_; input_cur_++) {
      auto chr = *input_cur_;
      switch (chr) {

        case '"':
        case '\'':
          if (escaped || quote_char != chr) {
            token_buf_ += chr;
            break;
          } else {
            eof = true;
            continue;
          }

        case '\\':
          if (escaped) {
            token_buf_ += "\\";
            break;
          } else {
            escaped = true;
            continue;
          }

        default:
          token_buf_ += chr;
          break;

      }

      escaped = false;
    }

    quote_char = 0;
    goto return_token;
  } else {
    while (
        *input_cur_ != ' ' &&
        *input_cur_ != '\t' &&
        *input_cur_ != '\n' &&
        *input_cur_ != '\r' &&
        *input_cur_ != ',' &&
        *input_cur_ != '(' &&
        *input_cur_ != ')' &&
        *input_cur_ != '{' &&
        *input_cur_ != '}' &&
        *input_cur_ != '"' &&
        *input_cur_ != '\'' &&
        input_cur_ < input_end_) {
      token_buf_ += *input_cur_;
      input_cur_++;
    }

    goto return_token;
  }

return_token:
  has_token_ = true;
  *ttype = token_type_;
  *tbuf = token_buf_.data();
  *tbuf_len = token_buf_.size();
  return true;
}

void ConfigParser::consumeToken() {
  has_token_ = false;
  token_buf_.clear();
}

bool ConfigParser::expectAndConsumeToken(TokenType desired_type) {
  TokenType actual_type;
  const char* tbuf = nullptr;
  size_t tbuf_len = 0;

  if (!getToken(&actual_type, &tbuf, &tbuf_len)) {
    return false;
  }

  if (actual_type != desired_type) {
    setError(
        StringUtil::format(
            "unexpected token; expected: $0, got: $1",
            printToken(desired_type),
            printToken(actual_type, tbuf, tbuf_len)));

    return false;
  }

  consumeToken();
  return true;
}

bool ConfigParser::expectAndConsumeString(std::string* buf) {
  TokenType ttype;
  if (!getToken(&ttype, buf)) {
    return false;
  }

  if (ttype != T_STRING) {
    setError(
        StringUtil::format(
            "unexpected token; expected: STRING, got: $0",
            printToken(ttype, *buf)));

    return false;
  }

  consumeToken();
  return true;
}

std::string ConfigParser::printToken(TokenType type) {
  return printToken(type, nullptr, 0);
}

std::string ConfigParser::printToken(
    TokenType type,
    const std::string& buf) {
  return printToken(type, buf.data(), buf.size());
}

std::string ConfigParser::printToken(
    TokenType type,
    const char* buf,
    size_t buf_len) {
  std::string out;
  switch (type) {
    case T_STRING: out = "STRING"; break;
    case T_COMMA: out = "COMMA"; break;
    case T_ENDLINE: out = "ENDLINE"; break;
    case T_LPAREN: out = "LPAREN"; break;
    case T_RPAREN: out = "RPAREN"; break;
    case T_LCBRACE: out = "LCBRACE"; break;
    case T_RCBRACE: out = "RCBRACE"; break;
  }

  if (buf && buf_len > 0) {
    out += "<";
    out += std::string(buf, buf_len);
    out += ">";
  }

  return out;
}

void ConfigParser::setError(const std::string& error) {
  has_error_ = true;
  error_msg_ = error;
  error_lineno_ = 0;
  error_colno_ = 0;
}

} // namespace fnordmetric
