/*
 * This file belongs to Brown University's computer
 * science department as part of csci1951L - Blockchains
 * and cryptocurrencies.
 *
 * This software was developed by Colby Anderson (HTA)
 * and Parker Ljung (TA) of csci1951L in Summer of 2021.
 */

#include <coin_database.h>
#include <rathcrypto.h>

/// Helper function for getting a hash for a transaction
uint32_t hash_transaction(const Transaction &transaction) {
    return RathCrypto::hash(Transaction::serialize(transaction));
}

CoinLocator::CoinLocator(
        uint32_t transaction_hash_, uint8_t output_index_)
        : transaction_hash(transaction_hash_),
          output_index(output_index_) {}

std::string CoinLocator::serialize(const CoinLocator &coin_locator) {
    return std::to_string(coin_locator.transaction_hash) + "-" +
           std::to_string(coin_locator.output_index);
}

std::unique_ptr<CoinLocator> CoinLocator::deserialize(const std::string &serialized_coin_locator) {
    std::string transaction_hash = serialized_coin_locator.substr(0, serialized_coin_locator.find("-"));
    std::string output_index = serialized_coin_locator.substr(serialized_coin_locator.find("-") + 1,
                                                              serialized_coin_locator.size());
    return std::make_unique<CoinLocator>(std::stoul(transaction_hash, nullptr, 0),
                                         std::stoul(output_index, nullptr, 0));
}

std::string CoinLocator::serialize_from_construct(uint32_t transaction_hash, uint8_t output_index) {
    return std::to_string(transaction_hash) + "-" +
           std::to_string(output_index);
}

CoinDatabase::CoinDatabase()
        : _database(std::make_unique<Database>()), _main_cache_capacity(30), _main_cache_size(0), _mempool_capacity(50),
          _mempool_size(0) {}


// TODO: Test implementation
bool CoinDatabase::validate_block(const std::vector<std::unique_ptr<Transaction>> &transactions) {
    for (auto& transaction : transactions) {
        if (!validate_transaction(*transaction)) {
            return false;
        }
    }
    return true;
}

// TODO: Implement
bool CoinDatabase::validate_transaction(const Transaction &transaction) {
    for (auto &transaction_input : transaction.transaction_inputs) {
        // Make a coin locator for transaction input
        CoinLocator coin_locator = CoinLocator(transaction_input->reference_transaction_hash,
                                               transaction_input->utxo_index);

        // Check main cache for coin
        if (_main_cache.find(CoinLocator::serialize(coin_locator)) != _main_cache.end()) {
            // Coin found
            Coin& coin = *_main_cache.find(CoinLocator::serialize(coin_locator))->second;
            return !coin.is_spent;
        } else {
            // Coin not found in main cache... check underlying database
            std::basic_string<char, std::char_traits<char>, std::allocator<char>> res = _database->get_safely(
                    std::to_string(transaction_input->reference_transaction_hash));
            if (!res.empty()) {
                // found in underlying database
                std::unique_ptr<CoinRecord> coin_record = CoinRecord::deserialize(res);
                return true;
            } else {
                // not found in underlying database
                return false;
            }
        }
    }

    return false;
}

// TODO: Test implementation
void CoinDatabase::store_block(const std::vector<std::unique_ptr<Transaction>> &transactions) {
    // Remove transactions from mempool
    remove_transactions_from_mempool(transactions);

    // store transactions in main cache
    store_transactions_to_main_cache(transactions);
}

// TODO: Test implementation
void CoinDatabase::store_transaction(std::unique_ptr<Transaction> transaction) {
    store_transaction_in_mempool(std::move(transaction));
}

// TODO: Test implementation
bool CoinDatabase::validate_and_store_block(std::vector<std::unique_ptr<Transaction>> transactions) {
    if (validate_block(transactions)) {
        store_block(transactions);
        return true;
    } else {
        return false;
    }
}

// TODO: Test implementation
bool CoinDatabase::validate_and_store_transaction(std::unique_ptr<Transaction> transaction) {
    if (validate_transaction(*transaction)) {
        store_transaction(std::move(transaction));
        return true;
    } else {
        return false;
    }
}

// TODO: Test implementation
void CoinDatabase::remove_transactions_from_mempool(const std::vector<std::unique_ptr<Transaction>> &transactions) {
    for (auto &transaction : transactions) {
        _mempool_cache.erase(hash_transaction(*transaction));
    }
}

// TODO: Implement
void CoinDatabase::store_transactions_to_main_cache(const std::vector<std::unique_ptr<Transaction>>& transactions) {
    for (auto &transaction : transactions) {
        std::vector<uint32_t> utxo;
        std::vector<uint32_t> amounts;
        std::vector<uint32_t> public_keys;

        for (auto &transaction_input : transaction->transaction_inputs) {
            // remove spent TXO from main cache
            CoinLocator coin_locator = CoinLocator(transaction_input->reference_transaction_hash,
                                                   transaction_input->utxo_index);

            if (_main_cache.find(CoinLocator::serialize(coin_locator)) != _main_cache.end()) {
                // in main cache
                _main_cache.erase(CoinLocator::serialize(coin_locator));
            } else {
                // remove from database
                _database->delete_safely(CoinLocator::serialize(coin_locator));
            }
        }

        for (auto &transaction_output : transaction->transaction_outputs) {
            // remove spent TXO from main cache
            utxo.push_back(transaction_output->amount);
            amounts.push_back(transaction_output->amount);
            public_keys.push_back(transaction_output->public_key);
        }

        CoinRecord coin_record = CoinRecord(transaction->version, utxo, amounts, public_keys);
    }
}

// TODO: Test implementation
void CoinDatabase::store_transaction_in_mempool(std::unique_ptr<Transaction> transaction) {
    if (_mempool_size < _mempool_capacity) {
        _mempool_cache.insert({hash_transaction(*transaction), std::move(transaction)});
    }
}

// TODO: Implement
void CoinDatabase::flush_main_cache() {
    for (auto &coin : _main_cache) {
        if (coin.second->is_spent) {
            std::unique_ptr<CoinLocator> coin_locator = CoinLocator::deserialize(coin.first);
        }
        _main_cache.erase(coin.first);
    }
}

void CoinDatabase::undo_coins(std::vector<std::unique_ptr<UndoBlock>> undo_blocks) {}



