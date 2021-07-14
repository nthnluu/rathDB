#include <chain.h>
#include <rathcrypto.h>

// TODO: Implement
std::unique_ptr<UndoBlock> make_undo_block(Block& block) {

    // Get transaction hashes from block
    std::vector<uint32_t> transaction_hashes;
    std::vector<std::unique_ptr<UndoCoinRecord>> undo_coin_records;
    for (auto& transaction : block.transactions) {
        transaction_hashes.push_back(RathCrypto::hash(Transaction::serialize(*transaction)));
        undo_coin_records.push_back(std::make_unique<UndoCoinRecord>(transaction->version, ))
    }

    // Create undo coin records

//    UndoCoinRecord(uint8_t version_,
//            std::vector<uint32_t> utxo_, std::vector<uint32_t> amounts_,
//            std::vector<uint32_t> public_keys_);



    return std::make_unique<UndoBlock>(transaction_hashes, )
}

Chain::Chain() {
    _active_chain_length = 1;
    _active_chain_last_block = construct_genesis_block();
    _block_info_database = std::make_unique<BlockInfoDatabase>();
    _coin_database = std::make_unique<CoinDatabase>();
    _chain_writer = std::make_unique<ChainWriter>();

    std::unique_ptr<UndoBlock> genesis_undo_block = make_undo_block(*_active_chain_last_block);

    // Write undo block to disk
    std::unique_ptr<FileInfo> undoBlockFileInfo = _chain_writer->write_undo_block(
            UndoBlock::serialize(*genesis_undo_block));

    // Write the genesis block to disk
    std::unique_ptr<FileInfo> genesisBlockFileInfo = _chain_writer->write_block(
            Block::serialize(*_active_chain_last_block));

    // Construct block record
    BlockRecord blockRecord = new BlockRecord(*_active_chain_last_block->block_header, NULL, NULL, genesisBlockFileInfo,
                                              undoBlockFileInfo);

    // Store the genesis block in the coin_database
    _block_info_database->store_block_record(RathCrypto::hash(Block::serialize(*_active_chain_last_block)),
                                             blockRecord);
}

// TODO: Implement
void Chain::handle_block(std::unique_ptr<Block> block) {
    // Check if the block is being appended onto the main chain
    const bool isAppendedToMainChain =
            RathCrypto::hash(Block::serialize(*block)) == block->block_header->previous_block_hash;

    if (isAppendedToMainChain) {
        // update UTXO and mempool
        _coin_database->validate_and_store_block(block->transactions);
    }

    if (_coin_database->validate_block(block->transactions)) {
        _chain_writer->store_block();

    }
}

void Chain::handle_transaction(std::unique_ptr<Transaction> transaction) {
    _coin_database->validate_and_store_transaction(std::move(transaction));
}

uint32_t Chain::get_chain_length(uint32_t block_hash) {
    if (_block_info_database->get_block_record(block_hash)) {
        return _block_info_database->get_block_record(block_hash)->height;
    } else {
        return 0;
    }
}

std::unique_ptr<Block> Chain::get_block(uint32_t block_hash) {
    // query block_info_database for corresponding block record
    std::unique_ptr<BlockRecord> blockRecord = _block_info_database->get_block_record(block_hash);

    if (blockRecord) {
        FileInfo fileInfo = FileInfo(blockRecord->block_file_stored, blockRecord->block_offset_start,
                                     blockRecord->block_offset_end);
        std::basic_string block = _chain_writer->read_block(fileInfo);
        return Block::deserialize(block);
    } else {
        return nullptr;
    }
}

// TODO: Implement
std::vector<std::unique_ptr<Block>> Chain::get_active_chain(uint32_t start, uint32_t end) {}

// TODO: Implement
std::vector<uint32_t> Chain::get_active_chain_hashes(uint32_t start, uint32_t end) {}

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
std::vector<std::unique_ptr<UndoBlock>> Chain::get_undo_blocks_queue(uint32_t branching_height) {}

// TODO: Implement
std::vector<std::shared_ptr<Block>> Chain::get_forked_blocks_stack(uint32_t starting_hash) {}

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
