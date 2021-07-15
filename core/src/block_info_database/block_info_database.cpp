#include <block_info_database.h>

BlockInfoDatabase::BlockInfoDatabase(): _database(std::make_unique<Database>()) {}

// TODO: Test implementation
void BlockInfoDatabase::store_block_record(uint32_t hash, const BlockRecord &record) {
    _database->put_safely(std::to_string(hash), BlockRecord::serialize(record));
}

// TODO: Test implementation
std::tuple<bool, std::unique_ptr<BlockRecord>> BlockInfoDatabase::get_block_record(uint32_t block_hash) {
    const std::string res = _database->get_safely(std::to_string(block_hash));
    return std::make_tuple(!res.empty(), BlockRecord::deserialize(res));
}