#pragma once
#include "types.hpp"
namespace lob
{
    struct Trade
    {
        TsNs ts_ns{};
        OrderId buy_id{}, sell_id{};
        Px px{};
        Qty qty{};
    };
}