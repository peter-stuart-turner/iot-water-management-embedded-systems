#ifndef RunningAverage_h
#define RunningAverage_h
//
//    FILE: RunningAverage.h
//  AUTHOR: Rob dot Tillaart at gmail dot com
// PURPOSE: RunningAverage library for Arduino
//     URL: http://arduino.cc/playground/Main/RunningAverage
// HISTORY: See RunningAverage.cpp
//
// Released to the public domain
//

// backwards compatibility
// clr() clear()
// add(x) addValue(x)
// avg() getAverage()

#define RUNNINGAVERAGE_LIB_VERSION "0.2.04"

#include "Arduino.h"

class RunningAverage
{
public:
    RunningAverage(void);
    RunningAverage(int);
    ~RunningAverage();

    void clr();
    void addValue(byte);
    void fillValue(byte, int);

    byte getAverage();

    byte getElement(uint8_t idx);
    uint8_t getSize() { return _size; }
    uint8_t getCount() { return _cnt; }
    byte lastAvg;

protected:
    uint8_t _size;
    uint8_t _cnt;
    uint8_t _idx;
    byte   _sum;
    byte * _ar;
};

#endif
// END OF FILE
