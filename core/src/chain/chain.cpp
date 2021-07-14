#include <chain.h>
#include <rathcrypto.h>


// TODO: Implement
std::unique_ptr<UndoBlock> make_undo_block(Block &block) {

    // Get transaction hashes from block
    std::vector<uint32_t> transaction_hashes;
    std::vector<std::unique_ptr<UndoCoinRecord>> undo_coin_records;
    for (auto &transaction : block.transactions) {
        transaction_hashes.push_back(RathCrypto::hash(Transaction::serialize(*transaction)));
        undo_coin_records.push_back(std::make_unique<UndoCoinRecord>(transaction->version,))
    }

    // Create undo coin records

//    UndoCoinRecord(uint8_t version_,
//            std::vector<uint32_t> utxo_, std::vector<uint32_t> amounts_,
//            std::vector<uint32_t> public_keys_);



    return std::make_unique<UndoBlock>(transaction_hashes,)
}

void Chain::update_recent_hashes(Block &block) {
    // remove oldest element if the deque has 5 or more elements
    if (_recent_block_hashes.size() >= 5) {
        _recent_block_hashes.pop_front();
    }

    _recent_block_hashes.push_back(RathCrypto::hash(Block::serialize(block)));
}

Chain::Chain() {
    _active_chain_length = 1;
    _active_chain_last_block = construct_genesis_block();
    _block_info_database = std::make_unique<BlockInfoDatabase>();
    _coin_database = std::make_unique<CoinDatabase>();
    _chain_writer = std::make_unique<ChainWriter>();
    _recent_block_hashes;

    std::unique_ptr<UndoBlock> genesis_undo_block = make_undo_block(*_active_chain_last_block);

    // Write undo block to disk
    std::unique_ptr<FileInfo> undoBlockFileInfo = _chain_writer->write_undo_block(
            UndoBlock::serialize(*genesis_undo_block));

    // Write the genesis block to disk
    std::unique_ptr<FileInfo> genesisBlockFileInfo = _chain_writer->write_block(
            Block::serialize(*_active_chain_last_block));

    // Add genesis block to recent block hashes
    update_recent_hashes(*_active_chain_last_block);

    // Copy block header
    std::unique_ptr<BlockHeader> block_header_copy = std::make_unique<BlockHeader>(
            _active_chain_last_block->block_header->version,
            _active_chain_last_block->block_header->previous_block_hash,
            _active_chain_last_block->block_header->merkle_root,
            _active_chain_last_block->block_header->difficulty_target,
            _active_chain_last_block->block_header->nonce,
            _active_chain_last_block->block_header->timestamp);


    // Construct block record
    BlockRecord blockRecord = BlockRecord(std::move(block_header_copy),
                                          _active_chain_last_block->transactions.size(),
                                          1, *genesisBlockFileInfo,
                                          *undoBlockFileInfo);

    // Store the genesis block in the coin_database
    _block_info_database->store_block_record(RathCrypto::hash(Block::serialize(*_active_chain_last_block)),
                                             blockRecord);
}

// TODO: Implement
void Chain::handle_block(std::unique_ptr<Block> block) {
    // Check if the block is being appended onto the main chain
    const bool appended_on_main_chain =
            RathCrypto::hash(Block::serialize(*block)) == block->block_header->previous_block_hash;

    if (appended_on_main_chain) {
        // update UTXO and mempool
        _coin_database->validate_and_store_block(block->transactions);
    }

    if (_coin_database->validate_block(block->transactions)) {
        uint32_t block_height;

        if (appended_on_main_chain) {
            block_height = _active_chain_length + 1;
        } else {
            std::tuple<bool, std::unique_ptr<BlockRecord>> prev_block = _block_info_database->get_block_record(
                    block->block_header->previous_block_hash);
            block_height = get<1>(prev_block)->height + 1;
        }

        // Create undo block
        std::unique_ptr<UndoBlock> undo_block = make_undo_block(*block);

        // Store block and undo block on disk
        _chain_writer->store_block(*block, block_height, *undo_block);

        // Update chain
        if (appended_on_main_chain) {
            update_recent_hashes(*block);
            _active_chain_length = _active_chain_length + 1;
            _active_chain_last_block = std::move(block);
        } else if (block_height > _active_chain_length) {
            // fork has surpassed active chain
            std::vector<std::shared_ptr<Block>> forked_blocks = get_forked_blocks_stack(
                    RathCrypto::hash(Block::serialize(*_active_chain_last_block)));
            std::vector<std::unique_ptr<UndoBlock>> undo_blocks = get_undo_blocks_queue();

            // Reverse UTXO with undo_blocks
            _coin_database->undo_coins(undo_blocks);

            // Add forked blocks to coin database
            for (auto &forked_block : forked_blocks) {
                _coin_database->store_block(forked_block->transactions);
            }
        }
    }
}

void Chain::handle_transaction(std::unique_ptr<Transaction> transaction) {
    _coin_database->validate_and_store_transaction(std::move(transaction));
}

uint32_t Chain::get_chain_length(uint32_t block_hash) {
    std::tuple<bool, std::unique_ptr<BlockRecord>> block = _block_info_database->get_block_record(block_hash);
    if (get<0>(block)) {
        return get<1>(block)->height;
    } else {
        return 0;
    }
}

std::unique_ptr<Block> Chain::get_block(uint32_t block_hash) {
    // query block_info_database for corresponding block record
    std::tuple<bool, std::unique_ptr<BlockRecord>> block_record = _block_info_database->get_block_record(block_hash);

    if (get<0>(block_record)) {
        FileInfo file_info = FileInfo(get<1>(block_record)->block_file_stored, get<1>(block_record)->block_offset_start,
                                      get<1>(block_record)->block_offset_end);
        std::basic_string block = _chain_writer->read_block(file_info);
        return Block::deserialize(block);
    } else {
        return nullptr;
    }
}

// TODO: Implement
std::vector<std::unique_ptr<Block>> Chain::get_active_chain(uint32_t start, uint32_t end) {
    // check valid bounds
    if ((end < start) || start == 0 || end > _active_chain_length) {
        std::vector<std::unique_ptr<Block>> empty_vector;
        return empty_vector;
    }

    // get block_record for _active_chain_last block
    std::vector<std::unique_ptr<Block>> return_blocks;
    std::tuple<bool, std::unique_ptr<BlockRecord>> curr_block = _block_info_database->get_block_record(
            RathCrypto::hash(Block::serialize(*_active_chain_last_block)));

    // traverse chain and add blocks within bounds to return vector
    while (get<1>(curr_block)->height >= start) {
        if (get<1>(curr_block)->height <= end) {
            FileInfo file_info = FileInfo(get<1>(curr_block)->block_file_stored, get<1>(curr_block)->block_offset_start,
                                          get<1>(curr_block)->block_offset_end);
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

// TODO: Implement
std::vector<uint32_t> Chain::get_active_chain_hashes(uint32_t start, uint32_t end) {
    std::vector<std::unique_ptr<Block>> chain = get_active_chain(start, end);
    std::vector<uint32_t> hashes;

    for (auto &block : chain) {
        hashes.push_back(RathCrypto::hash(Block::serialize(*block)));
    }

    return hashes;
}

// TODO: Check implementation.
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
    for (std::unique_ptr<Transaction> &transaction : _active_chain_last_block->transactions) {
        std::vector<std::unique_ptr<TransactionInput>> transactionInputs;
        std::vector<std::unique_ptr<TransactionOutput>> transactionOutputs;

        for (std::unique_ptr<TransactionInput> &txInput : transaction->transaction_inputs) {
            transactionInputs.push_back(
                    std::make_unique<TransactionInput>(txInput->reference_transaction_hash, txInput->utxo_index,
                                                       txInput->signature));
        }

        for (std::unique_ptr<TransactionOutput> &txOutput : transaction->transaction_outputs) {
            transactionOutputs.push_back(std::make_unique<TransactionOutput>(txOutput->amount, txOutput->public_key));
        }

        transactions.push_back(
                std::make_unique<Transaction>(transactionInputs, transactionOutputs, transaction->version,
                                              transaction->lock_time));
    }

    return std::make_unique<Block>(std::move(blockHeader), transactions);
}

// TODO: Check implementation.
uint32_t Chain::get_last_block_hash() {
    return RathCrypto::hash(Block::serialize(*_active_chain_last_block));
}

// TODO: Check implementation.
uint32_t Chain::get_active_chain_length() const {
    return _active_chain_length;
}

// TODO: Implement
std::vector<std::unique_ptr<UndoBlock>> Chain::get_undo_blocks_queue(uint32_t branching_height) {
    // Check bounds
    if (branching_height == 0 || branching_height > _active_chain_length) {
        std::vector<std::unique_ptr<UndoBlock>> empty_vector;
        return empty_vector;
    }

    std::vector<std::unique_ptr<UndoBlock>> undo_blocks;

    // Start at last block on active chain
    std::tuple<bool, std::unique_ptr<BlockRecord>> curr_block = _block_info_database->get_block_record(
            RathCrypto::hash(Block::serialize(*_active_chain_last_block)));

    // Retrieve undo blocks until we reach the block at the branching_height
    while (get<1>(curr_block)->height >= branching_height) {
        FileInfo file_info = FileInfo(get<1>(curr_block)->undo_file_stored, get<1>(curr_block)->undo_offset_start,
                                      get<1>(curr_block)->undo_offset_end);
        undo_blocks.push_back(UndoBlock::deserialize(_chain_writer->read_undo_block(file_info)));
    }

    return undo_blocks;
}

// TODO: Implement
std::vector<std::shared_ptr<Block>> Chain::get_forked_blocks_stack(uint32_t starting_hash) {
    std::tuple<bool, std::unique_ptr<BlockRecord>> curr_block = _block_info_database->get_block_record(starting_hash);

    // return empty vector if starting_hash wasn't found
    if (!get<0>(curr_block)) {
        std::vector<std::shared_ptr<Block>> empty_vector;
        return empty_vector;
    }

    uint32_t common_ancestor;
    int curr_iteration = 0;

    while (curr_iteration <= 5) {
        if (std::find(_recent_block_hashes.begin(), _recent_block_hashes.end(),
                      get<1>(curr_block)->block_header->previous_block_hash) != _recent_block_hashes.end()) {
            common_ancestor = get<1>(curr_block)->block_header->previous_block_hash;
            break;
        } else {
            curr_iteration = curr_iteration + 1;
            curr_block = _block_info_database->get_block_record(get<1>(curr_block)->block_header->previous_block_hash);
        }
    }

    if (common_ancestor) {
        return
    }
}

// TODO: Check implementation.
std::unique_ptr<Block> Chain::construct_genesis_block() {
    std::unique_ptr<BlockHeader> blockHeader = std::make_unique<BlockHeader>(1, 0, 0, 0, 0, 0);

    std::vector<std::unique_ptr<TransactionInput>> transactionInputs;
    std::vector<std::unique_ptr<TransactionOutput>> transactionOutputs;

    transactionOutputs.push_back(std::make_unique<TransactionOutput>(100, 12345));
    transactionOutputs.push_back(std::make_unique<TransactionOutput>(200, 67891));
    transactionOutputs.push_back(std::make_unique<TransactionOutput>(300, 23456));


    std::unique_ptr<Transaction> transaction = std::make_unique<Transaction>(transactionInputs, transactionOutputs);

    std::vector<std::unique_ptr<Transaction>> transactions;
    transactions.push_back(transaction);

    return std::make_unique<Block>(std::move(blockHeader), transactions);
}
