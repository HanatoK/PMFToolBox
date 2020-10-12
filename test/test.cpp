#include "test/test.h"

void testGraph() {
  QVector<Graph::Edge> edges{{0,1,2},
                             {0,2,4},
                             {1,4,4},
                             {1,5,6},
                             {2,3,2},
                             {3,4,3},
                             {3,5,1},
                             {4,5,3},
  };
  Graph graph_test(6, false);
  graph_test.setEdges(edges);
  graph_test.printGraph(std::cout);
  std::cout << "After sorting lists:\n";
  graph_test.sortByWeight();
  graph_test.printGraph(std::cout);
  graph_test.DFS(2, [](const Graph::Node& node){std::cout << node.mIndex << " ";});
  std::cout << '\n';
}
