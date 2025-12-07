//
// Created by ASUS Zenbook on 12/3/2025.
//

#include "ROB.h"
#include <stdexcept>

ROB::ROB() : head(0), tail(0), count(0)
{
    for (int i = 0; i < 8; i++)
    {
        // entries[i].type = "";
        entries[i].dest = -1;
        entries[i].value = 0;
        entries[i].ready = false;
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

int ROB::addEntry(int dest)
{
    if (isFull())
    {
        throw std::runtime_error("ROB is full");
    }
    // entries[tail].type = type;
    entries[tail].dest = dest;
    entries[tail].value = 0;
    entries[tail].ready = false;

    int old_tail = tail;

    tail = (tail + 1) % 8;
    count++;
    return old_tail;
}

void ROB::markReady(int robNum, int value)
{
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
        // entries[head].type = "";
        entries[head].dest = -1;
        entries[head].value = 0;
        entries[head].ready = false;

        head = (head + 1) % 8;
        count--;
    }
}

const ROBEntry *ROB::getEntries() const
{
    return entries;
}
