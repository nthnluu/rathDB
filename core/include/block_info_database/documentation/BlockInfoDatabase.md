# Note

The following description will remain agnostic to the actual
implementation. Instead, it will discuss things at an API level.

# What is a BlockInfoDatabase?

A BlockInfoDatabase is essentially a wrapper around
a key-value storage system. It acts as a middle-man
between storing key value pairs, retrieving values for
keys, and other common key-value store operations.
A key is a string which represents the hash of a block.
A value is a string which represents a serialized 
BlockRecord for the particular block. This allows a user
to query for a particular BlockRecord of a block by 
supplying the hash of the block. Note, the BlockInfoDatabase
will handle the conversion from BlockRecord to serialized 
BlockRecord and the conversion from uint32_t hash to string
hash itself.

# Why is BlockInfoDatabase a wrapper?

It is usually a good practice to have wrappers around databases
for optimization purposes. For instance, if we were to implement
caching, it would be done at this level. If we were to turn single
writes and reads into batches, it would be done at this level. Basically
all query optimizations would be done in this wrapper.

This also gives the flexibility of switching out the backend (which database)
we actually end up using with more ease. For example, this implementation
remains agnostic to whether the database will choose to store this information
in RAM, on disk, or on both.

# What is the purpose of BlockInfoDatabase?

For most cryptocurrencies, nodes participating in the network will
need to store some amount of blocks. These blocks aren't usually needed
to be accessed, and it is also quite a large amount of information. As
a result, they tend to be stored in the much slower, but much larger area
of memory known as disk. BlockRecords can keep track of where these blocks
and their undo blocks (read more about undo blocks in chain_writer) are stored
on disk. The information in BlockInfoDatabase should normally be stored in disk.
There may be a reason to have a small caching layer for batch writes to the database,
for optimization purposes, but other than this, all data should be kept on disk.
This is because users usually don't need to access blocks often. They tend to
write blocks more often than they read blocks (hence why batch writing may be helpful).