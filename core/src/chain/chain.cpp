#include <chain.h>
#include <rathcrypto.h>


// TODO: Implement
std::unique_ptr<UndoBlock> Chain::make_undo_block(Block &block) {
    // Iterate through transactions to get hashes and to construct undo coin records
    std::vector<uint32_t> transaction_hashes;
    std::vector<std::unique_ptr<UndoCoinRecord>> undo_coin_records;
    for (auto &transaction : block.transactions) {
        // add transaction hash
        transaction_hashes.push_back(RathCrypto::hash(Transaction::serialize(*transaction)));

        std::vector<uint32_t> utxo;
        std::vector<uint32_t> amounts;
        std::vector<uint32_t> public_keys;

        // constrict undo coin records
        for (auto &transaction_input : transaction->transaction_inputs) {
            CoinLocator coin_locator = CoinLocator(transaction_input->reference_transaction_hash,
                                                   transaction_input->utxo_index);
        }

        for (auto &transaction_output : transaction->transaction_outputs) {

        }

        std::unique_ptr<UndoCoinRecord> undo_coin_record = std::make_unique<UndoCoinRecord>(transaction->version,
                                                                                            utxo, amounts,
                                                                                            public_keys);
        undo_coin_records.push_back(std::move(undo_coin_record));
    }

    return std::make_unique<UndoBlock>(transaction_hashes, std::move(undo_coin_records));
}

/// Helper function for hashing a block
uint32_t hash_block(Block &block) {
    return RathCrypto::hash(Block::serialize(block));
}

/// Helper function for keeping track of blocks recently added to the active chain
void Chain::update_recent_hashes(Block &block) {
    // remove oldest element if the deque has 5 or more elements
    if (_recent_block_hashes.size() >= 5) {
        _recent_block_hashes.pop_front();
    }

    _recent_block_hashes.push_back(RathCrypto::hash(Block::serialize(block)));
}

/// Helper function for building a FileInfo from a BlockRecord
FileInfo block_record_to_file_info(BlockRecord &block_record) {
    return FileInfo(block_record.block_file_stored, block_record.block_offset_start, block_record.block_offset_end);
}

/// Helper function for getting a block from a BlockRecord
std::unique_ptr<Block> Chain::block_record_to_block(BlockRecord &block_record) {
    FileInfo file_info = block_record_to_file_info(block_record);
    std::basic_string block = _chain_writer->read_block(file_info);
    return Block::deserialize(block);
}

Chain::Chain() : _active_chain_length(1), _active_chain_last_block(construct_genesis_block()),
                 _block_info_database(std::make_unique<BlockInfoDatabase>()),
                 _coin_database(std::make_unique<CoinDatabase>()), _chain_writer(std::make_unique<ChainWriter>()) {

    std::unique_ptr<UndoBlock> genesis_undo_block = make_undo_block(*_active_chain_last_block);

    // Write genesis block and undo block to disk
    std::unique_ptr<BlockRecord> genesis_block_record = _chain_writer->store_block(*_active_chain_last_block, 1,
                                                                                   *genesis_undo_block);

    // Store genesis block record in block info database
    _block_info_database->store_block_record(hash_block(*_active_chain_last_block), *genesis_block_record);

    // Add genesis block to recent block hashes
    update_recent_hashes(*_active_chain_last_block);
}

// TODO: Test implementation
void Chain::handle_block(std::unique_ptr<Block> block) {
    // Check if the block is being appended onto the main chain
    const bool appended_on_main_chain =
            hash_block(*_active_chain_last_block) == block->block_header->previous_block_hash;

    if (appended_on_main_chain) {
        // update UTXO and mempool
        _coin_database->validate_and_store_block(std::move(block->transactions));
    }

    if (_coin_database->validate_block(block->transactions)) {
        uint32_t block_height;

        if (appended_on_main_chain) {
            block_height = _active_chain_length + 1;
        } else {
            // find previous block's height and add 1
            std::tuple<bool, std::unique_ptr<BlockRecord>> prev_block = _block_info_database->get_block_record(
                    block->block_header->previous_block_hash);
            block_height = get<1>(prev_block)->height + 1;
        }

        // Create undo block
        std::unique_ptr<UndoBlock> undo_block = make_undo_block(*block);

        // Store block and undo block on disk
        std::unique_ptr<BlockRecord> block_record = _chain_writer->store_block(*block, block_height, *undo_block);
        _block_info_database->store_block_record(hash_block(*block), *block_record);

        // Update chain
        if (appended_on_main_chain) {
            update_recent_hashes(*block);
            _active_chain_length = _active_chain_length + 1;
            _active_chain_last_block = std::move(block);
        } else if (block_height > _active_chain_length) {
            // fork has surpassed active chain
            std::vector<std::shared_ptr<Block>> forked_blocks = get_forked_blocks_stack(
                    hash_block(*_active_chain_last_block));
            std::vector<std::unique_ptr<UndoBlock>> undo_blocks = get_undo_blocks_queue(_active_chain_length);

            // Reverse UTXO with undo_blocks
            _coin_database->undo_coins(std::move(undo_blocks));

            // Add forked blocks to coin database
            for (auto &forked_block : forked_blocks) {
                _coin_database->store_block(forked_block->transactions);
            }
        }
    }
}

// TODO: Test implementation
void Chain::handle_transaction(std::unique_ptr<Transaction> transaction) {
    _coin_database->validate_and_store_transaction(std::move(transaction));
}

// TODO: Test implementation
uint32_t Chain::get_chain_length(uint32_t block_hash) {
    std::tuple<bool, std::unique_ptr<BlockRecord>> block = _block_info_database->get_block_record(block_hash);
    if (get<0>(block)) {
        return get<1>(block)->height;
    } else {
        return 0;
    }
}

// TODO: Test implementation
std::unique_ptr<Block> Chain::get_block(uint32_t block_hash) {
    // query block_info_database for corresponding block record
    std::tuple<bool, std::unique_ptr<BlockRecord>> block_record = _block_info_database->get_block_record(block_hash);

    if (get<0>(block_record)) {
        FileInfo file_info = block_record_to_file_info(*get<1>(block_record));
        std::basic_string block = _chain_writer->read_block(file_info);
        return Block::deserialize(block);
    } else {
        return nullptr;
    }
}

// TODO: Test implementation
std::vector<std::unique_ptr<Block>> Chain::get_active_chain(uint32_t start, uint32_t end) {
    // check valid bounds
    if ((end < start) || start == 0 || end > _active_chain_length) {
        std::vector<std::unique_ptr<Block>> empty_vector;
        return empty_vector;
    }

    // keep track of the blocks to return
    std::vector<std::unique_ptr<Block>> return_blocks;

    // get block_record for _active_chain_last block
    std::tuple<bool, std::unique_ptr<BlockRecord>> curr_block = _block_info_database->get_block_record(
            hash_block(*_active_chain_last_block));

    // traverse chain and add blocks within bounds to return vector
    while (get<0>(curr_block) && get<1>(curr_block)->height >= start) {
        if (get<1>(curr_block)->height <= end) {
            FileInfo file_info = block_record_to_file_info(*get<1>(curr_block));
            std::basic_string<char, std::char_traits<char>, std::allocator<char>> serialized_block = _chain_writer->read_block(
                    file_info);
            return_blocks.push_back(Block::deserialize(serialized_block));
        }

        curr_block = _block_info_database->get_block_record(get<1>(curr_block)->block_header->previous_block_hash);
    }

    // Reverse vector so that blocks are ordered from oldest to newest
    std::reverse(return_blocks.begin(), return_blocks.end());

    return return_blocks;
}

// TODO: Test implementation
std::vector<uint32_t> Chain::get_active_chain_hashes(uint32_t start, uint32_t end) {
    std::vector<std::unique_ptr<Block>> chain = get_active_chain(start, end);
    std::vector<uint32_t> hashes;

    hashes.reserve(chain.size());
    for (auto &block : chain) {
        hashes.push_back(hash_block(*block));
    }

    return hashes;
}

// TODO: Test implementation
std::unique_ptr<Block> Chain::get_last_block() {
    // Copy over block header info
    std::unique_ptr<BlockHeader> blockHeader = std::make_unique<BlockHeader>(
            _active_chain_last_block->block_header->version,
            _active_chain_last_block->block_header->previous_block_hash,
            _active_chain_last_block->block_header->merkle_root,
            _active_chain_last_block->block_header->difficulty_target,
            _active_chain_last_block->block_header->nonce,
            _active_chain_last_block->block_header->timestamp);

    // Copy over transactions
    std::vector<std::unique_ptr<Transaction>> transactions;
    for (auto &transaction : _active_chain_last_block->transactions) {
        std::vector<std::unique_ptr<TransactionInput>> transactionInputs;
        std::vector<std::unique_ptr<TransactionOutput>> transactionOutputs;

        for (auto &txInput : transaction->transaction_inputs) {
            std::unique_ptr<TransactionInput> tx_input = std::make_unique<TransactionInput>(
                    txInput->reference_transaction_hash, txInput->utxo_index,
                    txInput->signature);
            transactionInputs.push_back(std::move(tx_input));
        }

        for (auto &txOutput : transaction->transaction_outputs) {
            std::unique_ptr<TransactionOutput> tx_output = std::make_unique<TransactionOutput>(txOutput->amount,
                                                                                               txOutput->public_key);
            transactionOutputs.push_back(std::move(tx_output));
        }

        std::unique_ptr<Transaction> tx = std::make_unique<Transaction>(std::move(transactionInputs),
                                                                        std::move(transactionOutputs),
                                                                        transaction->version,
                                                                        transaction->lock_time);
        transactions.push_back(std::move(tx));
    }

    return std::make_unique<Block>(std::move(blockHeader), std::move(transactions));
}

// TODO: Test implementation
uint32_t Chain::get_last_block_hash() {
    return hash_block(*_active_chain_last_block);
}

// TODO: Test implementation
uint32_t Chain::get_active_chain_length() const {
    return _active_chain_length;
}

// TODO: Test implementation
std::vector<std::unique_ptr<UndoBlock>> Chain::get_undo_blocks_queue(uint32_t branching_height) {
    // Check bounds
    if (branching_height == 0 || branching_height > _active_chain_length) {
        std::vector<std::unique_ptr<UndoBlock>> empty_vector;
        return empty_vector;
    }

    // keep track of the undo blocks to return
    std::vector<std::unique_ptr<UndoBlock>> undo_blocks;

    // Start at last block on active chain
    std::tuple<bool, std::unique_ptr<BlockRecord>> curr_block = _block_info_database->get_block_record(
            hash_block(*_active_chain_last_block));

    // Retrieve undo blocks until we reach the block at the branching_height
    while (get<0>(curr_block) && get<1>(curr_block)->height >= branching_height) {
        FileInfo file_info = block_record_to_file_info(*get<1>(curr_block));
        std::unique_ptr<UndoBlock> deserialized_block = UndoBlock::deserialize(
                _chain_writer->read_undo_block(file_info));
        undo_blocks.push_back(std::move(deserialized_block));
    }

    return undo_blocks;
}

// TODO: Test implementation
std::vector<std::shared_ptr<Block>> Chain::get_forked_blocks_stack(uint32_t starting_hash) {
    std::tuple<bool, std::unique_ptr<BlockRecord>> curr_block = _block_info_database->get_block_record(starting_hash);

    // keep track of the forked blocked to return
    std::vector<std::shared_ptr<Block>> forked_blocks;

    // add blocks to forked_blocks while the prev block hash isn't the common ancestor
    while (get<0>(curr_block) && std::find(_recent_block_hashes.begin(), _recent_block_hashes.end(),
                                           get<1>(curr_block)->block_header->previous_block_hash) !=
                                 _recent_block_hashes.end()) {
        std::shared_ptr<Block> block = block_record_to_block(*get<1>(curr_block));
        forked_blocks.push_back(block);
        curr_block = _block_info_database->get_block_record(hash_block(*block));
    }

    // might need to include final common ancestor block in vector

    return forked_blocks;
}

// TODO: Test implementation
std::unique_ptr<Block> Chain::construct_genesis_block() {
    std::unique_ptr<BlockHeader> blockHeader = std::make_unique<BlockHeader>(1, 0, 0, 0, 0, 0);

    std::vector<std::unique_ptr<TransactionInput>> transactionInputs;
    std::vector<std::unique_ptr<TransactionOutput>> transactionOutputs;

    transactionOutputs.push_back(std::make_unique<TransactionOutput>(100, 12345));
    transactionOutputs.push_back(std::make_unique<TransactionOutput>(200, 67891));
    transactionOutputs.push_back(std::make_unique<TransactionOutput>(300, 23456));


    std::unique_ptr<Transaction> transaction = std::make_unique<Transaction>(std::move(transactionInputs),
                                                                             std::move(transactionOutputs));

    std::vector<std::unique_ptr<Transaction>> transactions;
    transactions.push_back(std::move(transaction));

    return std::make_unique<Block>(std::move(blockHeader), std::move(transactions));
}
