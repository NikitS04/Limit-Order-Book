#pragma once
#include "event.hpp"
#include "trade.hpp"
#include "matcher.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>
#include <algorithm>
namespace lob
{
  inline std::vector<std::string> split_csv_line(const std::string &line)
  {
    std::vector<std::string> out;
    std::string cur;
    std::istringstream iss(line);
    while (std::getline(iss, cur, ','))
      out.push_back(cur);
    return out;
  }
  inline std::optional<Event> parse_event_csv_line(const std::string &line, bool has_header)
  {
    if (has_header)
      return std::nullopt;
    auto cols = split_csv_line(line);
    if (cols.size() < 6)
      return std::nullopt;
    Event e{};
    e.ts_ns = std::stoull(cols[0]);
    char s = cols[1].empty() ? 'B' : cols[1][0];
    e.side = (s == 'B' || s == 'b') ? Side::Buy : Side::Sell;
    std::string type = cols[2];
    std::transform(type.begin(), type.end(), type.begin(), ::toupper);
    if (type == "NEW")
      e.type = Type::Limit;
    else if (type == "MKT")
      e.type = Type::Market;
    else if (type == "CXL")
      e.type = Type::Cancel;
    else if (type == "MOD")
      e.type = Type::Modify;
    else
      return std::nullopt;
    e.id = std::stoull(cols[3]);
    e.px = static_cast<Px>(std::stol(cols[4]));
    e.qty = static_cast<Qty>(std::stol(cols[5]));
    return e;
  }
  inline bool write_trades_csv_header(std::ofstream &ofs)
  {
    ofs << "ts_ns,buy_id,sell_id,price,qty\n";
    return (bool)ofs;
  }
  inline bool append_trade_csv(std::ofstream &ofs, const Trade &t)
  {
    ofs << t.ts_ns << ',' << t.buy_id << ',' << t.sell_id << ',' << t.px << ',' << t.qty << '\n';
    return (bool)ofs;
  }
  inline bool write_depth_csv_header(std::ofstream &ofs)
  {
    ofs << "ts_ns,bid_px,bid_sz,ask_px,ask_sz\n";
    return (bool)ofs;
  }
  inline bool append_depth_csv(std::ofstream &ofs, const DepthL1 &d)
  {
    ofs << d.ts_ns << ',' << d.bid_px << ',' << d.bid_sz << ',' << d.ask_px << ',' << d.ask_sz << '\n';
    return (bool)ofs;
  }
}