#ifndef REGISTERSTATUS_H
#define REGISTERSTATUS_H
#include <vector>
using namespace std;

class RegisterStatus {
    
    static const int REG_COUNT = 8;

    public:
        int robTag[REG_COUNT];

        RegisterStatus();
        RegisterStatus( vector <int>tag);
        

        void setTag(int regIndex, int robEntry);
        int getTag(int regIndex);
        bool isBusy();
        void clearTag(int regIndex);
        void clearAllTags();
};


#endif //REGISTERSTATUS_H