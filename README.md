
Multithreaded limit order book matching engine in C++

“I built a multithreaded limit order book matching engine in C++ that simulates the core of a stock exchange trading system. The goal of the project was to understand how real trading engines match buy and sell orders under price-time priority while maintaining correctness in a concurrent environment.”

---

## System Overview

“At a high level, the system processes incoming buy and sell orders, matches them based on price-time priority, executes trades when possible, and maintains the state of all active orders. It also supports market orders, order cancellation, modification, and real-time lookup of order status.”

---

##  Design Structure

“I designed the system with four main components:

First, I maintain two order books:

* A buy book sorted by descending price
* A sell book sorted by ascending price

This ensures that the best bid and best ask are always accessible in O(1) time from the map head.

Second, I maintain an order lookup table using an unordered map, which tracks the full lifecycle of every order including its quantity, price, side, and status like OPEN, PARTIAL, or FILLED.”

---

## Matching Engine Logic

“The core of the system is the matching engine.

When a new order arrives:

* If it is a buy order, I match it against the lowest available sell prices.
* If it is a sell order, I match it against the highest available buy prices.

I continuously execute trades by taking the minimum of available quantities between matching orders, updating both sides accordingly.

If an order is not fully filled, the remaining quantity is placed into the order book and waits for future matching.”

---

## Market Orders

“I also implemented market orders, which do not have a price constraint.

Market buy orders consume liquidity from the lowest sell prices immediately, and market sell orders consume from the highest buy prices. This simulates aggressive execution behavior seen in real exchanges.”

---

##  Order Lifecycle

“Each order goes through a clear lifecycle:

* OPEN when placed
* PARTIAL when partially executed
* FILLED when fully executed
* CANCELLED when removed before execution completes

This state is stored in a centralized order lookup table, which allows fast querying of any order at any time.”

---

##  Concurrency Model

“To simulate real-world concurrent trading activity, I used multiple threads where each thread generates random buy and sell orders.

To ensure thread safety, I used a single `shared_mutex`:

* Read operations use shared locks, allowing concurrency
* Write operations use exclusive locks to maintain consistency during modifications

This guarantees correctness but keeps the design simple and safe for a prototype system.”

---

## Modify and Cancel

“I also implemented order cancellation and modification.

Cancellation works by removing the order from all price levels and marking it as cancelled in the lookup table.

Modification is implemented as a cancel-and-reinsert operation with updated price and quantity while preserving the same order ID.”

---

## Trade Execution Output

“Whenever a match occurs, the system logs trade execution details including:

* traded quantity
* execution price
* buyer and seller IDs
* timestamps

This simulates a trade feed similar to what real exchanges publish.”

---

## Design Trade-offs

“I made several intentional trade-offs for simplicity and clarity:

I used a map-based structure instead of more complex low-latency data structures to simplify price ordering.

I used a coarse-grained lock instead of fine-grained locking to ensure correctness and avoid race conditions.

I also used string timestamps and console logging for debugging, even though in production systems these would be replaced with high-performance time representations and async logging systems.”

---

##  Limitations

“This is a prototype system and not production-ready. Some limitations include:

* Cancel operations are O(n) because of scanning queues
* Global locking can become a bottleneck under high throughput
* No persistent storage or crash recovery
* No real market data feed integration
* Logging inside critical sections impacts performance”

---

##  Real-world relevance

“In real trading systems, order books are highly optimized, often partitioned by symbol and designed for microsecond latency. They use specialized memory layouts, lock-free queues, and persistent write-ahead logging systems.

This project is a simplified but accurate representation of those core concepts.”

---

