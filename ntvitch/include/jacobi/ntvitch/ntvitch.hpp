#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#if defined( __clang__ ) || defined( __GNUC__ )
#    define JACOBI_NTVITCH_SWAP64( x ) __builtin_bswap64( x )
#    define JACOBI_NTVITCH_SWAP32( x ) __builtin_bswap32( x )
#    define JACOBI_NTVITCH_SWAP16( x ) __builtin_bswap16( x )

#elif defined( _MSC_VER )
#    include <stdlib.h>
#    define JACOBI_NTVITCH_SWAP64( x ) _byteswap_uint64( x )
#    define JACOBI_NTVITCH_SWAP32( x ) _byteswap_uint32( x )
#    define JACOBI_NTVITCH_SWAP16( x ) _byteswap_uint16( x )
#endif

namespace jacobi::ntvitch
{

//
// message_type
//

enum class message_type : char
{
    system_event              = 'S',
    stock_directory           = 'R',
    stock_trading_action      = 'H',
    add_order                 = 'A',
    add_order_mpid            = 'F',
    order_executed            = 'E',
    order_executed_with_price = 'C',
    order_cancel              = 'X',
    order_delete              = 'D',
    order_replace             = 'U',
    trade_non_cross           = 'P',
    cross_trade               = 'Q',
    broken_trade              = 'B',
    noii                      = 'I'
};

//
// system_event_code
//

/**
 * @brief Nasdaq supports the following event codes on a daily basis
 *        on the TotalView-ITCH data feed.
 */
enum class system_event_code : char
{
    /// Start of Messages. Outside of time stamp messages, the start of day
    /// message is the first message sent inany trading day.
    start_of_messages = 'O',
    /// Start of System hours. This message indicates that NASDAQ is open and ready
    /// to start accepting orders.
    start_of_system = 'S',
    /// Start of Market hours. This message is intended to indicate that Market
    /// Hours orders are available for execution.
    start_of_market = 'Q',
    /// End of Market hours. This message is intended to indicate that Market Hours
    /// orders are no longer available for execution.
    end_of_market = 'M',
    /// End of System hours. It indicates that Nasdaq is now closed and will
    /// not accept any new orders today. It is still possible to receive
    /// Broken Trade messages and Order Delete messages after the End of Day
    end_of_system = 'E',
    /// End of Messages. This is always the last message sent in any trading day.
    end_of_messages = 'C',

};

//
// stock_market_category
//

/**
 * @brief Indicates Listing market or listing market tier for the issue.
 */
enum class stock_market_category : char
{
    // Nasdaq-Listed Instruments:
    nasdaq_global_select_market = 'Q',
    nasdaq_global_market        = 'G',
    nasdaq_capital_market       = 'S',

    // Non-Nasdaq-Listed Instruments:
    nyse               = 'N',
    nyse_american      = 'A',
    nyse_arca          = 'P',
    bats_z_exchang     = 'Z',
    investors_exchange = 'V',
    not_available      = ' ',
};

//
// stock_financial_status
//

/**
 * @brief Indicates Listing market or listing market tier for the issue.
 */
enum class stock_financial_status : char
{
    /// Nasdaq-Listed Instruments:
    deficient                         = 'D',
    delinquent                        = 'E',
    bankrupt                          = 'Q',
    suspended                         = 'S',
    deficient_and_bankrupt            = 'G',
    deficient_and_delinquent          = 'H',
    delinquent_and_bankrupt           = 'J',
    deficient_delinquent_and_bankrupt = 'K',

    /// Creations and/or Redemptions Suspended for Exchange Traded Product.
    creations_andor_redemptions_suspended = 'C',

    /// Normal (Default): Issuer Is NOT Deficient, Delinquent, or Bankrupt.
    normal = 'N',

    /// Non-Nasdaq-Listed Instruments:
    nyse               = 'N',
    nyse_american      = 'A',
    nyse_arca          = 'P',
    bats_z_exchang     = 'Z',
    investors_exchange = 'V',

    /// Non-Nasdaq-Listed Instruments:
    /// Not available. Firms should refer to SIAC feeds for code if needed.
    not_available = ' ',
};

//
// stock_round_lots_flag
//

/**
 * @brief Indicates if Nasdaq system limits order entry for issue.
 */
enum class stock_round_lots_flag : char
{
    /// Nasdaq system only accepts round lots.
    round_lots_only = 'Y',
    /// Nasdaq system does not  restrictions for this security.
    /// Odd and mixed lot orders are allowed.
    any_order_size = 'N',
};

//
// stock_issue_classification
//

/**
 * @brief Identifies the security class for the issue as assigned by Nasdaq.
 */
enum class stock_issue_classification : char
{
    american_depositary_share     = 'A',
    bond                          = 'B',
    common_stock                  = 'C',
    depository_receipt            = 'F',
    the_144a                      = 'I',
    limited_partnership           = 'L',
    notes                         = 'N',
    ordinaryshare                 = 'O',
    preferredstock                = 'P',
    other_securities              = 'Q',
    right_                        = 'R',
    shares_of_beneficial_interest = 'S',
    convertible_debenture         = 'T',
    unit                          = 'U',
    units_benif_int               = 'V',
    warrant                       = 'W',
};

//
// stock_authenticity
//

/**
 * Denotes if an issue or quoting participant record is set-•-up in
 * Nasdaq systems in a live/production, test, or demo state.
 * Please note that firms should only show live issues and quoting
 * participants on public quotation displays.
 */
enum class stock_authenticity : char
{
    live_production = 'P',
    test            = 'T',
};

//
// stock_short_sale_threshold
//

/**
 * Indicates if a security is subject to mandatory close-•-out of
 * short sales under SEC Rule 203(b)(3).
 */
enum class stock_short_sale_threshold : char
{
    restricted     = 'Y',
    not_restricted = 'N',
    not_available  = ' ',
};

//
// stock_ipo_flag
//

/**
 * Indicates if a security is subject to mandatory close-•-out of
 * short sales under SEC Rule 203(b)(3).
 */
enum class stock_ipo_flag : char
{
    /// Nasdaq listed instrument is set up as a new IPO security.
    is_new = 'Y',
    /// Nasdaq listed instrument is not set up as a new IPO security.
    not_new = 'N',
    /// Not available.
    not_available = ' ',
};

//
// luld_reference
//

/**
 * @brief Alpha Indicates which Limit Up / Limit Down price band calculation
 *        parameter is to be used for the instrument.
 *
 * Refer to LULD Rule for details.
 */
enum class stock_luld_reference : char
{
    /// Tier 1 NMS Stocks and select ETPs.
    tier_1 = '1',
    /// Tier 2 NMS Stocks.
    tier_2 = '2',

    /// Not available.
    not_available = ' ',
};

//
// stok_etp_flag
//

/**
 * @brief Alpha Indicates whether the security is an exchange traded product (ETP).
 */
enum class stok_etp_flag : char
{
    /// Instrument is an ETP.
    is_etp = 'Y',
    /// Instrument is not an ETP.
    not_etp = 'N',

    /// Not available.
    not_available = ' ',
};

//
// stock_inverse_indicator
//

/**
 * @brief Alpha Indicates the directional relationship between the ETP and
 *        Underlying index.
 */
enum class stock_inverse_indicator : char
{
    /// ETP is an Inverse ETP.
    is_inverseetp = 'Y',
    /// ETP is not an Inverse ETP.
    notinverse_etp = 'N',

    /// Not available.
    not_available = ' ',
};

//
// stock_trading_state
//

/**
 * @brief Indicates the current trading state for the stock.
 */
enum class stock_trading_state : char
{
    halted         = 'H',
    paused         = 'P',
    quotation_only = 'Q',
    trading        = 'T',
};

//
// buy_sell_indicator
//

/**
 * @brief Indicates whether the order is a buy or sell.
 */
enum class buy_sell_indicator : char
{
    buy  = 'B',
    sell = 'S'
};

//
// printable_flag
//

/**
 * @brief Indicates if the execution should be reflected on consolidated prints.
 */
enum class printable_flag : char
{
    non_printable = 'N',
    printable     = 'Y'
};

//
// cross_type
//

/**
 * @brief Indicates the type of cross trade.
 */
enum class cross_type : char
{
    opening     = 'O',
    closing     = 'C',
    halt_or_ipo = 'H',
    intraday    = 'I'
};

//
// imbalance_direction
//

/**
 * @brief Indicates the market side of the order imbalance.
 */
enum class imbalance_direction : char
{
    buy                 = 'B',
    sell                = 'S',
    no_imbalance        = 'N',
    insufficient_orders = 'O'
};

//
// price_variation_indicator
//

/**
 * @brief Indicates the absolute value of the percentage of deviation of the Near
 * Indicative Clearing Price to the nearest Current Reference Price.
 */
enum class price_variation_indicator : char
{
    less_than_1_percent = 'L',
    from_1_to_1_99      = '1',
    from_2_to_2_99      = '2',
    from_3_to_3_99      = '3',
    from_4_to_4_99      = '4',
    from_5_to_5_99      = '5',
    from_6_to_6_99      = '6',
    from_7_to_7_99      = '7',
    from_8_to_8_99      = '8',
    from_9_to_9_99      = '9',
    from_10_to_19_99    = 'A',
    from_20_to_29_99    = 'B',
    from_30_or_greater  = 'C',
    not_available       = ' '
};

// ===================================================================
// MESSAGES
// ===================================================================
#include "packed_struct_begin.ipp"

//
// message_base_t
//

/**
 * @brief Base struct to start messages of the ITCH protocol.
 */
struct JACOBI_NTVITCH_PACKED message_base_t
{
public:
    [[nodiscard]] message_type msg_type() const noexcept { return m_msg_type; }
    [[nodiscard]] std::uint16_t tracking_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP16( m_tracking_number );
    }
    [[nodiscard]] std::uint16_t stock_locate() const noexcept
    {
        return JACOBI_NTVITCH_SWAP16( m_stock_locate );
    }

    [[nodiscard]] std::uint64_t timestamp_ns() const noexcept
    {
        std::uint64_t res{};
        std::memcpy( reinterpret_cast< std::byte * >( &res ) + 2,
                     &m_timestamp_ns,
                     sizeof( m_timestamp_ns ) );
        return JACOBI_NTVITCH_SWAP64( res );
    }

private:
    message_type m_msg_type;
    std::uint16_t m_stock_locate;
    std::uint16_t m_tracking_number;
    std::uint8_t m_timestamp_ns[ 6 ];
};

//
// common_event_fields_t
//

struct JACOBI_NTVITCH_PACKED system_event_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 12 };
    inline static constexpr message_type expected_message_type{
        message_type::system_event
    };

    [[nodiscard]] system_event_code sys_event_code() const noexcept
    {
        return m_sys_event_code;
    }

private:
    system_event_code m_sys_event_code;
};

static_assert( sizeof( system_event_t ) == system_event_t::image_size );

struct JACOBI_NTVITCH_PACKED stock_directory_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 39 };
    inline static constexpr message_type expected_message_type{
        message_type::stock_directory
    };

    [[nodiscard]] std::string_view stock() const noexcept
    {
        return std::string_view( m_stock, 8 );
    }
    [[nodiscard]] std::uint64_t stock_as_u64() const noexcept
    {
        std::uint64_t res;
        std::memcpy( &res, m_stock, sizeof( res ) );
        return res;
    }

    [[nodiscard]] stock_market_category market_category() const noexcept
    {
        return m_market_category;
    }

    [[nodiscard]] stock_financial_status financial_status_indicator()
        const noexcept
    {
        return m_financial_status_indicator;
    }

    [[nodiscard]] std::uint32_t round_lot_size() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_round_lot_size );
    }

    [[nodiscard]] stock_round_lots_flag round_lots() const noexcept
    {
        return m_round_lots;
    }

    [[nodiscard]] bool round_lots_only() const noexcept
    {
        return stock_round_lots_flag::round_lots_only == round_lots();
    }

    [[nodiscard]] stock_issue_classification issue_classification() const noexcept
    {
        return m_issue_classification;
    }

    [[nodiscard]] std::span< const char, 2 > issue_subtype() const noexcept
    {
        return m_issue_subtype;
    }

    [[nodiscard]] stock_authenticity authenticity() const noexcept
    {
        return m_authenticity;
    }

    [[nodiscard]] stock_short_sale_threshold short_sale_threshold_indicator()
        const noexcept
    {
        return m_short_sale_threshold_indicator;
    }

    [[nodiscard]] stock_ipo_flag ipo_flag() const noexcept { return m_ipo_flag; }

    [[nodiscard]] stock_luld_reference luld_reference() const noexcept
    {
        return m_luld_reference;
    }

    [[nodiscard]] stok_etp_flag etp_flag() const noexcept { return m_etp_flag; }

    [[nodiscard]] std::uint32_t etp_leverage_factor() const noexcept
    {
        return m_etp_leverage_factor;
    }

    [[nodiscard]] stock_inverse_indicator inverse_indicator() const noexcept
    {
        return m_inverse_indicator;
    }

private:
    char m_stock[ 8 ];
    stock_market_category m_market_category;
    stock_financial_status m_financial_status_indicator;
    std::uint32_t m_round_lot_size;
    stock_round_lots_flag m_round_lots;
    stock_issue_classification m_issue_classification;
    char m_issue_subtype[ 2 ];
    stock_authenticity m_authenticity;
    stock_short_sale_threshold m_short_sale_threshold_indicator;
    stock_ipo_flag m_ipo_flag;
    stock_luld_reference m_luld_reference;
    stok_etp_flag m_etp_flag;
    std::uint32_t m_etp_leverage_factor;
    stock_inverse_indicator m_inverse_indicator;
};

static_assert( sizeof( stock_directory_t ) == stock_directory_t::image_size );

//
// stock_trading_action_t
//

struct JACOBI_NTVITCH_PACKED stock_trading_action_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 25 };
    inline static constexpr message_type expected_message_type{
        message_type::stock_trading_action
    };

    [[nodiscard]] std::string_view stock() const noexcept
    {
        return std::string_view( m_stock, 8 );
    }

    [[nodiscard]] std::uint64_t stock_as_u64() const noexcept
    {
        std::uint64_t res;
        std::memcpy( &res, m_stock, sizeof( res ) );
        return res;
    }

    [[nodiscard]] stock_trading_state trading_state() const noexcept
    {
        return m_trading_state;
    }
    [[nodiscard]] std::string_view reason() const noexcept
    {
        return std::string_view( m_reason, 4 );
    }

private:
    char m_stock[ 8 ];
    stock_trading_state m_trading_state;
    char m_reserved;
    char m_reason[ 4 ];
};
static_assert( sizeof( stock_trading_action_t )
               == stock_trading_action_t::image_size );

//
// add_order_t
//

struct JACOBI_NTVITCH_PACKED add_order_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 36 };
    inline static constexpr message_type expected_message_type{
        message_type::add_order
    };

    [[nodiscard]] std::uint64_t order_reference_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_order_reference_number );
    }
    [[nodiscard]] buy_sell_indicator buy_sell() const noexcept
    {
        return m_buy_sell_indicator;
    }
    [[nodiscard]] std::uint32_t shares() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_shares );
    }

    [[nodiscard]] std::string_view stock() const noexcept
    {
        return std::string_view( m_stock, 8 );
    }
    [[nodiscard]] std::uint64_t stock_as_u64() const noexcept
    {
        std::uint64_t res;
        std::memcpy( &res, m_stock, sizeof( res ) );
        return res;
    }

    // Note: ITCH 5.0 prices are integer values (implied 4 decimal places)
    [[nodiscard]] std::uint32_t price() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_price );
    }

private:
    std::uint64_t m_order_reference_number;
    buy_sell_indicator m_buy_sell_indicator;
    std::uint32_t m_shares;
    char m_stock[ 8 ];
    std::uint32_t m_price;
};
static_assert( sizeof( add_order_t ) == add_order_t::image_size );

//
// add_order_mpid_t
//

struct JACOBI_NTVITCH_PACKED add_order_mpid_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 40 };
    inline static constexpr message_type expected_message_type{
        message_type::add_order_mpid
    };

    [[nodiscard]] std::uint64_t order_reference_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_order_reference_number );
    }
    [[nodiscard]] buy_sell_indicator buy_sell() const noexcept
    {
        return m_buy_sell_indicator;
    }
    [[nodiscard]] std::uint32_t shares() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_shares );
    }

    [[nodiscard]] std::string_view stock() const noexcept
    {
        return std::string_view( m_stock, 8 );
    }
    [[nodiscard]] std::uint64_t stock_as_u64() const noexcept
    {
        std::uint64_t res;
        std::memcpy( &res, m_stock, sizeof( res ) );
        return res;
    }

    [[nodiscard]] std::uint32_t price() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_price );
    }

    [[nodiscard]] std::string_view attribution() const noexcept
    {
        return std::string_view( m_attribution, 4 );
    }

private:
    std::uint64_t m_order_reference_number;
    buy_sell_indicator m_buy_sell_indicator;
    std::uint32_t m_shares;
    char m_stock[ 8 ];
    std::uint32_t m_price;
    char m_attribution[ 4 ];
};
static_assert( sizeof( add_order_mpid_t ) == add_order_mpid_t::image_size );

//
// order_executed_t
//

struct JACOBI_NTVITCH_PACKED order_executed_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 31 };
    inline static constexpr message_type expected_message_type{
        message_type::order_executed
    };

    [[nodiscard]] std::uint64_t order_reference_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_order_reference_number );
    }
    [[nodiscard]] std::uint32_t executed_shares() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_executed_shares );
    }
    [[nodiscard]] std::uint64_t match_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_match_number );
    }

private:
    std::uint64_t m_order_reference_number;
    std::uint32_t m_executed_shares;
    std::uint64_t m_match_number;
};
static_assert( sizeof( order_executed_t ) == order_executed_t::image_size );

//
// order_executed_with_price_t
//

struct JACOBI_NTVITCH_PACKED order_executed_with_price_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 36 };
    inline static constexpr message_type expected_message_type{
        message_type::order_executed_with_price
    };

    [[nodiscard]] std::uint64_t order_reference_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_order_reference_number );
    }
    [[nodiscard]] std::uint32_t executed_shares() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_executed_shares );
    }
    [[nodiscard]] std::uint64_t match_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_match_number );
    }
    [[nodiscard]] printable_flag printable() const noexcept { return m_printable; }
    [[nodiscard]] std::uint32_t execution_price() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_execution_price );
    }

private:
    std::uint64_t m_order_reference_number;
    std::uint32_t m_executed_shares;
    std::uint64_t m_match_number;
    printable_flag m_printable;
    std::uint32_t m_execution_price;
};
static_assert( sizeof( order_executed_with_price_t )
               == order_executed_with_price_t::image_size );

//
// order_cancel_t
//

struct JACOBI_NTVITCH_PACKED order_cancel_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 23 };
    inline static constexpr message_type expected_message_type{
        message_type::order_cancel
    };

    [[nodiscard]] std::uint64_t order_reference_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_order_reference_number );
    }
    [[nodiscard]] std::uint32_t canceled_shares() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_canceled_shares );
    }

private:
    std::uint64_t m_order_reference_number;
    std::uint32_t m_canceled_shares;
};
static_assert( sizeof( order_cancel_t ) == order_cancel_t::image_size );

//
// order_delete_t
//

struct JACOBI_NTVITCH_PACKED order_delete_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 19 };
    inline static constexpr message_type expected_message_type{
        message_type::order_delete
    };

    [[nodiscard]] std::uint64_t order_reference_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_order_reference_number );
    }

private:
    std::uint64_t m_order_reference_number;
};
static_assert( sizeof( order_delete_t ) == order_delete_t::image_size );

//
// order_replace_t
//

struct JACOBI_NTVITCH_PACKED order_replace_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 35 };
    inline static constexpr message_type expected_message_type{
        message_type::order_replace
    };

    [[nodiscard]] std::uint64_t original_order_reference_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_original_order_reference_number );
    }
    [[nodiscard]] std::uint64_t new_order_reference_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_new_order_reference_number );
    }
    [[nodiscard]] std::uint32_t shares() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_shares );
    }
    [[nodiscard]] std::uint32_t price() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_price );
    }

private:
    std::uint64_t m_original_order_reference_number;
    std::uint64_t m_new_order_reference_number;
    std::uint32_t m_shares;
    std::uint32_t m_price;
};
static_assert( sizeof( order_replace_t ) == order_replace_t::image_size );

//
// trade_non_cross_t
//

struct JACOBI_NTVITCH_PACKED trade_non_cross_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 44 };
    inline static constexpr message_type expected_message_type{
        message_type::trade_non_cross
    };

    [[nodiscard]] std::uint64_t order_reference_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_order_reference_number );
    }
    [[nodiscard]] buy_sell_indicator buy_sell() const noexcept
    {
        return m_buy_sell_indicator;
    }
    [[nodiscard]] std::uint32_t shares() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_shares );
    }
    [[nodiscard]] std::string_view stock() const noexcept
    {
        return std::string_view( m_stock, 8 );
    }
    [[nodiscard]] std::uint32_t price() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_price );
    }
    [[nodiscard]] std::uint64_t match_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_match_number );
    }

private:
    std::uint64_t m_order_reference_number;
    buy_sell_indicator m_buy_sell_indicator;
    std::uint32_t m_shares;
    char m_stock[ 8 ];
    std::uint32_t m_price;
    std::uint64_t m_match_number;
};
static_assert( sizeof( trade_non_cross_t ) == trade_non_cross_t::image_size );

//
// cross_trade_t
//

struct JACOBI_NTVITCH_PACKED cross_trade_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 40 };
    inline static constexpr message_type expected_message_type{
        message_type::cross_trade
    };

    [[nodiscard]] std::uint64_t shares() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_shares );
    }
    [[nodiscard]] std::string_view stock() const noexcept
    {
        return std::string_view( m_stock, 8 );
    }
    [[nodiscard]] std::uint32_t cross_price() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_cross_price );
    }
    [[nodiscard]] std::uint64_t match_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_match_number );
    }
    [[nodiscard]] cross_type cross() const noexcept { return m_cross_type; }

private:
    std::uint64_t m_shares;
    char m_stock[ 8 ];
    std::uint32_t m_cross_price;
    std::uint64_t m_match_number;
    cross_type m_cross_type;
};
static_assert( sizeof( cross_trade_t ) == cross_trade_t::image_size );

//
// broken_trade_t
//

struct JACOBI_NTVITCH_PACKED broken_trade_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 19 };
    inline static constexpr message_type expected_message_type{
        message_type::broken_trade
    };

    [[nodiscard]] std::uint64_t match_number() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_match_number );
    }

private:
    std::uint64_t m_match_number;
};
static_assert( sizeof( broken_trade_t ) == broken_trade_t::image_size );

//
// noii_t
//

/**
 * @brief Net Order Imbalance Indicator.
 */
struct JACOBI_NTVITCH_PACKED noii_t : public message_base_t
{
public:
    inline static constexpr std::size_t image_size{ 50 };
    inline static constexpr message_type expected_message_type{
        message_type::noii
    };

    [[nodiscard]] std::uint64_t paired_shares() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_paired_shares );
    }
    [[nodiscard]] std::uint64_t imbalance_shares() const noexcept
    {
        return JACOBI_NTVITCH_SWAP64( m_imbalance_shares );
    }
    [[nodiscard]] imbalance_direction imbalance_dir() const noexcept
    {
        return m_imbalance_direction;
    }
    [[nodiscard]] std::string_view stock() const noexcept
    {
        return std::string_view( m_stock, 8 );
    }
    [[nodiscard]] std::uint32_t far_price() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_far_price );
    }
    [[nodiscard]] std::uint32_t near_price() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_near_price );
    }
    [[nodiscard]] std::uint32_t current_reference_price() const noexcept
    {
        return JACOBI_NTVITCH_SWAP32( m_current_reference_price );
    }
    [[nodiscard]] cross_type cross() const noexcept { return m_cross_type; }
    [[nodiscard]] price_variation_indicator price_variation() const noexcept
    {
        return m_price_variation_indicator;
    }

private:
    std::uint64_t m_paired_shares;
    std::uint64_t m_imbalance_shares;
    imbalance_direction m_imbalance_direction;
    char m_stock[ 8 ];
    std::uint32_t m_far_price;
    std::uint32_t m_near_price;
    std::uint32_t m_current_reference_price;
    cross_type m_cross_type;
    price_variation_indicator m_price_variation_indicator;
};
static_assert( sizeof( noii_t ) == noii_t::image_size );

#include "packed_struct_end.ipp"

namespace details
{

//
// x_message_t
//

/**
 * @brief Primary template for message type traits.
 *        Left undefined to trigger a compile-time error for unmapped types.
 */
template < message_type Message_Type >
struct x_message_t;

// ===================================================================
// Explicit specializations providing the nested `type`

template <>
struct x_message_t< message_type::system_event >
{
    using type = system_event_t;
};

template <>
struct x_message_t< message_type::stock_directory >
{
    using type = stock_directory_t;
};

template <>
struct x_message_t< message_type::stock_trading_action >
{
    using type = stock_trading_action_t;
};

template <>
struct x_message_t< message_type::add_order >
{
    using type = add_order_t;
};

template <>
struct x_message_t< message_type::add_order_mpid >
{
    using type = add_order_mpid_t;
};

template <>
struct x_message_t< message_type::order_executed >
{
    using type = order_executed_t;
};

template <>
struct x_message_t< message_type::order_executed_with_price >
{
    using type = order_executed_with_price_t;
};

template <>
struct x_message_t< message_type::order_cancel >
{
    using type = order_cancel_t;
};

template <>
struct x_message_t< message_type::order_delete >
{
    using type = order_delete_t;
};

template <>
struct x_message_t< message_type::order_replace >
{
    using type = order_replace_t;
};

template <>
struct x_message_t< message_type::trade_non_cross >
{
    using type = trade_non_cross_t;
};

template <>
struct x_message_t< message_type::cross_trade >
{
    using type = cross_trade_t;
};

template <>
struct x_message_t< message_type::broken_trade >
{
    using type = broken_trade_t;
};

template <>
struct x_message_t< message_type::noii >
{
    using type = noii_t;
};

// ===================================================================

}  // namespace details

//
// itch_message_t
//

/**
 * @brief Helper alias template to easily extract the mapped struct type.
 */
template < message_type Message_Type >
using itch_message_t = typename details::x_message_t< Message_Type >::type;

}  // namespace jacobi::ntvitch
