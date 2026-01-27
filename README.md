
Limit Order Book (LOB) -  A major low latency project based on C++ and Python


INTRODUCTION
A LOB is the core structure of modern financial exchanges.
In the financial markets, LOB is the digital record of all outstanding “limit orders” for a specific security (like a stock or crypto). It is the core of an electronic exchange, showing how many people want to buy or sell at every possible price level.

It is different from Market Order (says “buy this right now at any price”) where Limit Order says “buy this only if the price is X or better.” The LOB is where these waiting orders live unit someone else comes along to trade with them.

It has two distinct sides:
The Bid Side (buyers):
- A list of people who wants to buy.
- Priority: Sorted from highest price to lowest.
- Best Bid: The highest price anyone is currently willing to pay.  This is the "top" of the buy side.

Price       | Quantity | Orders
-------------|---------------|--------
$100.50 | 500           | [Order1, Order2]
$100.49 | 300           | [Order3]
$100.48 | 200           | [Order4]


 The Ask Side (Sellers):
- A list of people who want to sell.
- Sorted from lowest price to highest.
- The lowest price anyone is currently willing to accept. This is the "top" of the sell side.

Price       | Quantity | Orders
-------------|--------------|--------
$100.51  | 400         | [Order5]
$100.52  | 600         | [Order6, Order7]
$100.53  | 100         | [Order8]





Formulas
The Spread: The gap between the best bid and the best ask. A "tight" spread indicates a very active, liquid market. I.e.,
		Spread = Ask min - Bid max

Mid price: halfway of two sides. This is often used as the “true” value of the asset but not particularly the trading price.
	Mid-price: Ask min + Bid max2


Order Types
Market Orders: Execute immediately at best available price
Limit Orders:  Execute only at specified price or better
Immediate or Cancel:  Execute what you can immediately, cancel rest
Fill or Kill:  Execute entire order or cancel completely
Good Till Cancel: Stays in book until filled or manually cancelled


How the "Matching Engine" Works
When a new order arrives, a program called the Matching Engine decides what happens. Most modern exchanges use a rule called Price-Time Priority (also known as FIFO):


Price Priority: The best price always gets filled first. A buyer offering $10.01 will always jump ahead of a buyer offering $10.00.
Time Priority: If two people offer the same price, the person who placed their order first gets filled first.
Execution: If a "Market Order" to buy comes in, the matching engine takes the cheapest available "Asks" from the book until the order is filled.


What is Slippage? 
If you place a massive market order that is larger than the amount available at the "Best Ask," the engine will move to the next (more expensive) price level to finish your order. This difference between your expected price and actual price is called slippage.


Levels of Data Transparency
Not all traders see the same amount of information. Exchanges categorize LOB data into “Levels”:


Data Level
What you see
Used by
Level 1
Only the Best Bid and Best Ask.
Casual retail investors.
Level 2
The top 5–10 price levels on both sides (The "Depth").
Day traders and scalpers.
Level 3
The entire book, including who placed which order.
Market makers and institutions.




Concepts to learn about more:
Iceberg Orders ( is interesting give a try :) )
Order Book Imbalance
Spoofing




DAY 2	

Live Stimulation
The Starting State
Right now, the market is quiet. Buyers and sellers have placed their "limit" orders, but nobody has matched yet.


Side
Price
Quantity (Shares)
Total Available
ASK
$100.10
1,000
1,500
ASK
$100.05
400
500
ASK (Best)
$100.02
100
100
---
SPREAD
$0.04
---
BID (Best)
$99.98
200
200
BID
$99.95
500
700
BID
$99.90
1,000
1,700




The Incoming Order
       
        	A large institutional trader decides they need to buy 600 shares of TECH immediately.
 They place a Market Buy Order (“I don’t care about the price, just give me 600 shares from whoever is selling right now.”) for 600 shares.
	

The Execution
The Matching Engine looks at the Ask Side and starts “eating” the orders from the cheapest to the most expensive.

Level 1: It takes all 100 shares available at $100.02 (Remaining 500 shares)
Level 2: It takes all 400 shares available at $100.05 (Remaining 100 shares)
Level 3: It takes 100 shares from the 1000 available at $100.10 (order fulfilled)

The Aftermath
The LOB has now changed. Because the buyer “cleared out” the lower price levels, the market price has moved up now.

The New Ask Side:
New Best Ask: $100.10 ( with 900 shares remaining).
The $100.02 and $100.05 levels are gone.
   
	The Result for the Trader: The trader didn’t pay $100.02 for all their shares. They paid a 
	Weighted Average Price : (100 * 100.02) + (400 * 100.05) + (100*100.10)600  $100.053
	
	The lesson: This trader experienced Slippage. They started buying when the price was $100.02, but their own demand pushed the final price they paid up to $100.05

SYSTEM OVERVIEW AND DESIGN FLOW:

https://drive.google.com/file/d/1sA6Wl_wTs2CbTl6IByC315qXqgEFgSkT/view?usp=drive_link






## Getting Started

### Prerequisites
- C++ 17 or higher
- Python 3.8+
- CMake 3.15+

### Building the Project
```bash
git clone <repository-url>
cd limit_order_book
mkdir build && cd build
cmake ..
make
```

### Running Tests
```bash
./tests/lob_tests
```

## Project Structure
```
limit_order_book/
├── src/           # C++ implementation
├── python/        # Python research and utilities
├── tests/         # Test suite
└── docs/          # Documentation
```

## Next Steps
- [ ] Implement core LOB data structures
- [ ] Build matching engine
- [ ] Add performance benchmarks
- [ ] Create Python bindings

---

**Author:** Aaditya  
**Status:** In Progress
=======
# limitorderbook
