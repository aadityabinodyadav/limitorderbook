#pragma once
#include <cstdint>

namespace trading
{
    using OrderId = uint64_t;
    using Price = uint64_t;
    using Quantity = uint32_t;

    enum class Side : uint8_t
    {
        BUY = 0,
        SIDE = 1
    };

    struct Order
    {
        OrderId  id;
        Price    price;
        Quantity quantity;
        Quantity filled_quantity;
        Side     side;

        Order(OrderId id_, Price price_, Quantity quantity_, Side side_)
            : id(id_),
              price(price_),
              quantity(quantity_),
              filled_quantity(0),
              side(side_)
        {
        }

        bool is_fully_filled() const
        {
            return filled_quantity >= quantity;
        }

        Quantity remaining_quantity() const
        {
            return quantity - filled_quantity;
        }
    };
}