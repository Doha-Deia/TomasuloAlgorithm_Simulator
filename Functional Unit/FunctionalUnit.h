#ifndef FUNCTIONALUNIT_H
#define FUNCTIONALUNIT_H
#include "ReservationStation.h"
#include <vector>
#include <string>
using namespace std;

class FunctionalUnit {

    public:
        FunctionalUnit(const string& type, int cycles, int numRS);
        
        string getType();
        int getNumCycles();
        vector <ReservationStation>& getReservationStations();        
        bool freeRS(); //check if the reservation station is free
        int freeRSIndex(); //simulate execution of instruction
        bool executeCycle(int *Memory, int &pc, bool & takenBranch, bool &target);
        bool ReadyResult(); //results ready to write back
        vector <pair <int, int>> getReadyResults(); //get ready results
        void clearRS(int index); //clear reservation stations
        void clearAll(); //clear all reservation stations
    private:
        string type;        // Type of functional unit (e.g., ALU, Multiplier)
        int numCycles;        // Number of cycles needed
        vector <ReservationStation> numReservationStation;     // Number of reservation stations available 
        

};


#endif //FUNCTIONALUNIT_H