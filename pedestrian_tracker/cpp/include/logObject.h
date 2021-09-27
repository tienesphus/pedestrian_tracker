#ifndef LOGOBJECT_H
#define LOGOBJECT_H

#include <bits/stdc++.h>
#include <string>       // std::string
#include <sstream>      // std::stringstream
#include <iostream>
#include <fstream>

class LogInformation
{
    //Sample Log
    // 3155, (Frame Number)
    //Mon Aug 23 13:37:27 2021, (Date)
    //0, (People In the Frame 0 indicates 1)
    //432, (x location)
    //70, (y location)
    //31, (box X)
    //70, (box Y)
    //0.931894, (Confidence)
    //New Demo Location 3 (Location)

    private:
    std::string segment;

    public:
    std::string frameNumber,dateTime, peopleInFrame,location;
    int x_Location, y_Location, x_box, y_box, confidence, uniqueID;
    //Split Log into relivant fields
     LogInformation(std::stringstream &log);


};

#endif