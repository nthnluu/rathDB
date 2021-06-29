# Note

The following description will remain agnostic to the actual
implementation. Instead, it will discuss things at an API level.

# What is a BlockRecord?

A BlockRecord contains information about where blocks and
their corresponding undo blocks are stored on disk. It
uses a file name, as well as a starting offset
and ending offset into the file (measured in bytes), to identify
where a particular block or undo block is stored on disk.In
addition to his, a block record will also contain some meta
information about the block. This includes the block header, the
number of transactions that were on the block, as well as the height
of the block in its respective chain. Height should stick to a particular
convention: namely, choosing to start counting from 0 or 1. We recommend
starting from 1 (e.g. the genesis block will have height of 1), since
this is usually how height is interpreted.

# What is the purpose of a BlockRecord?

Serialized BlockRecords (serialized as strings) are values for the 
BlockInfoDatabase. To understand why this might be useful, read more
about the BlockInfoDatabase.

