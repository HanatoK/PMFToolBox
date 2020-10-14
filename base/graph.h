#ifndef GRAPH_H
#define GRAPH_H

#include <QDebug>
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
  struct FindPathResult {
    size_t mNumLoops;
    QVector<bool> mVisitedNodes;
    QVector<size_t> mPathNodes;
    QVector<double> mDistances;
    void dump() const;
  };
  enum class FindPathMode {
    SumOfEdges,
    MaximumEdges,
    MFEPMode,
  };
  enum class FindPathAlgorithm {
    Dijkstra,
    SPFA,
  };
  Graph();
  Graph(const size_t numNodes, bool directed = false);
  bool setEdge(size_t source, size_t destination, double weight = 1.0);
  bool setEdges(const QVector<Edge> &edges);
  void sortByWeight();
  bool getEdge(size_t source, size_t destination, double &weight) const;
  void printGraph(std::ostream &os) const;
  void DFS(size_t start, std::function<void(const Node &)> func) const;
  FindPathResult Dijkstra(size_t start, size_t end, FindPathMode mode);
  template <typename DistanceType>
  FindPathResult Dijkstra(
      size_t start, size_t end, const DistanceType &dist_start,
      const DistanceType &dist_infinity,
      std::function<DistanceType(DistanceType, double)> calc_new_dist) const;
  FindPathResult SPFA(size_t start, size_t end, FindPathMode mode);
  template <typename DistanceType>
  FindPathResult
  SPFA(size_t start, size_t end, const DistanceType &dist_start,
       const DistanceType &dist_infinity,
       std::function<DistanceType(DistanceType, double)> calc_new_dist) const;
  double findMaxSumWeight() const;

protected:
  size_t mNumNodes;
  bool mIsDirected;
  QVector<QList<Node>> mHead;
  bool setEdgeHelper(size_t source, size_t destination, double weight = 1.0);
  void DFSHelper(size_t i, QVector<bool> &visited,
                 std::function<void(const Node &)> func) const;
};

template <typename DistanceType>
Graph::FindPathResult Graph::Dijkstra(
    size_t start, size_t end, const DistanceType &dist_start,
    const DistanceType &dist_infinity,
    std::function<DistanceType(DistanceType, double)> calc_new_dist) const {
  qDebug() << "Calling " << Q_FUNC_INFO;
  using std::deque;
  using std::tuple;
#ifdef DEBUG_DIJKSTRA
  using std::cout;
  using std::endl;
#endif
  using std::make_pair;
  using std::priority_queue;
  typedef std::pair<DistanceType, size_t> DistNodePair;
  QVector<bool> visited(mNumNodes, false);
  QVector<size_t> previous(mNumNodes);
  priority_queue<DistNodePair, QVector<DistNodePair>,
                 std::greater<DistNodePair>>
      pq;
  QVector<DistanceType> distances(mNumNodes);
  for (size_t i = 0; i < mNumNodes; ++i) {
    distances[i] = (i == start) ? dist_start : dist_infinity;
    previous[i] = mNumNodes;
  }
  pq.push(make_pair(dist_start, start));
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
    if (to_visit == end) {
      break;
    }
    visited[to_visit] = true;
    ++loop;
#ifdef DEBUG_DIJKSTRA
    cout << endl;
#endif
  }
  QVector<size_t> path;
  size_t target = end;
  while (previous[target] != mNumNodes) {
    path.push_back(target);
    target = previous[target];
  }
  if (path.back() != end)
    path.push_back(start);
  std::reverse(path.begin(), path.end());
  QVector<double> res_distance(distances.size());
  for (int i = 0; i < distances.size(); ++i) {
    res_distance[i] = static_cast<double>(distances[i]);
  }
  FindPathResult result{loop, visited, path, res_distance};
  return result;
}

template <typename DistanceType>
Graph::FindPathResult Graph::SPFA(
    size_t start, size_t end, const DistanceType &dist_start,
    const DistanceType &dist_infinity,
    std::function<DistanceType(DistanceType, double)> calc_new_dist) const {
  qDebug() << "Calling " << Q_FUNC_INFO;
  QVector<DistanceType> distances(mNumNodes);
  QVector<bool> visited(mNumNodes, false);
//  QVector<size_t> previous(mNumNodes);
  QVector<QList<size_t>> paths(mNumNodes);
  for (size_t i = 0; i < mNumNodes; ++i) {
    distances[i] = (i == start) ? dist_start : dist_infinity;
//    previous[i] = mNumNodes;
  }
  paths[start].append(start);
  QList<size_t> Q;
  Q.push_back(start);
  size_t loop = 0;
  while (!Q.empty()) {
#ifdef DEBUG_SPFA
    qDebug() << "Loop" << loop;
    qDebug() << "Current search queue:" << Q;
//    qDebug() << "Previous visited vertices:" << previous;
    qDebug() << "Visited vertices:" << visited;
#endif
    const size_t to_visit = Q.front();
    Q.pop_front();
#ifdef DEBUG_SPFA
    qDebug() << "Vertex being visited:" << to_visit;
#endif
    auto N = mHead[to_visit].cbegin();
    auto neighbor_node = N + 1;
    while (neighbor_node != mHead[to_visit].cend()) {
      const size_t neighbor_index = neighbor_node->mIndex;
#ifdef DEBUG_SPFA
      qDebug() << "Visiting neighbor vertex:" << neighbor_index;
#endif
      const DistanceType new_distance =
          calc_new_dist(distances[to_visit], neighbor_node->mWeight);
      auto new_path = paths[neighbor_index];
      new_path.append(paths[to_visit]);
      new_path.append(neighbor_index);
      if (new_distance < distances[neighbor_index]) {
#ifdef DEBUG_SPFA
        qDebug() << "Update new distance at" << neighbor_index
                 << " ; new distance = " << new_distance;
#endif
        distances[neighbor_index] = new_distance;
//        previous[neighbor_index] = to_visit;
        paths[neighbor_index] = new_path;
        if (!Q.contains(neighbor_index)) Q.push_back(neighbor_index);
      }
      std::advance(neighbor_node, 1);
    }
    visited[to_visit] = true;
    ++loop;
  }
#ifdef DEBUG_SPFA
  qDebug() << "Distances from" << start << "to each vertex:";
  for (size_t i = 0; i < mNumNodes; ++i) {
    qDebug() << "i =" << i << ":" << distances[i];
  }
  qDebug() << "Paths from" << start << "to each vertex:";
  for (size_t i = 0; i < mNumNodes; ++i) {
    qDebug() << "i =" << i << ":" << paths[i];
  }
#endif
  QVector<size_t> path;
  auto it_start = std::find(paths[end].rbegin(), paths[end].rend(), start);
  auto it_end = std::find(paths[end].rbegin(), paths[end].rend(), end);
  if (it_start != paths[end].rend() && it_end != paths[end].rend()) {
    for (auto i = it_start; i != it_end - 1; --i) {
      path.push_back(*i);
    }
  }
  QVector<double> res_distance(distances.size());
  for (int i = 0; i < distances.size(); ++i) {
    res_distance[i] = static_cast<double>(distances[i]);
  }
  FindPathResult result{loop, visited, path, res_distance};
  return result;
}

class MFEPDistance {
public:
  MFEPDistance();
  MFEPDistance(const std::initializer_list<double> &l);
  MFEPDistance operator+(const double &rhs) const;
  friend std::ostream &operator<<(std::ostream &os, const MFEPDistance &rhs);
  friend QDebug operator<<(QDebug dbg, const MFEPDistance &ax);
  friend auto operator<=>(const MFEPDistance &lhs, const MFEPDistance &rhs);
  explicit operator double() const;

private:
  std::priority_queue<double> mDistance;
};

std::ostream &operator<<(std::ostream &os, const MFEPDistance &rhs);
QDebug operator<<(QDebug dbg, const MFEPDistance &rhs);
auto operator<=>(const MFEPDistance &lhs, const MFEPDistance &rhs);

#endif // GRAPH_H
