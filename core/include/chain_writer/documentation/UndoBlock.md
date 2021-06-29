# What is an UndoBlock?

An UndoBlock is a block that contains all necessary information
for "undoing" a particular block. When adding to our blockchain,
we may encounter the scenario where a forked chain becomes the
main chain. This means that all UTXO we were keeping track of needs
to be updated. This is because some of the UTXO we were keeping track of
are from blocks that will no longer be on the main chain. Furthermore, some
of the UTXO we spent now needs to be reverted. This is why every block
has a corresponding undo block. Each undo block has the information needed
to revert the state of all UTXO back a block, as if the block was never
added in the first place.

To accomplish all this, it stores a list of transaction hashes that
corresponds to transactions that were on the block. For each transaction,
it stores a corresponding UndoCoinRecord, which is essentially just a
CoinRecord (see UndoCoinRecord or CoinRecord for further description).

# What is the purpose of an UndoBlock?

An UndoBlock is just one way of accounting for forked blocks.