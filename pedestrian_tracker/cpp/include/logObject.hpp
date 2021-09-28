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
    std::string segment;        ///< used to identify each log value

    public:
    std::string frameNumber;    ///< the frame number
    std::string dateTime;       ///< time of the of the logging
    std::string peopleInFrame;  ///< amount of people in the frame
    std::string location;       ///< location of the system
    int x_Location;             ///< the x chord top left corner of the user identified
    int y_Location;             ///< the y chord top left corner of the user identified
    int x_box;                  ///< the width of the box drawn around the person for the given frame
    int y_box;                  ///< the height of the box drawn around the person for the given frame
    int confidence;             ///< the confidence interval of the identification for the given frame
    int uniqueID;               ///< pedestrain unique id
    //Split Log into relivant fields
    LogInformation(std::stringstream &log);


};

#endif