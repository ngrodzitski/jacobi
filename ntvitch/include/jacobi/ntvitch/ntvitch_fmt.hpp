#pragma once

#include <algorithm>

#include <fmt/core.h>

#include <jacobi/ntvitch/ntvitch.hpp>

namespace jacobi::ntvitch
{

namespace details
{

//
// fmt_parser_format_base_t
//

/**
 * @brief A base class for fmt-integrattion via specialization.
 *
 * Allows to avoid code duplication for a parse() method
 * in case no spcific format options are needed and so
 * only "{}" format strning is expected.
 *
 * Example usage:
 * @code
 * template <>
 * struct formatter< my_type_t >
 *     : public jacobi::ntvitch::fmt_parser_format_base_t
 * {
 *     template < class Format_Context >
 *     auto format( const my_type_t & ec, Format_Context & ctx ) const
 *     {
 *         // ...
 *     }
 * };
 * @endcode
 */
struct fmt_parser_format_base_t
{
    template < class Parse_Context >
    constexpr auto parse( Parse_Context & ctx )
    {
        return ctx.begin();
    }
};

}  // namespace details

}  // namespace jacobi::ntvitch

namespace fmt
{

template <>
struct formatter< jacobi::ntvitch::message_type >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::message_type & type,
                 Format_Context & ctx ) const
    {
        using message_type = jacobi::ntvitch::message_type;
        switch( type )
        {
            case message_type::system_event:
                return fmt::format_to( ctx.out(), "system_event" );
            case message_type::stock_directory:
                return fmt::format_to( ctx.out(), "stock_directory" );
            case message_type::stock_trading_action:
                return fmt::format_to( ctx.out(), "stock_trading_action" );
            case message_type::add_order:
                return fmt::format_to( ctx.out(), "add_order" );
            case message_type::add_order_mpid:
                return fmt::format_to( ctx.out(), "add_order_mpid" );
            case message_type::order_executed:
                return fmt::format_to( ctx.out(), "order_executed" );
            case message_type::order_executed_with_price:
                return fmt::format_to( ctx.out(), "order_executed_with_price" );
            case message_type::order_cancel:
                return fmt::format_to( ctx.out(), "order_cancel" );
            case message_type::order_delete:
                return fmt::format_to( ctx.out(), "order_delete" );
            case message_type::order_replace:
                return fmt::format_to( ctx.out(), "order_replace" );
            case message_type::trade_non_cross:
                return fmt::format_to( ctx.out(), "trade_non_cross" );
            case message_type::cross_trade:
                return fmt::format_to( ctx.out(), "cross_trade" );
            case message_type::broken_trade:
                return fmt::format_to( ctx.out(), "broken_trade" );
            case message_type::noii:
                return fmt::format_to( ctx.out(), "noii" );
        }

        return fmt::format_to( ctx.out(),
                               "message_type_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( type ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::system_event_code >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::system_event_code & code,
                 Format_Context & ctx ) const
    {
        using system_event_code = jacobi::ntvitch::system_event_code;
        switch( code )
        {
            case system_event_code::start_of_messages:
                return fmt::format_to( ctx.out(), "start_of_messages" );
            case system_event_code::start_of_system:
                return fmt::format_to( ctx.out(), "start_of_system" );
            case system_event_code::start_of_market:
                return fmt::format_to( ctx.out(), "start_of_market" );
            case system_event_code::end_of_market:
                return fmt::format_to( ctx.out(), "end_of_market" );
            case system_event_code::end_of_system:
                return fmt::format_to( ctx.out(), "end_of_system" );
            case system_event_code::end_of_messages:
                return fmt::format_to( ctx.out(), "end_of_messages" );
        }

        return fmt::format_to( ctx.out(),
                               "system_event_code_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( code ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::stock_market_category >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::stock_market_category & category,
                 Format_Context & ctx ) const
    {
        using stock_market_category = jacobi::ntvitch::stock_market_category;
        switch( category )
        {
            case stock_market_category::nasdaq_global_select_market:
                return fmt::format_to( ctx.out(), "nasdaq_global_select_market" );
            case stock_market_category::nasdaq_global_market:
                return fmt::format_to( ctx.out(), "nasdaq_global_market" );
            case stock_market_category::nasdaq_capital_market:
                return fmt::format_to( ctx.out(), "nasdaq_capital_market" );
            case stock_market_category::nyse:
                return fmt::format_to( ctx.out(), "nyse" );
            case stock_market_category::nyse_american:
                return fmt::format_to( ctx.out(), "nyse_american" );
            case stock_market_category::nyse_arca:
                return fmt::format_to( ctx.out(), "nyse_arca" );
            case stock_market_category::bats_z_exchang:
                return fmt::format_to( ctx.out(), "bats_z_exchang" );
            case stock_market_category::investors_exchange:
                return fmt::format_to( ctx.out(), "investors_exchange" );
            case stock_market_category::not_available:
                return fmt::format_to( ctx.out(), "not_available" );
        }

        return fmt::format_to( ctx.out(),
                               "stock_market_category_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( category ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::stock_financial_status >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::stock_financial_status & status,
                 Format_Context & ctx ) const
    {
        using stock_financial_status = jacobi::ntvitch::stock_financial_status;
        switch( status )
        {
            case stock_financial_status::deficient:
                return fmt::format_to( ctx.out(), "deficient" );
            case stock_financial_status::delinquent:
                return fmt::format_to( ctx.out(), "delinquent" );
            case stock_financial_status::bankrupt:
                return fmt::format_to( ctx.out(), "bankrupt" );
            case stock_financial_status::suspended:
                return fmt::format_to( ctx.out(), "suspended" );
            case stock_financial_status::deficient_and_bankrupt:
                return fmt::format_to( ctx.out(), "deficient_and_bankrupt" );
            case stock_financial_status::deficient_and_delinquent:
                return fmt::format_to( ctx.out(), "deficient_and_delinquent" );
            case stock_financial_status::delinquent_and_bankrupt:
                return fmt::format_to( ctx.out(), "delinquent_and_bankrupt" );
            case stock_financial_status::deficient_delinquent_and_bankrupt:
                return fmt::format_to( ctx.out(),
                                       "deficient_delinquent_and_bankrupt" );
            case stock_financial_status::creations_andor_redemptions_suspended:
                return fmt::format_to( ctx.out(),
                                       "creations_andor_redemptions_suspended" );
            case stock_financial_status::normal:
                // Note: 'nyse' shares the exact same underlying char value ('N').
                // It is combined here to prevent a duplicate case compiler error.
                return fmt::format_to( ctx.out(), "normal/nyse" );
            case stock_financial_status::nyse_american:
                return fmt::format_to( ctx.out(), "nyse_american" );
            case stock_financial_status::nyse_arca:
                return fmt::format_to( ctx.out(), "nyse_arca" );
            case stock_financial_status::bats_z_exchang:
                return fmt::format_to( ctx.out(), "bats_z_exchang" );
            case stock_financial_status::investors_exchange:
                return fmt::format_to( ctx.out(), "investors_exchange" );
            case stock_financial_status::not_available:
                return fmt::format_to( ctx.out(), "not_available" );
        }

        return fmt::format_to( ctx.out(),
                               "stock_financial_status_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( status ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::stock_round_lots_flag >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::stock_round_lots_flag & flag,
                 Format_Context & ctx ) const
    {
        using stock_round_lots_flag = jacobi::ntvitch::stock_round_lots_flag;
        switch( flag )
        {
            case stock_round_lots_flag::round_lots_only:
                return fmt::format_to( ctx.out(), "round_lots_only" );
            case stock_round_lots_flag::any_order_size:
                return fmt::format_to( ctx.out(), "any_order_size" );
        }

        return fmt::format_to( ctx.out(),
                               "stock_round_lots_flag_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( flag ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::stock_issue_classification >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format(
        const jacobi::ntvitch::stock_issue_classification & classification,
        Format_Context & ctx ) const
    {
        using stock_issue_classification =
            jacobi::ntvitch::stock_issue_classification;
        switch( classification )
        {
            case stock_issue_classification::american_depositary_share:
                return fmt::format_to( ctx.out(), "american_depositary_share" );
            case stock_issue_classification::bond:
                return fmt::format_to( ctx.out(), "bond" );
            case stock_issue_classification::common_stock:
                return fmt::format_to( ctx.out(), "common_stock" );
            case stock_issue_classification::depository_receipt:
                return fmt::format_to( ctx.out(), "depository_receipt" );
            case stock_issue_classification::the_144a:
                return fmt::format_to( ctx.out(), "the_144a" );
            case stock_issue_classification::limited_partnership:
                return fmt::format_to( ctx.out(), "limited_partnership" );
            case stock_issue_classification::notes:
                return fmt::format_to( ctx.out(), "notes" );
            case stock_issue_classification::ordinaryshare:
                return fmt::format_to( ctx.out(), "ordinaryshare" );
            case stock_issue_classification::preferredstock:
                return fmt::format_to( ctx.out(), "preferredstock" );
            case stock_issue_classification::other_securities:
                return fmt::format_to( ctx.out(), "other_securities" );
            case stock_issue_classification::right_:
                return fmt::format_to( ctx.out(), "right_" );
            case stock_issue_classification::shares_of_beneficial_interest:
                return fmt::format_to( ctx.out(),
                                       "shares_of_beneficial_interest" );
            case stock_issue_classification::convertible_debenture:
                return fmt::format_to( ctx.out(), "convertible_debenture" );
            case stock_issue_classification::unit:
                return fmt::format_to( ctx.out(), "unit" );
            case stock_issue_classification::units_benif_int:
                return fmt::format_to( ctx.out(), "units_benif_int" );
            case stock_issue_classification::warrant:
                return fmt::format_to( ctx.out(), "warrant" );
        }

        return fmt::format_to( ctx.out(),
                               "stock_issue_classification_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( classification ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::stock_authenticity >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::stock_authenticity & authenticity,
                 Format_Context & ctx ) const
    {
        using stock_authenticity = jacobi::ntvitch::stock_authenticity;
        switch( authenticity )
        {
            case stock_authenticity::live_production:
                return fmt::format_to( ctx.out(), "live_production" );
            case stock_authenticity::test:
                return fmt::format_to( ctx.out(), "test" );
        }

        return fmt::format_to( ctx.out(),
                               "stock_authenticity_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( authenticity ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::stock_short_sale_threshold >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::stock_short_sale_threshold & threshold,
                 Format_Context & ctx ) const
    {
        using stock_short_sale_threshold =
            jacobi::ntvitch::stock_short_sale_threshold;
        switch( threshold )
        {
            case stock_short_sale_threshold::restricted:
                return fmt::format_to( ctx.out(), "restricted" );
            case stock_short_sale_threshold::not_restricted:
                return fmt::format_to( ctx.out(), "not_restricted" );
            case stock_short_sale_threshold::not_available:
                return fmt::format_to( ctx.out(), "not_available" );
        }

        return fmt::format_to( ctx.out(),
                               "stock_short_sale_threshold_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( threshold ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::stock_ipo_flag >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::stock_ipo_flag & flag,
                 Format_Context & ctx ) const
    {
        using stock_ipo_flag = jacobi::ntvitch::stock_ipo_flag;
        switch( flag )
        {
            case stock_ipo_flag::is_new:
                return fmt::format_to( ctx.out(), "is_new" );
            case stock_ipo_flag::not_new:
                return fmt::format_to( ctx.out(), "not_new" );
            case stock_ipo_flag::not_available:
                return fmt::format_to( ctx.out(), "not_available" );
        }

        return fmt::format_to( ctx.out(),
                               "stock_ipo_flag_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( flag ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::stock_luld_reference >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::stock_luld_reference & reference,
                 Format_Context & ctx ) const
    {
        using stock_luld_reference = jacobi::ntvitch::stock_luld_reference;
        switch( reference )
        {
            case stock_luld_reference::tier_1:
                return fmt::format_to( ctx.out(), "tier_1" );
            case stock_luld_reference::tier_2:
                return fmt::format_to( ctx.out(), "tier_2" );
            case stock_luld_reference::not_available:
                return fmt::format_to( ctx.out(), "not_available" );
        }

        return fmt::format_to( ctx.out(),
                               "stock_luld_reference_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( reference ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::stok_etp_flag >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::stok_etp_flag & flag,
                 Format_Context & ctx ) const
    {
        using stok_etp_flag = jacobi::ntvitch::stok_etp_flag;
        switch( flag )
        {
            case stok_etp_flag::is_etp:
                return fmt::format_to( ctx.out(), "is_etp" );
            case stok_etp_flag::not_etp:
                return fmt::format_to( ctx.out(), "not_etp" );
            case stok_etp_flag::not_available:
                return fmt::format_to( ctx.out(), "not_available" );
        }

        return fmt::format_to( ctx.out(),
                               "stok_etp_flag_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( flag ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::stock_inverse_indicator >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::stock_inverse_indicator & indicator,
                 Format_Context & ctx ) const
    {
        using stock_inverse_indicator = jacobi::ntvitch::stock_inverse_indicator;
        switch( indicator )
        {
            case stock_inverse_indicator::is_inverseetp:
                return fmt::format_to( ctx.out(), "is_inverseetp" );
            case stock_inverse_indicator::notinverse_etp:
                return fmt::format_to( ctx.out(), "notinverse_etp" );
            case stock_inverse_indicator::not_available:
                return fmt::format_to( ctx.out(), "not_available" );
        }

        return fmt::format_to( ctx.out(),
                               "stock_inverse_indicator_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( indicator ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::stock_trading_state >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::stock_trading_state & state,
                 Format_Context & ctx ) const
    {
        using stock_trading_state = jacobi::ntvitch::stock_trading_state;
        switch( state )
        {
            case stock_trading_state::halted:
                return fmt::format_to( ctx.out(), "halted" );
            case stock_trading_state::paused:
                return fmt::format_to( ctx.out(), "paused" );
            case stock_trading_state::quotation_only:
                return fmt::format_to( ctx.out(), "quotation_only" );
            case stock_trading_state::trading:
                return fmt::format_to( ctx.out(), "trading" );
        }

        return fmt::format_to( ctx.out(),
                               "stock_trading_state_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( state ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::buy_sell_indicator >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::buy_sell_indicator & indicator,
                 Format_Context & ctx ) const
    {
        using buy_sell_indicator = jacobi::ntvitch::buy_sell_indicator;
        switch( indicator )
        {
            case buy_sell_indicator::buy:
                return fmt::format_to( ctx.out(), "buy" );
            case buy_sell_indicator::sell:
                return fmt::format_to( ctx.out(), "sell" );
        }

        return fmt::format_to( ctx.out(),
                               "buy_sell_indicator_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( indicator ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::printable_flag >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::printable_flag & flag,
                 Format_Context & ctx ) const
    {
        using printable_flag = jacobi::ntvitch::printable_flag;
        switch( flag )
        {
            case printable_flag::non_printable:
                return fmt::format_to( ctx.out(), "non_printable" );
            case printable_flag::printable:
                return fmt::format_to( ctx.out(), "printable" );
        }

        return fmt::format_to( ctx.out(),
                               "printable_flag_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( flag ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::cross_type >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::cross_type & type,
                 Format_Context & ctx ) const
    {
        using cross_type = jacobi::ntvitch::cross_type;
        switch( type )
        {
            case cross_type::opening:
                return fmt::format_to( ctx.out(), "opening" );
            case cross_type::closing:
                return fmt::format_to( ctx.out(), "closing" );
            case cross_type::halt_or_ipo:
                return fmt::format_to( ctx.out(), "halt_or_ipo" );
            case cross_type::intraday:
                return fmt::format_to( ctx.out(), "intraday" );
        }

        return fmt::format_to( ctx.out(),
                               "cross_type_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( type ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::imbalance_direction >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::imbalance_direction & direction,
                 Format_Context & ctx ) const
    {
        using imbalance_direction = jacobi::ntvitch::imbalance_direction;
        switch( direction )
        {
            case imbalance_direction::buy:
                return fmt::format_to( ctx.out(), "buy" );
            case imbalance_direction::sell:
                return fmt::format_to( ctx.out(), "sell" );
            case imbalance_direction::no_imbalance:
                return fmt::format_to( ctx.out(), "no_imbalance" );
            case imbalance_direction::insufficient_orders:
                return fmt::format_to( ctx.out(), "insufficient_orders" );
        }

        return fmt::format_to( ctx.out(),
                               "imbalance_direction_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( direction ) );
    }
};

template <>
struct formatter< jacobi::ntvitch::price_variation_indicator >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::price_variation_indicator & indicator,
                 Format_Context & ctx ) const
    {
        using price_variation_indicator =
            jacobi::ntvitch::price_variation_indicator;
        switch( indicator )
        {
            case price_variation_indicator::less_than_1_percent:
                return fmt::format_to( ctx.out(), "less_than_1_percent" );
            case price_variation_indicator::from_1_to_1_99:
                return fmt::format_to( ctx.out(), "from_1_to_1_99" );
            case price_variation_indicator::from_2_to_2_99:
                return fmt::format_to( ctx.out(), "from_2_to_2_99" );
            case price_variation_indicator::from_3_to_3_99:
                return fmt::format_to( ctx.out(), "from_3_to_3_99" );
            case price_variation_indicator::from_4_to_4_99:
                return fmt::format_to( ctx.out(), "from_4_to_4_99" );
            case price_variation_indicator::from_5_to_5_99:
                return fmt::format_to( ctx.out(), "from_5_to_5_99" );
            case price_variation_indicator::from_6_to_6_99:
                return fmt::format_to( ctx.out(), "from_6_to_6_99" );
            case price_variation_indicator::from_7_to_7_99:
                return fmt::format_to( ctx.out(), "from_7_to_7_99" );
            case price_variation_indicator::from_8_to_8_99:
                return fmt::format_to( ctx.out(), "from_8_to_8_99" );
            case price_variation_indicator::from_9_to_9_99:
                return fmt::format_to( ctx.out(), "from_9_to_9_99" );
            case price_variation_indicator::from_10_to_19_99:
                return fmt::format_to( ctx.out(), "from_10_to_19_99" );
            case price_variation_indicator::from_20_to_29_99:
                return fmt::format_to( ctx.out(), "from_20_to_29_99" );
            case price_variation_indicator::from_30_or_greater:
                return fmt::format_to( ctx.out(), "from_30_or_greater" );
            case price_variation_indicator::not_available:
                return fmt::format_to( ctx.out(), "not_available" );
        }

        return fmt::format_to( ctx.out(),
                               "price_variation_indicator_unknown(0x{:02X})",
                               static_cast< std::uint8_t >( indicator ) );
    }
};

//====================================================================
// MESSAGES:

template <>
struct formatter< jacobi::ntvitch::message_base_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::message_base_t & msg,
                 Format_Context & ctx ) const
    {
        const auto nsec = msg.timestamp_ns() % 1'000'000'000ULL;
        auto seconds    = msg.timestamp_ns() / 1'000'000'000ULL;
        auto minutes    = seconds / 60;
        auto hours      = minutes / 60;
        minutes         = minutes % 60;
        seconds         = seconds % 60;

        return fmt::format_to( ctx.out(),
                               "{{type: {}, tracking:{}, stock_locate: {}, "
                               "timestamp:{:02}:{:02}:{:02}.{:09}}}",
                               msg.msg_type(),
                               msg.tracking_number(),
                               msg.stock_locate(),
                               hours,
                               minutes,
                               seconds,
                               nsec );
    }
};

template <>
struct formatter< jacobi::ntvitch::system_event_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::system_event_t & msg,
                 Format_Context & ctx ) const
    {

        return fmt::format_to(
            ctx.out(),
            "{} {{sys_event_code: {}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.sys_event_code() );
    }
};

template <>
struct formatter< jacobi::ntvitch::stock_directory_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::stock_directory_t & msg,
                 Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{stock: '{}', market_category: {}, financial_status: {}, "
            "round_lot_size: {}, round_lots: {}, issue_classification: {}, "
            "issue_subtype: '{}', authenticity: {}, short_sale_threshold: {}, "
            "ipo_flag: {}, luld_reference: {}, etp_flag: {}, "
            "etp_leverage: {}, inverse_indicator: {}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.stock().substr( 0, msg.stock().find( ' ' ) ),
            msg.market_category(),
            msg.financial_status_indicator(),
            msg.round_lot_size(),
            msg.round_lots(),
            msg.issue_classification(),
            std::string_view( msg.issue_subtype().begin(),
                              find( msg.issue_subtype().begin(),
                                    msg.issue_subtype().end(),
                                    ' ' ) ),
            msg.authenticity(),
            msg.short_sale_threshold_indicator(),
            msg.ipo_flag(),
            msg.luld_reference(),
            msg.etp_flag(),
            msg.etp_leverage_factor(),
            msg.inverse_indicator() );
    }
};

template <>
struct formatter< jacobi::ntvitch::stock_trading_action_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::stock_trading_action_t & msg,
                 Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{stock: '{}', trading_state: {}, reason: '{}'}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.stock().substr( 0, msg.stock().find( ' ' ) ),
            msg.trading_state(),
            msg.reason() );
    }
};

template <>
struct formatter< jacobi::ntvitch::add_order_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::add_order_t & msg,
                 Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{order_reference_number: {:X}, side: {}, shares: {}, stock: "
            "'{}', "
            "price: {}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.order_reference_number(),
            msg.buy_sell(),
            msg.shares(),
            msg.stock().substr( 0, msg.stock().find( ' ' ) ),
            msg.price() );
    }
};

template <>
struct formatter< jacobi::ntvitch::add_order_mpid_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::add_order_mpid_t & msg,
                 Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{order_reference_number: {:X}, side: {}, shares: {}, stock: "
            "'{}', "
            "price: {}, attribution: {}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.order_reference_number(),
            msg.buy_sell(),
            msg.shares(),
            msg.stock().substr( 0, msg.stock().find( ' ' ) ),
            msg.price(),
            msg.attribution() );
    }
};

template <>
struct formatter< jacobi::ntvitch::order_executed_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::order_executed_t & msg,
                 Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{order_reference_number: {:X}, executed_shares: {}, "
            "match_number: "
            "{}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.order_reference_number(),
            msg.executed_shares(),
            msg.match_number() );
    }
};

template <>
struct formatter< jacobi::ntvitch::order_executed_with_price_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::order_executed_with_price_t & msg,
                 Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{order_reference_number: {:X}, executed_shares: {}, "
            "match_number: "
            "{}, printable: {}, execution_price: {}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.order_reference_number(),
            msg.executed_shares(),
            msg.match_number(),
            msg.printable(),
            msg.execution_price() );
    }
};

template <>
struct formatter< jacobi::ntvitch::order_cancel_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::order_cancel_t & msg,
                 Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{order_reference_number: {:X}, canceled_shares: {}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.order_reference_number(),
            msg.canceled_shares() );
    }
};

template <>
struct formatter< jacobi::ntvitch::order_delete_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::order_delete_t & msg,
                 Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{order_reference_number: {:X}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.order_reference_number() );
    }
};

template <>
struct formatter< jacobi::ntvitch::order_replace_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::order_replace_t & msg,
                 Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{original_order_reference_number: {:X}, "
            "new_order_reference_number: {:X}, shares: {}, price: {}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.original_order_reference_number(),
            msg.new_order_reference_number(),
            msg.shares(),
            msg.price() );
    }
};

template <>
struct formatter< jacobi::ntvitch::trade_non_cross_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::trade_non_cross_t & msg,
                 Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{order_reference_number: {}, side: {}, shares: {}, stock: '{}', "
            "price: {}, match_number: {}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.order_reference_number(),
            msg.buy_sell(),
            msg.shares(),
            msg.stock().substr( 0, msg.stock().find( ' ' ) ),
            msg.price(),
            msg.match_number() );
    }
};

template <>
struct formatter< jacobi::ntvitch::cross_trade_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::cross_trade_t & msg,
                 Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{shares: {}, stock: '{}', cross_price: {}, match_number: {}, "
            "cross: {}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.shares(),
            msg.stock().substr( 0, msg.stock().find( ' ' ) ),
            msg.cross_price(),
            msg.match_number(),
            msg.cross() );
    }
};

template <>
struct formatter< jacobi::ntvitch::broken_trade_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::broken_trade_t & msg,
                 Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{match_number: {}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.match_number() );
    }
};

template <>
struct formatter< jacobi::ntvitch::noii_t >
    : public jacobi::ntvitch::details::fmt_parser_format_base_t
{
    template < class Format_Context >
    auto format( const jacobi::ntvitch::noii_t & msg, Format_Context & ctx ) const
    {
        return fmt::format_to(
            ctx.out(),
            "{} {{paired_shares: {}, imbalance_shares: {}, imbalance_dir: {}, "
            "stock: '{}', "
            "far_price: {}, near_price: {}, current_reference_price: {}, cross: "
            "{}, price_variation: {}}}",
            static_cast< const jacobi::ntvitch::message_base_t & >( msg ),
            msg.paired_shares(),
            msg.imbalance_shares(),
            msg.imbalance_dir(),
            msg.stock().substr( 0, msg.stock().find( ' ' ) ),
            msg.far_price(),
            msg.near_price(),
            msg.current_reference_price(),
            msg.cross(),
            msg.price_variation() );
    }
};

}  // namespace fmt
