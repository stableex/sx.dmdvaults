#include <sx.uniswap/uniswap.hpp>

using namespace eosio;
using namespace std;

namespace dmdvaults {

    const extended_symbol EOS  { symbol{"EOS",4}, "eosio.token"_n };
    const name id = "dmdvaults"_n;
    const name code = "dmddappvault"_n;

    struct [[eosio::table("stat")]] currencystat {
        asset       supply;
        asset       max_supply;
        name        issuer;

        uint64_t primary_key() const { return supply.symbol.code().raw(); }
    };
    typedef eosio::multi_index< "stat"_n, currencystat > reserves;

    /**
     * ## STATIC `get_fee`
     *
     * Get total fee. Applied only on withdrawals
     *
     * ### params
     *
     *
     * ### example
     *
     * ```c++
     * const uint64_t fee = dmdvaults::legacy::get_fee( );
     * // => 10
     * ```
     */
    static uint64_t get_fee()
    {
        return 10;
    }

    /**
     * ## STATIC `get_amount_out`
     *
     * Given an input amount of an asset and pair reserves, returns the output amount of the other asset
     *
     * ### params
     *
     * - `{uint64_t} amount_in` - amount input
     * - `{uint64_t} reserve_in` - reserve input
     * - `{uint64_t} reserve_out` - reserve output
     * - `{uint64_t} fee` - trading fee (pips 1/10000)
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const uint64_t amount_in = 10000;
     * const uint64_t reserve_in = 45851931234;
     * const uint64_t reserve_out = 46851931234;
     * const uint64_t fee = 5;
     *
     * // Calculation
     * const uint64_t amount_out = dmdvaults::get_amount_out( amount_in, reserve_in, reserve_out, fee );
     * // => 9996
     * ```
     */
    static uint64_t get_amount_out( const uint64_t amount_in, const uint64_t reserve_in, const uint64_t reserve_out,  const uint64_t fee )
    {
        // checks
        eosio::check(amount_in > 0, "sx.dmdvaults: INSUFFICIENT_INPUT_AMOUNT");
        eosio::check(reserve_in > 0 && reserve_out > 0, "sx.dmdvaults: INSUFFICIENT_LIQUIDITY");

        // calculations: no-fee quote, fee applied afterwards
        auto amount_out = uniswap::quote(amount_in, reserve_in, reserve_out) * (10000 - fee) / 10000 + (fee>0?1:0); //+1 to account for rounding error


        return amount_out;
    }

namespace legacy {
    //see https://eos.eosq.eosnation.io/tx/0ad709d9f17c6c6cfe5fd54fa7918b7f29f29b4a0db1d2b35c866559a41dcde8

    const name id = "dmd.legacy"_n;
    const name code = "eosdmdvaults"_n;
    const name vault = "dvaultproxy1"_n;

    const extended_symbol DEOS { symbol{"DEOS",4}, "eosdmddtoken"_n };


    struct [[eosio::table("rexbal")]] rex_balance {
        uint8_t     version;
        name        owner;
        asset       vote_stake;
        asset       rex_balance;
        int64_t     matured_rex;
        vector<pair<time_point_sec, int64_t>> rex_maturities;

        uint64_t primary_key() const { return owner.value; }
    };
    typedef eosio::multi_index< "rexbal"_n, rex_balance > rexbal;

    struct [[eosio::table("rexpool")]] rex_pool {
        uint8_t     version;
        asset       total_lent;
        asset       total_unlent;
        asset       total_rent;
        asset       total_lendable;
        asset       total_rex;
        asset       namebid_proceeds;
        uint64_t    loan_num;

        uint64_t primary_key() const { return loan_num; }
    };
    typedef eosio::multi_index< "rexpool"_n, rex_pool > rexpool;

    static int64_t _EOS_value_amount = 0;

    //Get EOS equivalent for REX resources for {account} account

    static asset get_rex_value(const name& account) {
        asset eos {_EOS_value_amount, EOS.get_symbol()};

        if(eos.amount) return eos;  //already calculated _EOS_value_amount

        dmdvaults::legacy::rexbal _rexbal( "eosio"_n, "eosio"_n.value );
        auto rexbalit = _rexbal.find(account.value);
        if(rexbalit == _rexbal.end()) return eos;

        dmdvaults::legacy::rexpool _rexpool( "eosio"_n, "eosio"_n.value );
        if(_rexpool.begin() == _rexpool.end()) return eos;
        auto pool = *_rexpool.begin();

        double_t price = static_cast<double_t>(pool.total_unlent.amount + pool.total_lent.amount) / pool.total_rex.amount;

        eos.amount = _EOS_value_amount = rexbalit->rex_balance.amount * price;

        return eos;
    }

    /**
     * ## STATIC `get_reserves`
     *
     * Get EOS/DEOS reserves from a vault contract
     *
     * ### params
     *
     * - `{symbol} sort` - symbol of a reserve that should go first
     *
     * ### example
     *
     * ```c++
     * const auto [ reserve0, reserve1 ]  = dmdvaults::legacy::get_reserves( {"EOS",4} );
     * // reserve0 => {"balance": "55988.4608 EOS"}
     * // reserve1 => {"55995.6259 DEOS"}
     * ```
     */
    static pair<asset, asset> get_reserves( const symbol sort )
    {
        check(sort==EOS.get_symbol() || sort==DEOS.get_symbol(), "sx.dmdvaults: Only EOS/DEOS pair available");
        dmdvaults::reserves _reserves( DEOS.get_contract(), DEOS.get_symbol().code().raw() );

        auto deos = _reserves.get( DEOS.get_symbol().code().raw(), "sx.dmdvaults: No DEOS row in eosdmddtoken stat table" ).supply;
        auto eos  = eosio::token::get_balance( EOS.get_contract(), dmdvaults::legacy::vault, EOS.get_symbol().code() );

        eos += get_rex_value(dmdvaults::legacy::vault);

        return sort==EOS.get_symbol() ? pair<asset, asset>{eos, deos} : pair<asset, asset>{deos, eos};
    }

}

namespace multi {

    //see: https://eos.eosq.eosnation.io/tx/04e6dcf50e876ddadfdc1a95b30231eebf1437320d263d1b7f3cb7e04f3308b5

    const name id = "dmd.multi"_n;
    const name code = "dmddappvault"_n;
    const name vault = "dvaultbgstak"_n;

    const extended_symbol BG { symbol{"BG",4}, "bgbgbgbgbgbg"_n };
    const extended_symbol DBG { symbol{"DBG",4}, "dvaultdtoken"_n };


    struct [[eosio::table("stake")]] stake_row {
        name        player;
        asset       amount;

        uint64_t primary_key() const { return player.value; }
    };
    typedef eosio::multi_index< "stake"_n, stake_row > stake;

    struct [[eosio::table("resvwd")]] resvwd_row {
        int64_t     id;
        //table incomplete because we only need to check if there are any rows

        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index< "resvwd"_n, resvwd_row > resvwd;

    /**
     * ## STATIC `get_reserves`
     *
     * Get BG/DBG reserves from a vault contract
     *
     * ### params
     *
     * - `{symbol} sort` - symbol of a reserve that should go first
     *
     * ### example
     *
     * ```c++
     * const auto [ reserve0, reserve1 ]  = dmdvaults::multi::get_reserves( {"BG",4} );
     * // reserve0 => {"balance": "55988.4608 BG"}
     * // reserve1 => {"55995.6259 DBG"}
     * ```
     */
    static pair<asset, asset> get_reserves( const symbol sort )
    {
        check(sort==BG.get_symbol() || sort==DBG.get_symbol(), "sx.dmdvaults: Only BG/DBG pair available");

        dmdvaults::reserves _reserves( DBG.get_contract(), DBG.get_symbol().code().raw() );

        auto dbg = _reserves.get( DBG.get_symbol().code().raw(), "sx.dmdvaults: No DBG row in dvaultdtoken stat table" ).supply;
        auto bg  = eosio::token::get_balance( BG.get_contract(), dmdvaults::multi::vault, BG.get_symbol().code() );

        return sort==BG.get_symbol() ? pair<asset, asset>{bg, dbg} : pair<asset, asset>{bg, dbg};
    }

    /**
     * ## STATIC `get_amount_out`
     *
     * Given an input amount of an asset and pair reserves, returns the output amount of the other asset
     *
     * ### params
     *
     * - `{uint64_t} amount_in` - amount input
     * - `{uint64_t} reserve_in` - reserve input
     * - `{uint64_t} reserve_out` - reserve output
     * - `{uint64_t} fee` - trading fee (pips 1/10000)
     *
     * ### example
     *
     * ```c++
     * // Inputs
     * const uint64_t amount_in = 10000;
     * const uint64_t reserve_in = 45851931234;
     * const uint64_t reserve_out = 46851931234;
     * const uint64_t fee = 5;
     *
     * // Calculation
     * const uint64_t amount_out = dmdvaults::get_amount_out( amount_in, reserve_in, reserve_out, fee );
     * // => 9996
     * ```
     */

    static uint64_t get_amount_out( const uint64_t amount_in, const uint64_t reserve_in, const uint64_t reserve_out, const symbol sym_out, const uint64_t fee )
    {
        // checks
        eosio::check(amount_in > 0, "sx.dmdvaults: INSUFFICIENT_INPUT_AMOUNT");
        eosio::check(reserve_in > 0 && reserve_out > 0, "sx.dmdvaults: INSUFFICIENT_LIQUIDITY");

        // calculations: no-fee quote, fee applied afterwards
        auto amount_out = uniswap::quote(amount_in, reserve_in, reserve_out) * (10000 - fee) / 10000 + (fee>0?1:0); //+1 to account for rounding error

        if(sym_out == BG.get_symbol()) {
            dmdvaults::multi::stake _resvwd( "dmddappvault"_n, "dmddappvault"_n.value );
            if(_resvwd.begin() != _resvwd.end()) return 0;  //reserved, so can't withdraw

            dmdvaults::multi::stake _stake( "dividend.bg"_n, "dividend.bg"_n.value );
            auto staked = _stake.get(dmdvaults::multi::vault.value, "No staked BG on dividend.bg").amount.amount;
            auto balance  = eosio::token::get_balance( BG.get_contract(), dmdvaults::multi::vault, BG.get_symbol().code() ).amount;

            if(amount_out > balance - staked) amount_out = 0;
        }

        return amount_out;
    }
}


}