#ifndef GRAPH_H
#define GRAPH_H

#include <QList>
#include <QVector>
#include <algorithm>
#include <compare>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <queue>

class Graph {
public:
  struct Node {
    size_t mIndex;
    double mWeight;
  };
  struct Edge {
    size_t mSource;
    size_t mDestination;
    double mWeight;
  };
  struct DijkstraResults {
    size_t mNumLoops;
    QVector<bool> mVisitedNodes;
    QVector<size_t> mPathNodes;
    QVector<double> mDistances;
    void dump() const;
  };
  enum DijkstraMode {
    SumOfEdges,
    MaximumEdges,
    MFEPMode,
  };
  Graph();
  Graph(const size_t numNodes, bool directed = false);
  bool setEdge(size_t source, size_t destination, double weight = 1.0);
  bool setEdges(const QVector<Edge> &edges);
  void sortByWeight();
  bool getEdge(size_t source, size_t destination, double &weight) const;
  void printGraph(std::ostream &os) const;
  void DFS(size_t start, std::function<void(const Node &)> func) const;
  DijkstraResults Dijkstra(size_t addr_start, size_t addr_end,
                           DijkstraMode mode);
  template <typename DistanceType>
  DijkstraResults
  Dijkstra(size_t addr_start, size_t addr_end, const DistanceType &dist_start,
           const DistanceType &dist_infinity,
           std::function<DistanceType(DistanceType, double)> calc_new_dist);
  double FindMaxSumWeight() const;

protected:
  size_t mNumNodes;
  bool mIsDirected;
  QVector<QList<Node>> mHead;
  bool setEdgeHelper(size_t source, size_t destination, double weight = 1.0);
  void DFSHelper(size_t i, QVector<bool> &visited,
                 std::function<void(const Node &)> func) const;
};

template <typename DistanceType>
Graph::DijkstraResults Graph::Dijkstra(
    size_t addr_start, size_t addr_end, const DistanceType &dist_start,
    const DistanceType &dist_infinity,
    std::function<DistanceType(DistanceType, double)> calc_new_dist) {
  using std::deque;
  using std::tuple;
#ifdef DEBUG_DIJKSTRA
  using std::cout;
  using std::endl;
#endif
  using std::make_pair;
  using std::priority_queue;
  typedef std::pair<DistanceType, size_t> DistNodePair;
  sortByWeight();
  QVector<bool> visited(mNumNodes, false);
  QVector<size_t> previous(mNumNodes);
  priority_queue<DistNodePair, QVector<DistNodePair>,
                 std::greater<DistNodePair>>
      pq;
  QVector<DistanceType> distances(mNumNodes);
  for (size_t i = 0; i < mNumNodes; ++i) {
    distances[i] = (i == addr_start) ? dist_start : dist_infinity;
    previous[i] = mNumNodes;
  }
  pq.push(make_pair(dist_start, addr_start));
  size_t loop = 0;
  while (!pq.empty()) {
#ifdef DEBUG_DIJKSTRA
    cout << "Loop " << loop << " ============= " << endl;
    print_pq(pq, "pq: ");
    print_vector(distances, "distances: ");
    print_deque(previous, "previous: ");
    print_deque(visited, "visited: ");
#endif
    const size_t to_visit = pq.top().second;
#ifdef DEBUG_DIJKSTRA
    cout << "to_visit = " << to_visit << endl;
#endif
    pq.pop();
    auto N = mHead[to_visit].cbegin();
    auto neighbor_node = N + 1;
#ifdef DEBUG_DIJKSTRA
    cout << "Visiting node " << N->mValue << ": \n";
#endif
    while (neighbor_node != mHead[to_visit].cend()) {
      const size_t neighbor_index = neighbor_node->mIndex;
      if (visited[neighbor_index] == false) {
#ifdef DEBUG_DIJKSTRA
        cout << neighbor_index << " (unvisited) ";
#endif
        const DistanceType new_distance =
            calc_new_dist(distances[to_visit], neighbor_node->mWeight);
        if (new_distance < distances[neighbor_index]) {
#ifdef DEBUG_DIJKSTRA
          cout << "neighbor_index = " << static_cast<double>(neighbor_index)
               << " ; new_distance = " << new_distance << endl;
#endif
          distances[neighbor_index] = new_distance;
          previous[neighbor_index] = to_visit;
          pq.push(make_pair(distances[neighbor_index], neighbor_index));
        }
      } else {
#ifdef DEBUG_DIJKSTRA
        cout << neighbor_index << " (visited) ";
#endif
      }
      neighbor_node = neighbor_node + 1;
    }
#ifdef DEBUG_DIJKSTRA
    cout << endl;
#endif
    if (to_visit == addr_end) {
      break;
    }
    visited[to_visit] = true;
    ++loop;
#ifdef DEBUG_DIJKSTRA
    cout << endl;
#endif
  }
  QVector<size_t> path;
  size_t target = addr_end;
  while (previous[target] != mNumNodes) {
    path.push_back(target);
    target = previous[target];
  }
  if (path.back() != addr_end)
    path.push_back(addr_start);
  std::reverse(path.begin(), path.end());
  QVector<double> res_distance(distances.size());
  for (int i = 0; i < distances.size(); ++i) {
    res_distance[i] = static_cast<double>(distances[i]);
  }
  DijkstraResults result{loop, visited, path, res_distance};
  return result;
}

class MFEPDistance {
public:
  MFEPDistance();
  MFEPDistance(const std::initializer_list<double> &l);
  MFEPDistance operator+(const double &rhs) const;
  friend std::ostream &operator<<(std::ostream &os, const MFEPDistance &rhs);
  friend auto operator<=>(const MFEPDistance &lhs, const MFEPDistance &rhs);
  explicit operator double() const;

private:
  std::priority_queue<double> mDistance;
};

std::ostream &operator<<(std::ostream &os, const MFEPDistance &rhs);
auto operator<=>(const MFEPDistance &lhs, const MFEPDistance &rhs);

#endif // GRAPH_H
