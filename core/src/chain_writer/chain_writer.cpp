#include <chain_writer.h>
#include <stdio.h>

const std::string ChainWriter::_file_extension = "data";
const std::string ChainWriter::_block_filename = "blocks";
const uint16_t ChainWriter::_max_block_file_size = 1000;
const std::string ChainWriter::_undo_filename = "undo_blocks";
const uint16_t ChainWriter::_max_undo_file_size = 1000;
const std::string ChainWriter::_data_directory = "data";
//const std::string ChainWriter::_data_directory = "/Users/nathanluu/CLionProjects/rathDB_stencil/data";


ChainWriter::ChainWriter() : _current_block_file_number(0), _current_block_offset(0), _current_undo_file_number(0),
                             _current_undo_offset(0) {}

// TODO: Test implementation
std::unique_ptr<BlockRecord> ChainWriter::store_block(const Block &block, uint32_t height, const UndoBlock &undoBlock) {
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> serialized_block = Block::serialize(block);
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> serialized_undo_block = UndoBlock::serialize(
            undoBlock);

    std::unique_ptr<FileInfo> block_file_info = write_block(serialized_block);
    std::unique_ptr<FileInfo> undo_block_file_info = write_undo_block(serialized_undo_block);

    std::unique_ptr<BlockHeader> block_header = std::make_unique<BlockHeader>(block.block_header->version,
                                                                              block.block_header->previous_block_hash,
                                                                              block.block_header->merkle_root,
                                                                              block.block_header->difficulty_target,
                                                                              block.block_header->nonce,
                                                                              block.block_header->timestamp);

    return std::make_unique<BlockRecord>(std::move(block_header), block.transactions.size(), height, *block_file_info,
                                         *undo_block_file_info);
}

// TODO: Test implementation
std::unique_ptr<FileInfo> ChainWriter::write_block(const std::string &serialized_block) {
    FILE* current_file;

    // Check if the block will fit in the current file
    // Move onto next file if not...
    if (_current_block_offset + (serialized_block.length() + 1) >= _max_block_file_size) {
        _current_block_file_number = _current_block_file_number + 1;
        _current_block_offset = 0;
    }

    std::string current_file_path =
            _data_directory + "/" + _block_filename + "_" + std::to_string(_current_block_file_number) + "." +
            _file_extension;

    // open the file in append mode
    current_file = fopen(current_file_path.c_str(), "a");
    // write the block to the file
    fprintf(current_file, "%s", serialized_block.c_str());

    // update current offsets accordingly
    uint16_t block_length = serialized_block.length();
    std::cerr << block_length;
    _current_block_offset = _current_block_offset + block_length + 1;



    // Close the file
    fclose(current_file);

    return std::make_unique<FileInfo>(_block_filename + "_" + std::to_string(_current_block_file_number) + "." +
                                      _file_extension, _current_block_offset - serialized_block.length(),
                                      _current_block_offset);
}

// TODO: Test implementation
std::unique_ptr<FileInfo> ChainWriter::write_undo_block(const std::string &serialized_block) {
    FILE *current_file;

    // Check if the undo block will fit in the current file
    // Move onto next file if not...
    if (_current_undo_offset + serialized_block.length() >= _max_undo_file_size) {
        _current_undo_file_number = _current_undo_file_number + 1;
        _current_undo_offset = 0;
    }

    std::string current_file_path =
            _data_directory + "/" + _undo_filename + "_" + std::to_string(_current_undo_file_number) + "." +
            _file_extension;

    // open the file
    current_file = fopen(current_file_path.c_str(), "a");

    // write the undo block to the file
    fprintf(current_file, "%s", serialized_block.c_str());

    // update current offsets accordingly
    _current_undo_offset = _current_undo_offset + serialized_block.length();

    // Close the file
    fclose(current_file);



    return std::make_unique<FileInfo>(_undo_filename + "_" + std::to_string(_current_undo_file_number) + "." +
                                      _file_extension, _current_undo_offset - serialized_block.length(),
                                      _current_undo_offset);
}

// TODO: Test implementation
std::string ChainWriter::read_block(const FileInfo &block_location) {
    int num_elements = block_location.end - block_location.start;

    // open the file
    FILE *current_file = fopen(block_location.file_name.c_str(), "r");

    // create buffer to store read values
    char buffer[num_elements];

    // adjust offset in current file
    fseek(current_file, block_location.start, SEEK_SET);

    // read data from file
    fread(buffer, sizeof(char), num_elements, current_file);

    // close the file
    fclose(current_file);

    return std::string(buffer);
}

// TODO: Test implantation
std::string ChainWriter::read_undo_block(const FileInfo &undo_block_location) {
    int num_elements = undo_block_location.end - undo_block_location.start;

    // open the file
    FILE *current_file = fopen(undo_block_location.file_name.c_str(), "r");

    // create buffer to store read values
    char buffer[num_elements];

    // adjust offset in current file
    fseek(current_file, undo_block_location.start, SEEK_SET);

    // read data from file
    fread(buffer, sizeof(char), num_elements, current_file);

    // close the file
    fclose(current_file);

    return std::string(buffer);
}