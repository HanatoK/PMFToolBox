/*
  PMFToolBox: A toolbox to analyze and post-process the output of
  potential of mean force calculations.
  Copyright (C) 2020  Haochuan Chen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "test/test.h"

void testGraph() {
  std::vector<Graph::Edge> edges{
      {0, 1, 2}, {0, 2, 4}, {1, 4, 4}, {1, 5, 6},
      {2, 3, 2}, {3, 4, 3}, {3, 5, 1}, {4, 5, 3},
  };
  Graph graph_test(6, false);
  graph_test.setEdges(edges);
  graph_test.printGraph(std::cout);
  std::cout << "After sorting lists:\n";
  graph_test.sortByWeight();
  graph_test.printGraph(std::cout);
  graph_test.DFS(
      2, [](const Graph::Node &node) { std::cout << node.mIndex << " "; });
  std::cout << '\n';
}

void testDijkstra() {
  std::vector<Graph::Edge> edges3{
      {0, 1, 4},   {0, 3, 4},   {1, 0, 1},   {1, 2, 1},  {1, 4, 10}, {2, 1, 4},
      {2, 5, 3},   {3, 0, 1},   {3, 4, 10},  {3, 6, 1},  {4, 3, 4},  {4, 1, 4},
      {4, 5, 3},   {4, 7, 10},  {5, 4, 10},  {5, 2, 1},  {5, 8, 1},  {6, 3, 4},
      {6, 7, 10},  {6, 9, 2},   {7, 6, 1},   {7, 4, 10}, {7, 8, 1},  {7, 10, 1},
      {8, 7, 10},  {8, 5, 3},   {8, 11, 1},  {9, 6, 1},  {9, 10, 1}, {10, 9, 2},
      {10, 7, 10}, {10, 11, 1}, {11, 10, 1}, {11, 8, 1},
  };
  Graph graph(12, true);
  graph.setEdges(edges3);
  graph.printGraph(std::cout);
  std::cout << "After sorting lists:\n";
  graph.sortByWeight();
  graph.printGraph(std::cout);
  graph.Dijkstra(0, 9, Graph::FindPathMode::MFEPMode).dump();
}

void testSPFA() {
  std::vector<Graph::Edge> edges3{
      {0, 1, 4},   {0, 3, 4},   {1, 0, 1},   {1, 2, 1},  {1, 4, 10}, {2, 1, 4},
      {2, 5, 3},   {3, 0, 1},   {3, 4, 10},  {3, 6, 1},  {4, 3, 4},  {4, 1, 4},
      {4, 5, 3},   {4, 7, 10},  {5, 4, 10},  {5, 2, 1},  {5, 8, 1},  {6, 3, 4},
      {6, 7, 10},  {6, 9, 2},   {7, 6, 1},   {7, 4, 10}, {7, 8, 1},  {7, 10, 1},
      {8, 7, 10},  {8, 5, 3},   {8, 11, 1},  {9, 6, 1},  {9, 10, 1}, {10, 9, 2},
      {10, 7, 10}, {10, 11, 1}, {11, 10, 1}, {11, 8, 1},
  };
  Graph graph(12, true);
  graph.setEdges(edges3);
  graph.printGraph(std::cout);
  std::cout << "After sorting lists:\n";
  graph.sortByWeight();
  graph.printGraph(std::cout);
  graph.SPFA(0, 9, Graph::FindPathMode::MFEPMode).dump();
}

void testSPFA2() {
  std::vector<Graph::Edge> edges2{
      {0, 1, 4},   {0, 3, 4},   {1, 0, 1},   {1, 2, 1},  {1, 4, 10}, {2, 1, 4},
      {2, 5, 0},   {3, 0, 1},   {3, 4, 10},  {3, 6, 1},  {4, 3, 4},  {4, 1, 4},
      {4, 5, 0},   {4, 7, 10},  {5, 4, 10},  {5, 2, 1},  {5, 8, 1},  {6, 3, 4},
      {6, 7, 10},  {6, 9, 2},   {7, 6, 1},   {7, 4, 10}, {7, 8, 1},  {7, 10, 1},
      {8, 7, 10},  {8, 5, 0},   {8, 11, 1},  {9, 6, 1},  {9, 10, 1}, {10, 9, 2},
      {10, 7, 10}, {10, 11, 1}, {11, 10, 1}, {11, 8, 1},
  };
  Graph graph(12, true);
  graph.setEdges(edges2);
  graph.printGraph(std::cout);
  std::cout << "After sorting lists:\n";
  graph.sortByWeight();
  graph.printGraph(std::cout);
  graph.SPFA(0, 9, Graph::FindPathMode::MFEPMode).dump();
}

void testDivergence(const QString& input_filename, const QString& output_filename) {
  qDebug() << "========== Start testDivergence ==========";
  qDebug() << "Start reading file:" << input_filename;
  HistogramGradient hist_grad;
  hist_grad.readFromFile(input_filename);
  qDebug() << "Start calculating and writing divergence to" << output_filename;
  hist_grad.divergence().writeToFile(output_filename);
  qDebug() << "========== End of testDivergence ==========";
}
