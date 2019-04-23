#include "tbb/flow_graph.h"
#include <iostream>
#include <unistd.h>

using namespace std;
using namespace tbb::flow;

int main() {
    
    int count = 0;
        
    graph g;
    
    // produce numbers from 1 to limit
    source_node<int> src( g,
      [&](int &v) -> bool {
          v = count++;
          cout << ("SRC " + to_string(count)) << endl;
          return true;
      }, 
      false);
    
    limiter_node<int> limiter(g, 2);
    
    function_node<int, continue_msg> func( g, serial,
      [](int num) -> int {
          
          cout << ("Sleeping " + to_string(num)) << endl;
          usleep(1000*1000);
          cout << ("Wakeup " + to_string(num)) << endl;
          
          return num;
    });
    
    make_edge(src, limiter);
    make_edge(limiter, func);
    make_edge(func, limiter.decrement);
    src.activate();
    
    g.wait_for_all();
    
    return 0;
}
