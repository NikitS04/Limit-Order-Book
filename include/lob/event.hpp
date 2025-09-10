#pragma once
#include "types.hpp"
namespace lob
{
    struct Event
    {
        TsNs ts_ns{};
        Type type{Type::Limit};
        Side side{Side::Buy};
        OrderId id{};
        Px px{};
        Qty qty{};
    };
}