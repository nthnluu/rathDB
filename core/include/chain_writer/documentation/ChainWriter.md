# What is a ChainWriter?

A ChainWriter is an interface to the blocks that are stored
on disk. It gives the ability to write blocks and undo blocks
to disk as well as read blocks and undo blocks from disk. When
a block and its corresponding undo block are written to disk, a
BlockRecord will be produced. This BlockRecord will be used to keep
track of where the block and undo block are stored on disk. When reading
a block or undo block from disk, a subset of the BlockRecord information
is needed. This subset is known as FileInfo. This is because a BlockRecord
stores information about a block and an undo block. A FileInfo stores
information about one or the other.

# What is the purpose of the ChainWriter?

Blocks are rarely accessed by users and they take up
a lot of space. As a result, it is a better strategy to reserve RAM
for other things and store these blocks on disk. 