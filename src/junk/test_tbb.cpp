#include "tbb/flow_graph.h"
#include <iostream>
#include <unistd.h>

using namespace std;
using namespace tbb::flow;

/** 
 * Forces the computer to stop and think for a bit
 **/
int wait() {
    /*int a = 1;
    for (int i = 1; i < ((rand()%5)+1)*1000000; i++) {
        a *= (rand() % i) + 1;
    }*/
    usleep(1000*1000);
    return 0;
}

int main() {
    
    while (true)
        wait();
    
    const int limit = 50;
    int count = 0;
        
    graph g;
    
    // produce numbers from 1 to limit
    source_node<int> src( g,
      [&](int &v) -> bool {
          if (count < limit) {
              ++count;
              v = count;
              //cout << ("START MAKE " + to_string(count)) << endl;
              wait();
              //cout << ("END MAKE " + to_string(count)) << endl;
              return true;
          } else {
              return false;
          }
      }, 
      false);
    
    join_node<tuple<int, int>> loop_join(g);
    
    function_node<tuple<int, int>, int> loop_back( g, serial,
      [](tuple<int, int> nums) -> int {
          int a = std::get<0>(nums);
          int b = std::get<1>(nums);
                    
          //cout << ("START loop " + to_string(a) + " " + to_string(b)) << endl;
          wait();
          //cout << ("END loop " + to_string(a) + " " + to_string(b)) << endl;
          return a;
    });
    
    function_node<tuple<int, int>, int> dual_sum( g, unlimited,
      [](tuple<int, int> nums) -> int {
          int a = std::get<0>(nums);
          int b = std::get<1>(nums);
                    
          cout << ("START stored " + to_string(a) + " " + to_string(b)) << endl;
          wait();
          cout << ("END stored " + to_string(a) + " " + to_string(b)) << endl;
          return a + b;
    });
    
    make_edge(src, input_port<0>(loop_join));
    make_edge(loop_join, loop_back);
    make_edge(loop_back, input_port<1>(loop_join));
    make_edge(loop_join, dual_sum);
    
    loop_back.try_put(make_tuple(0, -1));
    src.activate();
    
    g.wait_for_all();
    
    return 0;
}
