#include <bits/stdc++.h>
#include <string>       // std::string
#include <sstream>      // std::stringstream
#include <iostream>
#include <fstream>


#include "logObject.h"

    //Split Log into relivant fields
   LogInformation::LogInformation(std::stringstream &log){
        int i=0;
   
        //int x_Location, y_Location, x_box, y_box, confidence;
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
                uniqueID = stoi(segment);
                i++;
                break;
            case 3:
                x_Location = stoi(segment);
                i++;
                break;
            case 4:
                y_Location = stoi(segment);
                i++;
                break;
            case 5:
                x_box = stoi(segment);
                i++;
                break;
            case 6:
                y_box = stoi(segment);
                i++;
                break;
            case 7:
                confidence = stoi(segment);
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
    };

