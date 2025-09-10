#pragma once
#include <cstdint>
namespace lob
{
    enum class Side : uint8_t
    {
        Buy,
        Sell
    };
    enum class Type : uint8_t
    {
        Limit,
        Market,
        Cancel,
        Modify
    };
    using OrderId = uint64_t;
    using Px = int32_t;
    using Qty = int32_t;
    using TsNs = uint64_t;
}