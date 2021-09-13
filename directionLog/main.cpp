#include <bits/stdc++.h>
#include <string>       // std::string
#include <sstream>      // std::stringstream
#include <iostream>
#include <fstream>
#include <cmath>


using namespace std;

class LogInformation
{
    private:
    string segment;

    public:
    string frameNumber,dateTime, location;
    string entireLog;
    int x_Location, y_Location,x_box,y_box,confidence, unqiueID;
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
                unqiueID = stoi(segment);
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
               
        }
    }


};

void listVector(vector<LogInformation> &logList){

    for(std::vector<LogInformation>::iterator it = logList.begin(); it != logList.end(); ++it) {
    cout << it->location << endl;
 }

}


std::map<int,string> locateDirection(vector<LogInformation> &logList){
    //if Y is going down the nthe user is walking up
    //if X is going lower they are going towards the left
    //This log will log the user ID and the loction left or right that they are moving
    std::map<int, int> yMap;
    std::map<int, int> xMap;
    std::map<int, string> direction;

    for(std::vector<LogInformation>::iterator it = logList.begin(); it != logList.end(); ++it) {

        //If the user is not in the HashMap at the X,Y Chords with the Unique ID as the key
        //Users starting location
        if (yMap.count(it->unqiueID)==0){
            yMap[it->unqiueID] = it-> y_Location;
            xMap[it->unqiueID] = it -> x_Location;
        }
        else{
            //larger of the two numbers will be the direction that the user is moving
            int maxXValue = abs(xMap[it->unqiueID] - it->x_Location);
            int maxYValue = abs(yMap[it->unqiueID] - it->y_Location);
            
            if(maxXValue > maxYValue){
                if(xMap[it->unqiueID] < it->x_Location)
                    direction[it->unqiueID] = "LEFT";
                    
                else
                    direction[it->unqiueID] = "RIGHT";
            }
            else{
                if(yMap[it->unqiueID] > it->y_Location)
                    direction[it->unqiueID] = "FORWARD";
                else
                    direction[it->unqiueID] = "BACKWARD";
            }

        }
    }
    return direction;
    
}


int main (int argc, char **argv){

    if( argv[1] == nullptr){
        cout << "Please Specify Log Name:  ./program 'fileName'" <<endl;
        return -1;
    }

    //Define relivant private parameters
    cout << "FileName is: " << argv[1] << endl;
    std::ifstream logFile(argv[1]);
    std::vector<LogInformation> logList;

    std::string line;

    while (std::getline(logFile, line))
    {
        std::stringstream iss(line);
        LogInformation log(iss);
        logList.push_back(log);
        //std::cout << iss.str() << std::endl;
        // process pair (a,b)
    }

    std::map<int, string> directionList = locateDirection(logList);

    for (const auto& p : directionList ) {
        std::cout  <<p.first << "," << p.second<< "," << logList[0].location << std::endl; 
   
    }

    return 0;
    
};

     
        
    

