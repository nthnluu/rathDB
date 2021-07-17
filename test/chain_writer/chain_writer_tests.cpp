/*
 * This file belongs to Brown University's computer
 * science department as part of csci1951L - Blockchains
 * and cryptocurrencies.
 *
 * This software was developed by Colby Anderson (HTA)
 * and Parker Ljung (TA) of csci1951L in Summer of 2021.
 */
#include "gtest/gtest.h"
#include "chain_writer.h"


TEST(testChainWriter, testWriteFile) {
    ChainWriter chainWriter = ChainWriter();
    std::unique_ptr<FileInfo> file_info = chainWriter.write_block("hello");
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> res = chainWriter.read_block(*file_info);
    EXPECT_EQ(res, "hello");

    std::unique_ptr<FileInfo> file_info1 = chainWriter.write_block("block_sample_2");
    std::basic_string<char, std::char_traits<char>, std::allocator<char>> res1 = chainWriter.read_block(*file_info1);
    EXPECT_EQ(res1, "block_sample_2");


}