# JACOBI project (JACOBI - Just Another Collection of Order Book Implementations)

* [JACOBI project (JACOBI - Just Another Collection of Order Book Implementations)](#jacobi-project-jacobi---just-another-collection-of-order-book-implementations)
   * [Technical details](#technical-details)
      * [Build instructions](#build-instructions)
* [Order Book Implementation Details](#order-book-implementation-details)
   * [Core Abstractions](#core-abstractions)
   * [Vocabulary types](#vocabulary-types)
      * [Handling Bid/Ask Asymmetry](#handling-bidask-asymmetry)
   * [Price Level](#price-level)
      * [Lifecycle](#lifecycle)
      * [Price Level Implementations](#price-level-implementations)
   * [Orders Table](#orders-table)
      * [Implementation: CRTP](#implementation-crtp)
      * [Ordered Map Storage](#ordered-map-storage)
      * [Linear Storage](#linear-storage)
      * [Mixed Storage: LRU](#mixed-storage-lru)
      * [Mixed Storage: Hot Cold](#mixed-storage-hot-cold)
   * [Order References Index](#order-references-index)
* [Benchmarking Suite](#benchmarking-suite)
   * [Events Data File](#events-data-file)
   * [Common settings](#common-settings)

<!-- Created by https://github.com/ekalinin/github-markdown-toc -->

This project explores various design approaches to implementing high-performance
Order Books (L3) in C++.

The goal is to provide a comprehensive benchmark suite
to test different implementations against actual market data
(streams of events, updates, and execution operations).

While experimental, this is not a sandbox toy.
All implementations come with tests and decent documentation.

It serves two purposes:

  * Educational: Study different memory layouts and
    algorithmic trade-offs in L3 order book design.

  * Benchmarking: Run these implementations against your own data
    to see which strategy yields the best performance for your
    data and your hardware.

Once built, you can run the suite against your dataset on your hardware.
TODO-instructions and TODO-experiment-example.

## Technical details

Prerequisites:
  * C++20 compiler (the oldest version tested is g++-11).
  * CMake.
  * [Conan package manager](https://conan.io/) or alternatevely you
    provide an environment that will make all CMake `find(XXX)`
    in the root cmake-file work.

### Build instructions

Here are sample build instruction you can use as a starting point:

```bash
# Debug
conan install -pr:a ./conan_profiles/ubu-gcc-11 -s:a build_type=Debug --build missing -of _build .
( source ./_build/conanbuild.sh && cmake -B_build . -DCMAKE_TOOLCHAIN_FILE=_build/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug)
cmake --build _build -j 6

# Release
conan install -pr:a ./conan_profiles/ubu-gcc-11-x86-64-v3 -s:a build_type=Release --build missing -of _build_release .
( source ./_build_release/conanbuild.sh && cmake -B_build_release . -DCMAKE_TOOLCHAIN_FILE=_build_release/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release)
cmake --build _build_release -j 6

# UB and Address sanitizer build
conan install -pr:a ./conan_profiles/ubu-gcc-11-ubasan --build missing -of _build_ubasan .
( source ./_build_ubasan/conanbuild.sh && cmake -B_build_ubasan . -DCMAKE_TOOLCHAIN_FILE=_build_ubasan/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug -DJACOBI_BUILD_BENCHMARKS=OFF)
cmake --build _build_ubasan -j 6


# Clang:
conan install . -pr:a ubu-clang-18 -s:a build_type=Debug --build missing -of _build_clang
( source ./_build_clang/conanbuild.sh && cmake -B_build_clang . -DCMAKE_TOOLCHAIN_FILE=_build_clang/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug)
cmake --build _build_clang -j 6


```

Running tests:
```bash
ctest -T test --test-dir _build
```

# Order Book Implementation Details


**JACOBI** is a header-only library that uses
template metaprogramming to compose the order book at compile-time.
The main reason is performance. Because having required customization points
as template parameters the compiler generates a monolithic class with
zero virtual functions overhead and does better with inlining stuff.

There are 3 major points of customization:

1. Price Levels Storage:
   how do we find a price level? (e.g., RB-tree vs. Flat Vector).
2. Price Level Implementation:
   how do we store the queue of orders at a specific price?
   (e.g., std::list vs. xxx_lib::list vs. custom SOA implementations).
3. Order Index:
   how do we locate an order in a book by its ID?
   (e.g., std::unordered_map vs. other hash-tables implementation).

The most basic choices for the above would be:

1. using associative container `std::map<P, LVL>`;
2. using implementation based on `std::list<Order>`;
3. using `std::unordered_map< Id, Data >`.

Here are the detailed schematic view of
how combinations are wired to become an eventual type
implementing a book:

```
             ┌─────────── std::map<P, LVL> ─────────┐
*********    │                                      │
* Order *    │     std::vector<PLVL>                │
* Book  *────┼──── Linear Storage    ───────────────┴─────────┐
* Impl  *    │                                                │
*********    │                                                ├──────┐
             │             ┌──── map + LRU cache ────────┐    │      │
             └──── Mixed───┤                             ├────┘      │
                           └───── HOT(linear)+COLD(map) ─┘           │
                                                                     │
                                                                     │
┌────────────────────────────────────────────────────────────────────┘
│
│      BSN                                  Sequence of
│      Book                                 orders using
│      Seq Number        Order              a list per level
│      Counter           on level           or share same list
│
│    ┌── void ──┐     ┌── std::list ──┐   ┌── isolated list ──┐
│    │          │     │               │   │                   │
└────┤          ├──┬──┤               ├───┤                   ├────┐
     │          │  │  │               │   │                   │    │
     └── std ───┘  │  └── plf::list ──┘   └─── shared list ───┘    │
                   │                                               ├──┐
                   │                                               │  │
                   ├───────────── SOA price─level ─────────────────┘  │
                   │                                                  │
                   │                      Node Container              │
                   │  ┌── chunks ──┐    ┌── std::list ──┐             │
                   │  │   list     │    │               │             │
                   └──┤            ├────┤               ├─────────────┤
                      │   SOA      │    │               │             │
                      └── chunks ──┘    └── plf::list ──┘             │
                          list                                        │
                                                                      │
┌─────────────────────────────────────────────────────────────────────┘
│
│       Index container
│       order_id => order location
│
│      ┌── std::unordered_map ──┐        ***************
│      │                        │        *   Eventual  *
└──────┼── tsl::robin_map ──────├────────*   Book      *
       │                        │        *   Routine   *
       └── boost::unordered:: ──┘        ***************
           unordered_flat_map
```

## Core Abstractions

**JACOBI** exposes these main components:

* `book_t< Book_Traits >`: the main engine
  (see [book.hpp](./book/include/jacobi/book/book.hpp)).
  `Book_Traits` template parameter defines customizations
  that are used in the order book implementation.
  This class provides the natural API for order operations
  (add, delete, execute, reduce, modify)..

 It defines how price levels are stored and provides an internal API similar to the book itself. The book_t simply dispatches operations to the correct Orders Table based on the side.

* Orders Table (_customizable_).
  Represents one side of the book (BUY or SELL).
  It defines how price levels are stored and provides an internal API similar
  to the book itself. The book_t simply dispatches operations to the correct
  Orders Table based on the side (which it knows through Order References Index).

* Order References Index (_customizable_).
  A lookup table that maps an Order ID to its location
  (trade side, price level, price level position reference) and extra attributes.
  You can extend this reference.
  For example, you might add symbol metadata here to handle events that come in
  without a symbol attached.
  Note: since this is a lookup mechanism, the same Index type can
  technically be shared between multiple books and for some exchanges
  that is a necessity.

* Price level (_customizable_).
  The routine responsible for managing the queue of orders at a specific price point.


Additionaly here are some no so important components:

  * Book Sequence Number (BSN):
    The book can track the sequence number of events (updates/operations).
    If you need it, use `jacobi::book::std_bsn_counter_t` (benchmark code: **bsn1**).
    If you don't care, use `jacobi::book::void_bsn_counter_t` and
    the compiler will optimize the counting logic entirely out of the binary
    (benchmark code: **bsn2**).

The eventual Order Book implementation is a composite type
that glues together routines responsible for some part of the job
and provides common unified API work with the book.

## Vocabulary types

Vocabulary types are defined in [vocabulary_types.hpp](book/include/jacobi/book/vocabulary_types.hpp)

**JACOBI** leverages the [foonathan/type_safe](https://github.com/foonathan/type_safe)
library to enforce strong typing on primitives.
This prevents "stringly typed" errors and ensures you don't
accidentally assign  a price to a quantity.

Key types include:

 * `order_id_t`
 * `order_qty_t`
 * `order_price_t`

### Handling Bid/Ask Asymmetry

A critical component of the vocabulary is the price operations trait:

```C++
template < trade_side Side_Indicator >
struct order_price_operations_t
```

This struct abstracts the sorting logic between the two sides of the book,
allowing us to use a single generic algorithm for both.

 * SELL (Asks): Lower price has priority (Price A < Price B).
 * BUY (Bids): Higher price has priority (Price A > Price B).

Instead of polluting the code with `if( trade_side::buy == ts)` checks,
`order_price_operations_t<TS>` exposes a unified interface
for comparison and price manipulations.

## Price Level

Any implementation of a price level must satisfy
`jacobi::book::Price_Level_Concept`
(see [price_level_fwd.hpp](./book/include/jacobi/book/price_level_fwd.hpp)).

The interface contract looks like this:

```C++
struct price_level_example_t
{
    // Must define a type that is a price-level wide
    // "address" of the order (identify its position).
    // The reference_t itself must satisfy
    // Price_Level_Order_Reference_Concept.
    using reference_t = /* implementation defined */;

    // Price and volume
    order_price_t price() const;
    order_qty_t orders_qty() const;

    // Size queries
    auto orders_count() const -> std::size_t; // or convertible to size_t.
    bool empty() const; // or convertible to bool

    // Modifications
    reference_t add_order( order_t order );
    void delete_order( const reference_t & ref );
    reference_t reduce_qty( const reference_t & ref, order_qty_t qty );

    // Range support
    auto orders_range() const -> std::ranges::range;
    auto orders_range_reverse() const -> std::ranges::range;
};
```

A standard reference implementation using `std::list<order_t>`
is provided by `std_price_level_t`
(see [price_level.hpp](./book/include/jacobi/book/price_level.hpp)).

### Lifecycle

The Orders Table does not manage Price Levels directly;
it delegates lifecycle management to a Factory.

This abstraction allows for critical optimizations such as Object Pooling
and resource sharing.

 * **Creation**. Factories can pass shared contexts or
   pre-allocated memory buffers to new levels.

 * **Retirement**. When a price level becomes empty,
   it is "retired" rather than immediately destructed.
   This allows the factory to reclaim resources for future reuse
   without releasing memory.

See `Price_Levels_Factory_Concept` in
[price_level_fwd.hpp](./book/include/jacobi/book/price_level_fwd.hpp)
for details.

### Price Level Implementations

The choice of Price Level data structure is a critical benchmark parameter.
**JACOBI** provides five distinct implementations to explore
the trade-offs between memory overhead, cache locality, and update complexity.

1. Simple List Price Level (Baseline).

   One linked list per price level. Each node holds exactly one order.
   Implementation: `std_price_level_t<List_Traits>` ([jacobi/book/price_level.hpp](./book/include/jacobi/book/price_level.hpp)).

    Containers:
    - `std::list` (benchmark code: **plvl11**)
    - `plf::list` (benchmark code: **plvl12**).

2. Shared List Price Level.

   Instead of each price level managing its own isolated allocator,
   all price levels share a single underlying list instance
   (which allows to take advantage of internal nodes pooling mechanics).
   This goal it to improve locality  and minimize allocations by reusing
   already allocated objects.
   Implementation: `shared_list_container_price_level_t<List_Traits>`
   ([jacobi/book/price_level.hpp](./book/include/jacobi/book/price_level.hpp)).

   Containers:
    - `std::list` (benchmark code: **plvl21**)
    - `plf::list` (benchmark code: **plvl22**).


3. SOA Price Level.

   Stores the queue in a linear `std::vector<T>` using a Structure of Arrays layout.
   `id` and `qty` attributes are stored in separate arrays.
   Since the price is constant for the level, it is not stored per order
   saving 8 bytes per order.
   Queue order preservation is handled via an embedded index-based linked list
   (using integers instead of pointers) to avoid shifting elements on deletion
   and enabling reuse of free slots that appear after deleting an order.
   Trade-off is that the storage capacity never shrinks;
   it grows to the high-water mark of orders on that level.
   Implementatiosn:`soa_price_level_t<List_Traits>`
   ([jacobi/book/soa_price_level.hpp](./book/include/jacobi/book/soa_price_level.hpp)).

   Containers:
    - `std::vector<T>` (benchmark code: **plvl30**)
    - `boost::container::small_vector< T, N >` (benchmark code: **plvl31**).

4. Chunked Price Level.

   A hybrid approach reducing pointer overhead.
   Each node is a "chunk" with capacity for 64 orders.
   Occupancy is tracked via a 64-bit binary mask (allowing `O(1)`
   slot finding via intrinsics).
   This reduces the "next" pointer overhead.
   Trade-off is that it can suffer from fragmentation
   if orders are deleted randomly but in a way that keeps the node alive
   (creating "Swiss cheese" chunks).
   Implementation: `chunked_price_level_t<List_Traits>`
   ([jacobi/book/chunked_price_level.hpp](./book/include/jacobi/book/chunked_price_level.hpp)).

   Underlying node container:
    - `std::list` (benchmark code: **plvl41**)
    - `plf::list` (benchmark code: **plvl42**).

5. Chunked SOA Price Level.

   Combines the blocking strategy of the Chunked List with
   the cache-friendly layout of SOA.
   Each chunk stores data for 14 orders using internal SOA layout.
   Queue order preservation is handled via an embedded index-based linked list
   with 4-bit integers totaling the space of 16-bytes, that is from where
   14 capacity comes: we have 16 index-based link nodes and 1 goes for anchor node
   and 1 goes for anchor node for free-nodes list.
   Implementation: `chunked_soa_price_level_t<List_Traits>`
    ([jacobi/book/chunked_soa_price_level.hpp](./book/include/jacobi/book/chunked_soa_price_level.hpp)).

   Underlying node container:
    - `std::list` (benchmark code: **plvl51**)
    - `plf::list` (benchmark code: **plvl52**).

## Orders Table

The **Orders Table** manages the book state for a single side of the trade (BUY or SELL).
Its primary responsibility is the efficient storage and retrieval
of active Price Levels.

**JACOBI** provides three main storage strategies:

 * Ordered map storage. The baseline implementation uses `std::map<P, LVL>`.
   `O(log N)` lookups.

 * Linear storage. Price levels are stored in `std::vector<PLVL>`.
   Price levels are stored contiguously.
   Extremely cache-friendly for iteration, quick for access (`O(1)` lookups)
   but requires a strategy for mapping sparse prices to dense indices.

 * Mixed storage (hybrid).
   Relies on heuristic approach separating "Hot" and "Cold" data.
    - **Hot** (Linear). Near-the-money prices are stored in a vector for maximum speed.
      Hot levels are considered more likely to be requested by events.
    - **Cold** (Map). Out-of-the-money prices are offloaded to a map to save memory.


The Orders Table doesn't exist in a vacuum.
It is initialized with a shared context (`Book_Impl_Data`) that provides:

  1. Order References Index: to locate existing orders for modification/deletion.
  2. Price Levels Factory:
     to handle the lifecycle (creation/retirement) of level objects.

The above is passed to Orders Table implementation in constructor
in a form of shared context refered in the implementation as
`Book_Impl_Data` (see also `Book_Impl_Data_Concept`).

### Implementation: CRTP

To avoid virtual function overhead,
**JACOBI** uses the [CRTP](https://en.cppreference.com/w/cpp/language/crtp.html).

The base functionality is provided by `orders_table_crtp_base_t`
(see [jacobi/book/orders_table_crtp_base.hpp](./book/include/jacobi/book/orders_table_crtp_base.hpp))
Your specific storage strategy must inherit from this class.

The Contract: The derived class must implement `level_at( price )`
and few other functions:

```C++
auto level_at( order_price_t price )
    -> std::pair< price_level_type*, price_level_ref_type >;
    // first: A raw pointer to the price level object
    //       (for the CRTP base to manipulate orders).
    // second: A handle meaningful only to the derived storage class.
    //         If the price level becomes empty, the CRTP base asks
    //         the derived class to "retire" it.
    //         The base passes this object back to the derived class,
    //         allowing the storage implementation to remove the level
    //         (e.g., map::erase(iterator)) efficiently
    //         without searching for it again.

void retire_level( const price_level_ref_type & ref );

price_level_type & top_level();
order_price_t      top_price();
bool               empty();
```

### Ordered Map Storage

That is the most straight forward approach.

Pros:
 * Simplicity. Trivial to implement and verify.
 * Correctness. Handles sparse price distributions naturally.

Cons:
 * Performance. For low-latency systems, node-based containers are not acceptable.
 * Pointer Chasing. Traversing the tree (Red-Black usually)
   destroys CPU cache locality.

See implementation here: [jacobi/book/map/orders_table.hpp](./book/include/jacobi/book/map/orders_table.hpp)

**JACOBI** benchmarks 2 implementations:
  * `std::map<K, V>`
  * `absl:btree_map<K, V>`

### Linear Storage

This approach uses contiguous memory (`std::vector<PLVL>`).

**JACOBI** implements three variations of linear storage.

**Offset Mapping + Dynamic Top** (linear_v1)

  * Concept: maps a price directly to a vector index using a calculated offset.
    `index = (price - base_price)` (prices are normilized).
  * Layout: `levels[0]` represents the "deepest" (worst) price in the book.
    `levels.back()` represents the "best" (top of book) price.
  * Behavior: vector resizes dynamically at both ends.
    - Pros: `O(1)` lookups.
    - Cons: if a new order comes in with a price strictly "worse"
      than the current base, we must insert at the front.
      This forces a shift of all elements (`O(N)`).
      valnurable to some  book profiles that have orders
      at extreme prices which streches the storage.

**Offset Mapping + Append-Only Top** (linear_v2)

  * Concept: similar to linear_v1,  but the vector never shrinks and
    the top price is tracked separately.

  * Layout: `levels[0]` represents the "deepest" (worst) price in the book.
    `levels.back()` represents the top price ever reached by this
    trade side of the book.

  * Behavior: the vector only grows to accommodate better prices
    (extending the top of the book if new "all time" highs is reached).
    - Pros: avoids the resizing of the vector of linear_v1 in cases
      prices move often. And still has `O(1)` lookups.
    - Cons: memory usage is strictly monotonic.
      If the market moves significantly, this vector will consume vast
      amounts of RAM for empty price levels.
      As llinear_v1 it is valnurable to some  book profiles that have orders
      at extreme prices which streches the storage.

**Sorted Vector** (linear_v3)

  * Concept: a dense vector storing only non-empty levels, kept sorted by price.
  * Layout: Compact. No gaps for empty prices.
  * Behavior: uses binary search (O(logN)) for lookups.
    new levels are inserted preserving order.
    Insertion: Shifts elements to maintain order (O(N)).
    - Pros: ideal when the price distribution is extremely sparse
    (e.g., crypto alt-coins) where Linear_v1/v2 would allocate millions of empty slots.
    - Cons: on dense books performs poorly.

See implementations here: [jacobi/book/linear/orders_table.hpp](./book/include/jacobi/book/linear/orders_table.hpp)

### Mixed Storage: LRU

This approach uses a small LRU cache in front of a map storage.

See implementation here: [jacobi/book/mixed/lru/orders_table.hpp](./book/include/jacobi/book/mixed/lru/orders_table.hpp)

### Mixed Storage: Hot Cold

This strategy splits the book into two distinct storage zones
to balance speed and memory usage:

  * Hot Storage (linear-based ring buffer):
    Stores prices near the Best Bid/Offer (BBO). Lookup is `O(1)`.
  * Cold Storage (ordered map): Stores outliers. Access is `O(log N)`.


**The Ring Buffer**

The hot storage is implemented as a power-of-two sized ring buffer.
This allows us to map prices to indices using fast bitwise operations
(masking) rather than slow modulo arithmetic.

**The Sliding Window Strategy**

To maximize cache hits, we need the BBO to stay inside the "Hot" vector.
However, constantly shifting the vector elements (`O(h)`, h is hot storage size)
every time the price ticks is too expensive.

**JACOBI** uses a "Lazy Sliding" approach based on quarters:

  * The "Safe Zone" (Q1-Q3): As long as the Top Price (BBO) stays within the first 75%
    of the vector, we do nothing. This creates hysteresis, preventing jitter.

  * Backward Slide (Retreat).
    If the market moves away and the top price falls into the bottom quarter (Q4),
    we shift the window backwards to recenter at the new top price.

  * Forward Slide (Advance).
    If the top price moves forward (out of the hot range entirely),
    we shift the window forward to recenter.

This logic minimizes expensive memory moves while ensuring
the most relevant prices are always hot.

```
*******************
* BUY TRADE SIDE  *
*******************
          [...]                                    FWD             [...]
          [511]      F                        @    Direction       [511]
          [510]      F                       @@@                   [510]
          [509]      F                      @@@@@                  [509]_____
          [508]      F                     @@@@@@@                 [508]
          [507]      F                    @@@@@@@@@                 [507]
          [505]      F                       @@@                   [505] Q1
          [505]      F                       @@@                   [505]_____
          [504]      F                       @@@                   [504]
          [503]      F                       @@@              [503]
          [502]      F                      @@@@@                  [502] Q2      Slide
          [501]_____ F ***                                         [501]_____ <= Forward
       ┌──[500]     ───────────────┐                               [500]         after Top
       │  [499]                    │                               [499]         moves to
       │  [498] Q1                 │                               [498] Q3      501
       │  [497]_____               │                               [497]_____
       │  [496]                    │   Q1-Q3 SUBRANGE         _____[496]
       │  [495]                    ├── Top price can               [495]
       │  [494] Q2      Middle     │   move here without           [494] Q4
 CURR  │  [493]_____ <= Position   │   range sliding            Q1 [493]_____
  HOT ─┤  [492]                    │   (forward/backward)     _____[492]
RANGE  │  [491]                    │                               [491]
       │  [490] Q3                 │                               [490]
       │  [489]_____───────────────┘                 Slide      Q2 [489]
       │  [488]      B ***                        Backward => _____[488]
       │  [487]      B                            after top        [487]
       │  [486] Q4   B                            moves to         [486]
       └──[485]_____ B                                 488      Q3 [485]
          [484]      B                                        _____[484]
          [483]      B                                             [483]
          [482]      B                                             [482]
          [481]      B                                          Q4 [481]
          [480]                                               _____[480]
          [...]                                                    [...]
```

See implementation here: [jacobi/book/mixed/hot_cold/orders_table.hpp](./book/include/jacobi/book/mixed/lru/orders_table.hpp)

## Order References Index

This component maps an "Order ID" to its physical location in the book.
Since this lookup happens on every generic modification or cancellation,
**JACOBI** chooses to use well established implementations favoring
high-performance open-addressing ones.

Supported Backends:

  * **Standard**: `std::unordered_map` (benchmark code: **refIX1**) - Baseline.
  * **Robin Hood**: `tsl::robin_map` (benchmark code: **refIX2**, [Tessil/robin-map](https://github.com/Tessil/robin-map)).
  * **Boost Flat**: `boost::unordered::unordered_flat_map` (benchmark code: **refIX3**, [unordered_flat_map reference](https://www.boost.org/doc/libs/latest/libs/unordered/doc/html/unordered/reference/unordered_flat_map.html)).
  * **Swiss Table**: `absl::flat_hash_map` (benchmark code: **refIX4**, [Abseil Containers](https://abseil.io/docs/cpp/guides/container)).

To plug in a custom hash map, you simply provide a lightweight wrapper satisfying
`Order_Refs_Index_Concept`
(see [jacobi/book/order_refs_index.hpp](./book/include/jacobi/book/order_refs_index.hpp)).

**Customizing the Index Value**

The Index doesn't just store a some kind of pointer.
It stores a user-defined Value.
This is not an internal detail of the book implementation - it is an extension point.
This allows you to embed "hot" metadata directly into the index entry.
For example, if you pack a "Symbol ID" or "Client ID" into this struct,
you can route events without performing a secondary lookup in a different table.

Your custom value type must satisfy Order_Refs_Index_Value_Concept.
The contract is as follows:

```C++
struct RefValue
{
    // CONSTRUCTION
    // Must be constructible from an Order_Reference (the location in the Price Level).
    RefValue( Order_Reference r );

    // ACCESSORS
    // Retrieve the order object (or a copy of it).
    order_t access_order() const;

    // Retrieve the pointer/reference to the location in the Price Level.
    // This allows the book to jump directly to the memory address of the order.
    Order_Reference* access_order_reference();

    // METADATA
    // The index must know which side (Buy/Sell) the order belongs to
    // so it can dispatch the operation to the correct Orders Table.
    void set_trade_side( trade_side s );
    trade_side get_trade_side() const;

    // ... Your custom data members (e.g., symbol_id) go here ...
};
```

Since **JACOBI** prioritizes open-addressing backends
(like `boost::unordered_flat_map`), your `RefValue` must be lightweight.

Rule of Thumb: Your Custom Index Value should better be _trivially copyable_.
If you need to store strings consider to use a separate dictionary data structure
and store the id from the dictionary, not the string itself.

# Benchmarking Suite

The code which doesn't "lie" lives in the [benchmarks](./benchmarks) directory.

**JACOBI** provides a comprehensive suite to measure both Throughput (events/sec)
and Latency (time-to-process distribution).

**Benchmark Types**

  * Throughput: Measures raw processing speed.
    Uses the standard [googlebenchmark](https://github.com/google/benchmark) library.
    **Single Book**: feeds filtered events to one isolated book instance.
    **Multi-Book**: simulates a market feed across up to 500 distinct books.

  * Latency: Measures the distribution of processing times (P50, P90, P99, ... P99.999, Max).
    It implements custom timer loop and prints data in custom format.

**Binary Naming Convention**

Each Orders Table Strategy compiles into its own binaries:

  * `_bench.throughput.ORDERS_TABLE_CODE_levels_storage.single_book`;
  * `_bench.throughput.ORDERS_TABLE_CODE_levels_storage.multi_book`;
  * `_bench.latency.ORDERS_TABLE_CODE_levels_storage`.

Where `ORDERS_TABLE_CODE` is on of:
  * `map`, see [ordered map storage](#ordered-map-storage);
  * `absl_map`, see [ordered map storage](#ordered-map-storage);
  * `linear_v1`, see [linear storage](#linear-storage);
  * `linear_v2`, see [linear storage](#linear-storage);
  * `linear_v3`, see [linear storage](#linear-storage);
  * `mixed_hot_cold`, see [mixed storage: hot cold](#mixed-storage-hot-cold);
  * `mixed_lru`, see [mixed storage: lru](#mixed-storage-lru).

Each binary runs multiple permutations of internal components:
BSN counter, price level and orders references indexes.
Each combination appears in the output as a concatenation of 3 codes
`<BSN>_<PLVL>_<RefIX>` (all codes are listed in related documentation).

The final goal of all benchmarks is to test different
Order Book implementation approaches  against different loads.
To achieve it **JACOBI** uses binary files containing list of events.

Throughput benchmarks are using [googlebenchmark](https://github.com/google/benchmark)
and as such support only standard googlebenchmark options.
Latency benchmarks are implemented without googlebenchmark and support
own set of parameters and provides a custom output: distribution and Pxx values.

To control which files to use for load and also what are some other parameters
throughput benchmarks consider the following environment variables
(note: it's not feasable to pass a custom filename and other params to a benchmark
implemented with googlebenchmark):

**Data input**

  * `JACOBI_BENCHMARK_EVENTS_FILE=<path>`: Path to the binary file containing the stream of events..

**Runtime Tuning**

  * `JACOBI_BENCHMARK_EVENTS_RANGE=<Start>,<End>`: specefy a specific range of events.
    If exists benchmark would do measurements only for events in a range: `[Start, End)`.
    Note: the range should be valid (`Start<End`) and be within the events data file.

  * `JACOBI_BENCHMARK_HOT_STORAGE_SIZE`: specifies hot storage size.
    It has meaning only for [mixed storage: hot cold](#mixed-storage-hot-cold)
    approach. If doesn't  exist - a default value of 32 is used.

Note: `JACOBI_BENCHMARK_HOT_STORAGE_SIZE` is also considered by latency benchmarks
(`mixed_hot_cold`).

**Profiling Integrations**

**JACOBI** can automatically attach Linux perf to the benchmark process.

  * `JACOBI_BENCHMARK_PROFILE_MODE=<Mode>`.
    - `0`: not enabled;
    - `1`: records perf data: `perf record -g -p <PID>`;
    - `2`: records perf data: `perf record -g -o <FILEPATH> -p <PID>`,
           where `FILEPATH` is generated as
           `perf-<TIMESPAMP:SEC_PART>_<TIMESPAMP:MSEC_PART>.data`;
    - `3`: gather perf stats: `perf stat --append -o perf-stat.txt -p <PID>`;
    - `4`: gather perf stats (detailed): `perf stat --append -d -o perf-stat.txt -p <PID>`;
    - `5`: gather perf stats (very detailed): `perf stat --append -d -d -o perf-stat.txt -p <PID>`;



Exmaple runs for one of the benchmarks:

<pre><font color="#00FF00"><b>ngrodzitski@ngrodzitski</b></font>:<font color="#5C5CFF"><b>~/github/ngrodzitski/jacobi</b></font>$ JACOBI_BENCHMARK_EVENTS_FILE=./_snapshots_multi/sample_feed.jacobi_data JACOBI_BENCHMARK_HOT_STORAGE_SIZE=1024 JACOBI_BENCHMARK_EVENTS_RANGE=100000,17500000 taskset -c 3 ./_build_release/bin/_bench.throughput.map_levels_storage.multi_book --benchmark_min_time=4s --benchmark_filter=bsn1_.*_refIX[34]
2026-02-07T18:46:30+01:00
Running ./_build_release/bin/_bench.throughput.map_levels_storage.multi_book
Run on (8 X 4600 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 8192 KiB (x1)
Load Average: 1.14, 1.20, 1.22
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------------------------------
Benchmark                   Time             CPU   Iterations UserCounters...
-----------------------------------------------------------------------------
<font color="#00CD00">bsn1_plvl11_refIX3 </font><font color="#CDCD00">1799211458 ns   1797803150 ns   </font><font color="#00CDCD">         3</font> items_per_second=9.67848M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl11_refIX4 </font><font color="#CDCD00">1866905189 ns   1865926980 ns   </font><font color="#00CDCD">         3</font> items_per_second=9.32512M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl12_refIX3 </font><font color="#CDCD00">1920367712 ns   1919169814 ns   </font><font color="#00CDCD">         3</font> items_per_second=9.06642M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl12_refIX4 </font><font color="#CDCD00">2005219770 ns   2004209156 ns   </font><font color="#00CDCD">         3</font> items_per_second=8.68173M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl21_refIX3 </font><font color="#CDCD00">1979583729 ns   1978381332 ns   </font><font color="#00CDCD">         3</font> items_per_second=8.79507M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl21_refIX4 </font><font color="#CDCD00">2043550762 ns   2042618605 ns   </font><font color="#00CDCD">         3</font> items_per_second=8.51848M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl22_refIX3 </font><font color="#CDCD00">1832988789 ns   1831851708 ns   </font><font color="#00CDCD">         3</font> items_per_second=9.49859M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl22_refIX4 </font><font color="#CDCD00">1919314979 ns   1918315549 ns   </font><font color="#00CDCD">         3</font> items_per_second=9.07046M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl30_refIX3 </font><font color="#CDCD00">2241682185 ns   2240470485 ns   </font><font color="#00CDCD">         2</font> items_per_second=7.76623M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl30_refIX4 </font><font color="#CDCD00">2285975381 ns   2284943654 ns   </font><font color="#00CDCD">         2</font> items_per_second=7.61507M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl31_refIX3 </font><font color="#CDCD00">1753996629 ns   1752999432 ns   </font><font color="#00CDCD">         3</font> items_per_second=9.92584M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl31_refIX4 </font><font color="#CDCD00">1881453081 ns   1879993218 ns   </font><font color="#00CDCD">         3</font> items_per_second=9.25535M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl41_refIX3 </font><font color="#CDCD00">2283718530 ns   2278588672 ns   </font><font color="#00CDCD">         2</font> items_per_second=7.63631M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl41_refIX4 </font><font color="#CDCD00">2600473635 ns   2591598338 ns   </font><font color="#00CDCD">         2</font> items_per_second=6.714M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl42_refIX3 </font><font color="#CDCD00">2662828144 ns   2652305645 ns   </font><font color="#00CDCD">         2</font> items_per_second=6.56033M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl42_refIX4 </font><font color="#CDCD00">2486334790 ns   2484471650 ns   </font><font color="#00CDCD">         2</font> items_per_second=7.0035M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl51_refIX3 </font><font color="#CDCD00">1854523253 ns   1854048998 ns   </font><font color="#00CDCD">         3</font> items_per_second=9.38487M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl51_refIX4 </font><font color="#CDCD00">1953475562 ns   1952576897 ns   </font><font color="#00CDCD">         3</font> items_per_second=8.9113M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl52_refIX3 </font><font color="#CDCD00">2545382108 ns   2532748191 ns   </font><font color="#00CDCD">         2</font> items_per_second=6.87001M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00CD00">bsn1_plvl52_refIX4 </font><font color="#CDCD00">2580966686 ns   2566760380 ns   </font><font color="#00CDCD">         2</font> items_per_second=6.77897M/s ./_snapshots_multi/sample_feed.jacobi_data(18100K,r=[100000, 17500000), 17400K)
<font color="#00FF00"><b>ngrodzitski@ngrodzitski</b></font>:<font color="#5C5CFF"><b>~/github/ngrodzitski/jacobi</b></font>$ ./_build_release/bin/_bench.latency.map_levels_storage -h
JACOBI latency benchmark


./_build_release/bin/_bench.latency.map_levels_storage [OPTIONS]


OPTIONS:
  -h,     --help              Print this help message and exit
  -f,     --file TEXT:FILE REQUIRED
                              Path to events data file
  -r,     --range UINT x 2    Range [A, B) of events to measure
  -n,     --count UINT:UINT in [100000 - 18446744073709551615] [100000]
                              Number of measurements
          --benchmark_filter TEXT
                              Regex filter for benchmark names
<font color="#00FF00"><b>ngrodzitski@ngrodzitski</b></font>:<font color="#5C5CFF"><b>~/github/ngrodzitski/jacobi</b></font>$ ./_build_release/bin/_bench.latency.map_levels_storage -f ./_snapshots/sample_300k.jacobi_data -r 10000 298000 -n 10000000 --benchmark_filter=bsn1_plvl.*X[34]
Benchmark            Distribution                   P50       P90       P99      P999     P9999    P99999          MAX
bsn1_plvl11_refIX3:  N(0.077, 0.208);     PX:     0.069     0.092     0.182     0.412    13.731    20.935      112.796
bsn1_plvl11_refIX4:  N(0.084, 0.226);     PX:     0.075     0.103     0.188     0.407     15.64     21.06       51.185
bsn1_plvl12_refIX3:  N(0.087, 0.240);     PX:      0.08     0.104     0.199      0.35     15.89    21.367       44.311
bsn1_plvl12_refIX4:  N(0.094, 0.272);     PX:     0.086     0.111     0.213     0.464    18.313    21.628       70.515
bsn1_plvl21_refIX3:  N(0.091, 0.247);     PX:     0.081     0.112     0.218     0.385    15.824    22.998      104.743
bsn1_plvl21_refIX4:  N(0.092, 0.252);     PX:     0.084     0.114     0.208     0.351    17.442    21.218       55.404
bsn1_plvl22_refIX3:  N(0.084, 0.247);     PX:     0.074     0.108     0.182     0.395    17.142    21.084       30.383
bsn1_plvl22_refIX4:  N(0.085, 0.245);     PX:     0.077     0.107     0.167     0.301    16.153    21.465       38.089
bsn1_plvl30_refIX3:  N(0.128, 0.226);     PX:     0.113     0.178     0.334      0.53    13.917     23.87       67.874
bsn1_plvl30_refIX4:  N(0.134, 0.255);     PX:     0.118     0.182      0.38     0.614    15.674    24.569       43.053
bsn1_plvl31_refIX3:  N(0.080, 0.240);     PX:     0.075     0.102     0.168     0.321    15.836    21.188       41.587
bsn1_plvl31_refIX4:  N(0.082, 0.235);     PX:     0.078     0.105     0.163      0.39    15.726    21.032        32.79
bsn1_plvl41_refIX3:  N(0.157, 0.276);     PX:     0.119     0.285     0.429     0.685    18.162    25.087       56.368
bsn1_plvl41_refIX4:  N(0.164, 0.267);     PX:     0.126     0.294     0.432     0.702    15.888    24.464       85.238
bsn1_plvl42_refIX3:  N(0.137, 0.247);     PX:     0.121     0.205     0.341     0.552    15.819    23.881       44.565
bsn1_plvl42_refIX4:  N(0.152, 0.272);     PX:     0.136      0.23     0.424     0.684    18.084    26.864       64.318
bsn1_plvl51_refIX3:  N(0.085, 0.216);     PX:     0.081     0.105     0.196     0.337    17.322    24.057       35.948
bsn1_plvl51_refIX4:  N(0.090, 0.235);     PX:     0.084      0.11       0.2     0.427    17.855    25.059       52.973
bsn1_plvl52_refIX3:  N(0.123, 0.272);     PX:      0.11     0.182     0.278     0.476    18.102    26.821       84.165
bsn1_plvl52_refIX4:  N(0.131, 0.294);     PX:     0.116     0.191     0.317     0.584    18.189    26.866      100.009
</pre>


## Events Data File

All benchmarks need data file. **JACOBI** uses custom binary format.
Eeach record takes 32 bytes (Little Endian is assumed).

```
Book update record:
┌───────┬────────────────────────────────┐
│ bytes │ Definition                     │
╞═══════╪════════════════════════════════╡
│ 03:00 │ Book id (uint32)               │
├───────┼────────────────────────────────┤
│    04 │ Operation code (uint8):        │
│       │   0x0 - add order              │
│       │   0x1 - exec order             │
│       │   0x2 - reduce order           │
│       │   0x3 - modify order           │
│       │   0x4 - delete order           │
├───────┼────────────────────────────────┤
│    05 │ Trade side (uint8):            │
│       │   0x0 - SELL                   │
│       │   0x1 - BUY                    │
│       │ Meaningfull only if            │
│       │ operation is 'add order'       │
├───────┼────────────────────────────────┤
│ 07:06 │ reserved (must be ignored)     │
├───────┼────────────────────────────────┤
│ 31:08 │ Event specific data structure  │
└───────┴────────────────────────────────┘

Add order record:
┌───────┬───────┬────────────────────────┐
│       │ Master│                        │
│ bytes │ image │ Definition             │
│       │ bytes │                        │
╞═══════╪═══════╪════════════════════════╡
│ 07:00 │ 15:08 │ Order id (uint64)      │
├───────┼───────┼────────────────────────┤
│ 11:08 │ 19:16 │ qty (uint32)           │
├───────┼───────┼────────────────────────┤
│ 15:12 │ 23:20 │ reserved (padding)     │
├───────┼───────┼────────────────────────┤
│ 23:16 │ 31:24 │ price (uint64)         │
└───────┴───────┴────────────────────────┘

Exec order record:
┌───────┬───────┬────────────────────────┐
│       │ Master│                        │
│ bytes │ image │ Definition             │
│       │ bytes │                        │
╞═══════╪═══════╪════════════════════════╡
│ 07:00 │ 15:08 │ Order id (uint64)      │
├───────┼───────┼────────────────────────┤
│ 11:08 │ 19:16 │ Exec qty (uint32)      │
├───────┼───────┼────────────────────────┤
│ 23:12 │ 31:20 │ reserved (padding)     │
└───────┴───────┴────────────────────────┘

Reduce order record:
┌───────┬───────┬────────────────────────┐
│       │ Master│                        │
│ bytes │ image │ Definition             │
│       │ bytes │                        │
╞═══════╪═══════╪════════════════════════╡
│ 07:00 │ 15:08 │ Order id (uint64)      │
├───────┼───────┼────────────────────────┤
│ 11:08 │ 19:16 │ Cancel qty (uint32)    │
├───────┼───────┼────────────────────────┤
│ 23:12 │ 31:20 │ reserved (padding)     │
└───────┴───────┴────────────────────────┘

Modify order record:
┌───────┬───────┬────────────────────────┐
│       │ Master│                        │
│ bytes │ image │ Definition             │
│       │ bytes │                        │
╞═══════╪═══════╪════════════════════════╡
│ 07:00 │ 15:08 │ Order id (uint64)      │
├───────┼───────┼────────────────────────┤
│ 11:08 │ 19:16 │ qty (uint32)           │
├───────┼───────┼────────────────────────┤
│ 15:12 │ 23:20 │ reserved (padding)     │
├───────┼───────┼────────────────────────┤
│ 23:16 │ 31:24 │ price (uint64)         │
└───────┴───────┴────────────────────────┘


Delete order record:
┌───────┬───────┬────────────────────────┐
│       │ Master│                        │
│ bytes │ image │ Definition             │
│       │ bytes │                        │
╞═══════╪═══════╪════════════════════════╡
│ 07:00 │ 15:08 │ Order id (uint64)      │
├───────┼───────┼────────────────────────┤
│ 23:08 │ 31:16 │ reserved (padding)     │
└───────┴───────┴────────────────────────┘
```

See format details in
[snapshots/events_snapshots.hpp](./snapshots/include/jacobi/snapshots/events_snapshots.hpp);

## Common settings

When running benchmarks consider the following fine tunning:

```bash

# governor settings
sudo cpupower frequency-set --governor performance
sudo cpupower frequency-set --governor powersave
cpupower frequency-info

# Pin thread:
taskset -c 0 $my_benchmark_app $params
```
