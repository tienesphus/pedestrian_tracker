#include <bits/stdc++.h>
#include <string>       // std::string
#include <sstream>      // std::stringstream
#include <iostream>
#include <fstream>


#include "logObject.h"
class LogInformation
{
    private:
    string segment;

    public:
    string frameNumber,dateTime, peopleInFrame, x_Location, y_Location,x_box,y_box,confidence,location;
    string entireLog;
    //Split Log into relivant fields
    LogInformation(stringstream &log){
        int i=0;
        while(getline(log, segment, ','))
            {   
            
            switch (i)
            {
            case 0:
                frameNumber = segment;
                i++;
                break;
            case 1:
                dateTime = segment;
                i++;
                break;
            case 2:
                peopleInFrame = segment;
                i++;
                break;
            case 3:
                x_Location = segment;
                i++;
                break;
            case 4:
                y_Location = segment;
                i++;
                break;
            case 5:
                x_box = segment;
                i++;
                break;
            case 6:
                y_box = segment;
                i++;
                break;
            case 7:
                confidence = segment;
                i++;
                break;
            case 8:
                location = segment;
                i++;
                break;
            
            default:
                break;
            }
               //seglist.push_back(segment);
        }
    }


};