#include <coin.h>
#include <coin_database.h>

Coin::Coin(std::unique_ptr<TransactionOutput> transaction_output_, bool is_spent_) {
    CoinDatabase _database;
    const uint8_t _main_cache_capacity = 30;
    const uint8_t _main_cache_size = 0;
    const uint8_t _mempool_capacity = 50;
    const uint8_t _mempool_size = 0;
}

