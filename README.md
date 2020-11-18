# **`DMD Vaults`**

> Peripheral functions for interacting with DMD Vaults

## Dependencies

- [`sx.uniswap`](https://github.com/stableex/sx.uniswap)

## Quickstart

```c++
#include <sx.dmdvaults/dmdvaults.hpp>
#include <sx.uniswap/uniswap.hpp>

// user input
const asset quantity = asset{10000, symbol{"EOS", 4}};
const uint64_t pair_id = "deposit:1"; // EOS/DEOS pair

// get reserve and fee info
const auto [ reserve_in, reserve_out ] = dmdvaults::legacy::get_reserves( pair_id, quantity.symbol );
const uint8_t fee = dmdvaults::get_fee();

// calculate out price
const asset out = dmdvaults::get_amount_out( quantity, reserves_in, reserves_out, fee );
// => "0.9990 DEOS"
```
