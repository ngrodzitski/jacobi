#pragma once

#include <cstdint>
#include <span>
#include <vector>
#include <string>

#include <jacobi/book/vocabulary_types.hpp>

namespace jacobi::snapshots
{

//
// book_operation
//

enum class book_operation : std::uint8_t
{
    add_order    = 0x0,
    exec_order   = 0x1,
    reduce_order = 0x2,
    modify_order = 0x3,
    delete_order = 0x4,
};

//
// common_event_fields_t
//

#include "packed_struct_begin.ipp"
struct JACOBI_BENCH_PACKED common_event_fields_t
{
    // MSVC comforting:
    inline static constexpr book_operation op =
        static_cast< book_operation >( static_cast< std::uint8_t >( 255 ) );

    std::uint64_t id;

    [[nodiscard]] book::order_id_t order_id() const noexcept
    {
        return book::order_id_t{ id };
    }
};

//
// update_add_order_t
//

struct JACOBI_BENCH_PACKED update_add_order_t : public common_event_fields_t
{
    inline static constexpr book_operation op = book_operation::add_order;
    std::uint32_t qty;
    std::uint32_t padding0;
    std::int64_t price;

    [[nodiscard]] book::order_t make_order() const noexcept
    {
        return book::order_t{ .id    = order_id(),
                              .qty   = book::order_qty_t{ qty },
                              .price = book::order_price_t{ price } };
    }
};

//
// update_exec_order_t
//

struct JACOBI_BENCH_PACKED update_exec_order_t : public common_event_fields_t
{
    inline static constexpr book_operation op = book_operation::exec_order;

    std::uint32_t q;

    [[nodiscard]] book::order_qty_t exec_qty() const noexcept
    {
        return book::order_qty_t{ q };
    }
};

//
// update_reduce_order_t
//

struct JACOBI_BENCH_PACKED update_reduce_order_t : public common_event_fields_t
{
    inline static constexpr book_operation op = book_operation::reduce_order;
    std::uint32_t q;

    [[nodiscard]] book::order_qty_t canceled_qty() const noexcept
    {
        return book::order_qty_t{ q };
    }
};

//
// update_modify_order_t
//

struct JACOBI_BENCH_PACKED update_modify_order_t : public update_add_order_t
{
    inline static constexpr book_operation op = book_operation::modify_order;
};

//
// update_delete_order_t
//

struct JACOBI_BENCH_PACKED update_delete_order_t : public common_event_fields_t
{
    inline static constexpr book_operation op = book_operation::delete_order;
    std::uint64_t order_id;
};

//
// update_xxx_t
//

/**
 * @brief A generic update storage.
 */
union JACOBI_BENCH_PACKED update_xxx_t
{
    common_event_fields_t common;
    update_add_order_t add_order;
    update_exec_order_t exec_order;
    update_reduce_order_t reduce_order;
    update_modify_order_t modify_order;
    update_delete_order_t delete_order;
};

//
// update_record_image_t
//

struct JACOBI_BENCH_PACKED update_record_image_t
{
    std::uint32_t book_id;
    std::uint8_t op_code;
    std::uint8_t ts;
    std::uint8_t padding[ 2 ];

    update_xxx_t u;

    [[nodiscard]] book::order_id_t order_id() const noexcept
    {
        return u.common.order_id();
    }

    [[nodiscard]] book::trade_side trade_side() const noexcept
    {
        return ts == 0 ? book::trade_side::sell : book::trade_side::buy;
    }
};

static_assert( sizeof( update_record_image_t ) == 32 );

//
// decode_update()
//

template < typename Handler >
std::span< const std::byte > decode_update( std::span< const std::byte > buf,
                                            Handler handler )
{
    update_record_image_t update_record;
    std::memcpy( &update_record, buf.data(), sizeof( update_record ) );

    // Call the handler:
    handler( update_record );

    // Return the rest of the buffer:
    return buf.subspan( sizeof( update_record ) );
}

#include "packed_struct_end.ipp"

//
// read_events_from_file()
//

[[nodiscard]] std::vector< update_record_image_t > read_events_from_file(
    const std::string & filename );

}  // namespace jacobi::snapshots
