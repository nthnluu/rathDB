#include <chain_writer.h>
#include <stdio.h>

const std::string ChainWriter::_file_extension = "data";
const std::string ChainWriter::_block_filename = "blocks";
const uint16_t ChainWriter::_max_block_file_size = 1000;
const std::string ChainWriter::_undo_filename = "undo_blocks";
const uint16_t ChainWriter::_max_undo_file_size = 1000;
const std::string ChainWriter::_data_directory = "data";


ChainWriter::ChainWriter() {
    _current_block_file_number = 0;
    _current_block_offset = 0;
    _current_undo_file_number = 0;
    _current_undo_offset = 0;
}

// TODO: Remove undo block from function signature
std::unique_ptr<BlockRecord> ChainWriter::store_block(const Block &block, uint32_t height, const UndoBlock &undoBlock) {
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> serializedBlock = Block::serialize(block);
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> serializedUndoBlock = UndoBlock::serialize(
            undoBlock);

    std::unique_ptr<FileInfo> blockFileInfo = write_block(serializedBlock);
    std::unique_ptr<FileInfo> undoBlockFileInfo = write_undo_block(serializedUndoBlock);

    std::unique_ptr<BlockHeader> blockHeader = std::make_unique<BlockHeader>(block.block_header->version,
                                                                             block.block_header->previous_block_hash,
                                                                             block.block_header->merkle_root,
                                                                             block.block_header->difficulty_target,
                                                                             block.block_header->nonce,
                                                                             block.block_header->timestamp);

    return std::make_unique<BlockRecord>(std::move(blockHeader), block.transactions.size(), height, *blockFileInfo,
                                         *undoBlockFileInfo);
}

std::unique_ptr<FileInfo> ChainWriter::write_block(std::string serialized_block) {
    FILE *currentFile;

    // Check if the block will fit in the current file
    if (_current_block_offset + serialized_block.size() > _max_block_file_size) {
        _current_block_file_number = _current_block_file_number + 1;
        _current_block_offset = 0;
    }

    std::string currentFilePath =
            _data_directory + "/" + _block_filename + "_" + std::to_string(_current_block_file_number) + "." +
            _file_extension;

    // open the file
    currentFile = fopen(currentFilePath.c_str(), "r");

    // write the block to the file
    fprintf(currentFile, "%s", serialized_block.c_str());

    // update current offsets accordingly
    _current_block_offset = _current_block_offset + serialized_block.size() + 1;

    // Close the file
    fclose(currentFile);
}

std::unique_ptr<FileInfo> ChainWriter::write_undo_block(std::string serialized_block) {
    FILE *currentFile;

    // Check if the undo block will fit in the current file
    if (_current_undo_offset + serialized_block.size() > _max_undo_file_size) {
        _current_undo_file_number = _current_undo_file_number + 1;
        _current_undo_offset = 0;
    }

    std::string currentFilePath =
            _data_directory + "/" + _undo_filename + "_" + std::to_string(_current_undo_file_number) + "." +
            _file_extension;

    // open the file
    currentFile = fopen(currentFilePath.c_str(), "r");

    // write the undo block to the file
    fprintf(currentFile, "%s", serialized_block.c_str());

    // update current offsets accordingly
    _current_undo_offset = _current_undo_offset + serialized_block.size() + 1;

    // Close the file
    fclose(currentFile);
}

std::string ChainWriter::read_block(const FileInfo& block_location) {
    int numElements = block_location.end - block_location.start;

    // open the file
    FILE *currentFile = fopen(block_location.file_name.c_str(), "r");

    // create buffer to store read values
    char buffer[numElements];

    // adjust offset in current file
    fseek(currentFile, block_location.start, SEEK_SET);

    // read data from file
    fread(buffer, sizeof(char), numElements, currentFile);

    // close the file
    fclose(currentFile);

    return std::string(buffer);
}