
#include "ROB.h"
#include <stdexcept>

ROB::ROB() : head(0), tail(0), count(0)
{
    // Initialize all ROB entries
    for (int i = 0; i < 8; i++)
    {
        entries[i].busy = false;
        entries[i].type = "";
        entries[i].dest = -1;
        entries[i].value = 0;
        entries[i].ready = false;
        entries[i].instrIndex = -1;
    }
}

bool ROB::isFull() const
{
    return count == 8;
}

bool ROB::isEmpty() const
{
    return count == 0;
}

int ROB::addEntry(int dest, int instrIndex)
{
    if (isFull())
    {
        throw std::runtime_error("ROB is full");
    }

    // Initialize the new ROB entry
    entries[tail].busy = true;             // Mark as in use
    entries[tail].type = "REG";            // Default to REG, caller can change if STORE
    entries[tail].dest = dest;             // Destination register or memory address
    entries[tail].value = 0;               // Result not yet computed
    entries[tail].ready = false;           // Execution not yet complete
    entries[tail].instrIndex = instrIndex; // Track which instruction this is

    int old_tail = tail;
    tail = (tail + 1) % 8;
    count++;

    return old_tail;
}

void ROB::markReady(int robNum, int value)
{
    // Find the ROB entry and mark it ready
    for (int i = 0, idx = head; i < count; i++, idx = (idx + 1) % 8)
    {
        if (idx == robNum)
        {
            entries[idx].value = value;
            entries[idx].ready = true;
            return;
        }
    }
}

ROBEntry *ROB::peek()
{
    if (isEmpty())
        return nullptr;
    return &entries[head];
}

void ROB::remove()
{
    if (!isEmpty())
    {
        // Clear the entry at head
        entries[head].busy = false;
        entries[head].type = "";
        entries[head].dest = -1;
        entries[head].value = 0;
        entries[head].ready = false;
        entries[head].instrIndex = -1;

        head = (head + 1) % 8;
        count--;
    }
}

const ROBEntry *ROB::getEntries() const
{
    return entries;
}