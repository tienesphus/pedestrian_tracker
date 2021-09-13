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
    string frameNumber,dateTime, unqiueID, x_Location, y_Location,x_box,y_box,confidence,location;
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
                unqiueID = segment;
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
               
        }
    }


};

void listVector(vector<LogInformation> &logList){

    for(std::vector<LogInformation>::iterator it = logList.begin(); it != logList.end(); ++it) {
    cout << it->location << endl;
 }

}


std::map<std::string,string> locateDirection(vector<LogInformation> &logList){
    //if Y is going down the nthe user is walking up
    //if X is going lower they are going towards the left
    //This log will log the user ID and the loction left or right that they are moving
    std::map<std::string, int> yMap;
    std::map<std::string, int> xMap;
    std::map<std::string, string> direction;

    for(std::vector<LogInformation>::iterator it = logList.begin(); it != logList.end(); ++it) {

        //If the user is not in the HashMap at the X,Y Chords with the Unique ID as the key
        if (yMap.find(it->unqiueID) != yMap.end()){
            yMap[it->unqiueID] = stoi(it-> y_Location);
            xMap[it->unqiueID] = stoi(it -> x_Location);
        }
        else{
            //larger of the two numbers will be the direction that the user is moving
            int maxXValue = abs(stoi(it->x_Location) - xMap[it->unqiueID]);
            int maxYValue = abs(stoi(it->y_Location) - yMap[it->unqiueID]);

            if(maxXValue > maxYValue){
                if(xMap[it->unqiueID] < stoi(it->x_Location))
                    direction[it->unqiueID] = "LEFT";
                else
                    direction[it->unqiueID] = "RIGHT";
            }
            else{
                if(yMap[it->unqiueID] < stoi(it->y_Location))
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

    std::map<std::string, string> directionList = locateDirection(logList);

    for (const auto& p : directionList ) {
        std::cout << "User ID: " <<p.first << " Direction: " << p.second<< std::endl; 
   
    }

    return 0;
    
};

     
        
    

