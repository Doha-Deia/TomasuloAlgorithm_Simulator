
#ifndef ROB_H
#define ROB_H
#include <string>
using namespace std;

struct ROBEntry
{
    bool busy;      // Whether this ROB entry is in use
    string type;    // "REG" or "STORE"
    int dest;       // Register number or memory address
    int value;      // Computed value
    bool ready;     // Whether execution has completed
    int instrIndex; // Track which instruction this ROB entry belongs to
    int commitCountdown = 1;
};

class ROB
{
public:
    ROB();

    bool isFull() const;
    bool isEmpty() const;

    int addEntry(int dest, int instrIndex); // Updated signature
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
