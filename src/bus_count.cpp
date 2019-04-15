// STD CPP includes
#include <algorithm>
#include <chrono> 
#include <fstream>
#include <iostream>

// STD C includes
#include <cstdio>

// External library includes
#include <opencv2/dnn.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/video/tracking.hpp>

#include <boost/filesystem.hpp>

// Local includes
#include <detection.hpp>
#include <dirent.h>

using namespace std;
using namespace cv;

class Line {
public:
  Point a;
  Point b;
  
  Point na; // normals
  Point nb;
  
  Line(Point a, Point b): a(a), b(b) {
    
    // calculate the normal
    int dx = b.x - a.x;
    int dy = b.y - a.y;
    
    // div 20 makes the drawn length look about right
    int ndx = dy / 20; 
    int ndy = -dx / 20;
    
    Point center = na = (a+b)/2;
    
    // define the normal to be on the positive side
    nb = Point(center.x+ndx, center.y+ndy);
    if (!side(nb))
      nb = Point(center.x-ndx, center.y-ndy);
  }
  
  void draw(Mat &img) {
    cv::line(img, a, b, Scalar(180, 180, 180));
    cv::line(img, na, nb, Scalar(0, 0, 255));
  }

  bool side(Point &p) {
    return ((p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x)) < 0;
  }

};

class World {
public:

  Line inside;
  Line outside;
  
  Line inner_bounds_a;
  Line inner_bounds_b;
  
  int count_out = 0;
  int count_in = 0;
  
  World(Line inside, Line outside, Line inner_bounds_a, Line inner_bounds_b):
    inside(inside), outside(outside),
    inner_bounds_a(inner_bounds_a), inner_bounds_b(inner_bounds_b) {}
    
  void draw(Mat &img) {
    inside.draw(img);
    outside.draw(img);
    
    inner_bounds_a.draw(img);
    inner_bounds_b.draw(img);
    
    string txt = "IN: " + std::to_string(count_in) + "; OUT:" + std::to_string(count_out);
     cv::putText(img, txt, Point(5, 280), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 2);
  }
  
  bool is_inside(Point &p) {
    // cout << inner_bounds_a.side(p) << " " << inner_bounds_b.side(p) << endl;
    return inner_bounds_a.side(p) && inner_bounds_b.side(p);
  }
};

/** Caclulates the intersection over union of two boxes */
float IoU(Rect2d a, Rect2d b) {
  int i_x1 = max(a.x, b.x);
  int i_y1 = max(a.y, b.y);
  int i_x2 = min(a.x+a.width,  b.x+b.width);
  int i_y2 = min(a.y+a.height, b.y+b.height);
  
  if ((i_x2 < i_x1) || (i_y2 < i_y1)) {
    return 0;
  }
  
  Rect2d intersect = Rect2d(
      Point2d(i_x1, i_y1),
      Point2d(i_x2, i_y2)
  );
  
  float i_area = intersect.area();
  float u_area = a.area() + b.area() - i_area;
  
  return i_area / u_area;
}


/* A jumble of mismatched data that follows a detection result around */
class Track {
public:
  Rect2d box;
  float confidence;
  int index;

private:
  bool been_inside = false;
  bool been_outside = false;
  bool counted_in = false;
  bool counted_out = false;

public:  
  Track( Rect2d b, float c, int i): 
      box(b),
      confidence(c),
      index(i) {}
  
  bool update(World &world, ofstream &results, int frame_count) {
    // look only at the boxes center point
    int x = box.x + box.width/2;
    int y = box.y + box.height/4;
    Point p = Point(x, y);
    
    if (world.is_inside(p)) {  
      // Count the people based on which direction they pass
      if (world.inside.side(p)) {
        been_inside = true;
        if (been_outside && !counted_in && !counted_out) {
          world.count_in++;
          counted_in = true;
          cout << "IN: " + to_string(index) << endl;
          results << frame_count << ", IN" << endl;
        }
        
        if (been_outside && counted_out) {
          world.count_out--;
          counted_out = false;
          been_outside = false;
          cout << "BACK IN: " + to_string(index) << endl;
          results << frame_count << ", BACK IN" << endl;
        }
      }      
      
      if (world.outside.side(p)) {
        been_outside = true;
        if (been_inside && !counted_out && !counted_in) {
          world.count_out++;
          counted_out = true;
          cout << "OUT: " + to_string(index) << endl;
          results << frame_count << ", OUT" << endl;
        }
        
        if (been_inside && counted_in) {
          world.count_in--;
          counted_in = false;
          been_inside = false;
          cout << "BACK OUT: " + to_string(index) << endl;
          results << frame_count << ", BACK OUT" << endl;
        }
      }
      
      /*      
      if (world.outside.side(p)) {
        been_outside = true;
        if (been_inside && !counted_out) {
          world.count_out++;
          counted_out = true;
          cout << "OUT: " + to_string(index) << endl;
          results << frame_count << ", OUT" << endl;
        }
      } else if (counted_out) {
        counted_out = false;
        been_outside = false;
        world.count_out--;
        cout << "BACK IN: " + to_string(index) << endl; 
        results << frame_count << ", BACK IN" << endl;
      }*/
    }
    
    // decrease the confidence (will increase when merging with a detection)
    confidence -= 0.05;
    return confidence > 0.2;
  }
  
  void draw(Mat &img) {
    Scalar clr = Scalar(0, confidence*255, 0);
    cv::rectangle(img, box, clr, 1);
    
    int x = box.x + box.width/2;
    int y = box.y + box.height/4;
    Point p = Point(x, y);
    cv::circle(img, p, 3, clr, 2);
    
    string txt = std::to_string(index);
    Point txt_loc = Point(box.x+5, box.y+15);
    cv::putText(img, txt, txt_loc,  FONT_HERSHEY_SIMPLEX, 0.5, clr, 2);
  }
  
};


class Tracks {
public:
  vector<Track*> tracks;
  int index_count = 0;

  Tracks() {}
  
  void add_detections(vector<Detection> &detections) 
  {
    for (Detection detection : detections) {
      cout << "Looking for merges" << endl;
      int replace_index = -1;
      float best_merginess = 0;
      Track* merge_track;
      
      for (int i = 0; i < tracks.size(); i++) {
        Track* track = tracks[i];
        float iou = IoU(track->box, detection.box);
        float merginess = iou;//* track->confidence;
        cout << " " + std::to_string(track->index) + ": " + std::to_string(merginess) << endl;
        if ((merginess > 0.4) && (merginess > best_merginess)) {
          best_merginess = merginess;
          replace_index = i;
          merge_track = track;
        }
      }
      
      // If the track confidence is higher, then just keep using the track's box
      if ((replace_index >= 0) and (merge_track->confidence > detection.confidence)) {
        continue;
      }
      
      Rect2d box = detection.box;
      if (replace_index >= 0) {
        merge_track->box = detection.box;
        merge_track->confidence = detection.confidence;
      } else {
        cout << "Making a new box" << endl;
        Track* t = new Track(box, detection.confidence, index_count++);
        tracks.push_back(t);
      }
    }
  }
  
  void update(World &world, ofstream &results, int frame_count) {
    vector<Track*> new_tracks;
    for (Track* t : tracks) {
      if (t->update(world, results, frame_count))
        new_tracks.push_back(t);
      else
        delete t;
    }
    tracks = new_tracks;
  }
  
  void draw(Mat img) {
    for (Track* track : tracks) {
      track->draw(img);
    }
  }
  
};

int run(Net net, World &world, Tracks &tracks, string dir, string file, bool flip) {
  cout << "Loading video stream" << endl;
  VideoCapture stream(dir + file);
  stream.set(cv::CAP_PROP_POS_FRAMES, 0);
  
  ofstream results;
  results.open(dir + file + ".csv");
  results << "frame, event" << endl;
  
  //VideoWriter video(file + ".avi",VideoWriter::fourcc('M','J','P','G'), 25, Size(300,300));
  
  int frame_size = 300;
  bool run_continuous = true;
  
  // start the detections
  Mat next_frame;
  Mat frame;
  stream.read(next_frame);
  cv::resize(next_frame, next_frame, Size(frame_size, frame_size));
  if (flip)
    cv::flip(next_frame, next_frame, 0);
  swap_image(next_frame, net);
  frame = next_frame;
  
  int frame_count = 0;
  auto start = std::chrono::system_clock::now();
  
  while (stream.read(next_frame)) {
    // video is at 25fps, but our program runs at 9 fps. Therefore, skip
    // a couple frames so it is realtime.
    if(!stream.read(next_frame))
      break;
    //frame_count++;
    if(!stream.read(next_frame))
      break;
    //frame_count++;
    
    cv::resize(next_frame, next_frame, Size(frame_size, frame_size));
    if (flip)
      cv::flip(next_frame, next_frame, 0);
    
    // Get some detections (results will be for frame, not next_frame)
    cout << "Detecting Stuff" << endl;
    vector<Detection> detections = swap_image(next_frame, net);
    
    tracks.add_detections(detections);
    for (Detection d : detections) {      
      cv::rectangle(frame, d.box, Scalar(0, 0, d.confidence*255), 3);
    }
    
    cout << "Updating" << endl;
    tracks.update(world, results, frame_count);
    
    cout << "Drawing results" << endl;
    world.draw(frame);
    tracks.draw(frame);
        
    cv::imshow("output", frame);
    //video.write(frame);
    int key = cv::waitKey(run_continuous);
    if (key == 'q')
      return 1; // complete quit
    if (key == 'w')
      break; // next video
          
    // Rotate the frame buffer
    frame = next_frame;
    frame_count++;
    auto end_2 = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = end_2-start;
    std::cout << "Frames: " << frame_count << " - FPS: " << (frame_count/diff.count()) << " s\n";
    
    if (frame_count == -1)
      break;
  }
  
  results.close();
  stream.release();
  
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> diff = end-start;
  std::cout << "Frames: " << frame_count << " - FPS: " << (frame_count/diff.count()) << " s\n";
  return 0;
}

bool ends_with(string value, string ending) {
  if (ending.size() > value.size())
    return false;
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

bool begins_with(string value, string prefix) {
  return value.rfind(prefix, 0) == 0;
}

int process_folder(Net &net, string folder) {
  // Read config info
  string config = folder + "/config.csv";
  //char* thePath = realpath(config.c_str(), NULL);
  cout << " Reading world parameters: " << config << endl;
  fstream config_file(config);
  if (!config_file.is_open()) {
    cout << "Cannot find config file. Skipping dir";
    return 0;
  }
  int iax, iay, ibx, iby; // input line
  int oax, oay, obx, oby; // output line
  int b1ax, b1ay, b1bx, b1by; // bounds 1 line
  int b2ax, b2ay, b2bx, b2by; // bounds 2 line
  config_file >> iax >> iay >> ibx >> iby;
  config_file >> oax >> oay >> obx >> oby;
  config_file >> b1ax >> b1ay >> b1bx >> b1by;
  config_file >> b2ax >> b2ay >> b2bx >> b2by;
  config_file.close();

  // setup the world and persistent data
  int frame_size = 300;
  World world = World(
    // inside, outside:
    Line(Point(iax, iay), Point(ibx, iby)),
    Line(Point(oax, oay), Point(obx, oby)),

    // inner bounds:
    Line(Point(b1ax, b1ay), Point(b1bx, b1by)),
    Line(Point(b2ax, b2ay), Point(b2bx, b2by))
  );
  /*World world = World(
      // inside, outside:
      Line(Point(0, frame_size/2), Point(frame_size, frame_size/2)),
      Line(Point(frame_size, frame_size/2-20), Point(0, frame_size/2-20)),

      // inner bounds:
      Line(Point(frame_size/6, frame_size), Point(frame_size/5, 0)),
      Line(Point(frame_size*2/3, 0), Point(frame_size*2/3, frame_size))
  );*/
  Tracks tracks = Tracks();

  bool flip = (folder.find("flip") != string::npos);
  if (flip) {
    cout << "Flipping" << endl;
  } else {
    cout << "Not flipping" << endl;
  }

  // read out all filenames
  const char* file_type = "mp4";
  vector<string> filenames;
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(folder.c_str())) == NULL) {
    cout << "Cannot open '" << folder << "' directory" << endl;
    return 0;
  }
  while ((ent = readdir(dir)) != NULL) {
    const char* filename = ent->d_name;
    if (!ends_with(filename, file_type) || begins_with(filename, ".")) {
      cout << " Skipping " << filename << endl;
      continue;
    }
    filenames.push_back(filename);
  }
  
  // sort filenames in name order so that video rolls over nicely
  std::sort(filenames.begin(), filenames.end()); 
  
  // process the video
  for (string filename : filenames) {
    cout << endl << "##   PROCESSING " << folder << "/" << filename << "  ##" << endl << endl;
    
    bool quit = run(net, world, tracks, folder+"/", filename, flip);
    if (quit) {
      cout << endl << "REQUESTED QUIT" << endl;
      return 1;
    }
  }
  return 0;
}


int main() {  
  cout << "Loading Model" << endl;
  Net net = create_net();

  string data_folder = "video/";
  DIR *data_dir;
  struct dirent *d_ent;
  if ((data_dir = opendir(data_folder.c_str())) == NULL) {
    cout << "Cannot open data directory" << endl;
    return 0;
  }
  while ((d_ent = readdir(data_dir)) != NULL) {
    
    string video_folder_name(d_ent->d_name);
    
    if (!begins_with(video_folder_name, "pi")) {
      cout << " Skipping directory " + video_folder_name << endl;
      continue;
    }
    
    cout << "### ENTERING " << video_folder_name << " ###" << endl;
    
    bool quit = process_folder(net, data_folder + video_folder_name);
    if (quit) {
      cout << "PROGRAM QUITTING" << endl; 
      return 0;
    }
  }
  
  cout << " PROGRAM FINISHED SUCCESSFULLY" << endl; 
  return 0;
  
}

