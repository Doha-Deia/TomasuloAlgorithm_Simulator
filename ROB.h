//
// Created by ASUS Zenbook on 12/3/2025.
//

#ifndef ROB_H
#define ROB_H
#include <string>
#include <queue>
using namespace std;

struct ROBEntry
{
    // string type;
    int dest;
    int value;
    bool ready;
};

class ROB
{
public:
    ROB();

    bool isFull() const;
    bool isEmpty() const;

    int addEntry(int dest);
    void markReady(int robNum, int value);
    ROBEntry *peek();
    void remove();

    const ROBEntry *getEntries() const;
    int head;
    int tail;
    int count;

    ROBEntry entries[8];
};

#endif // ROB_H
