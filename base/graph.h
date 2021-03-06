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

#ifndef GRAPH_H
#define GRAPH_H

#include "base/helper.h"

#include <QDebug>
#include <QElapsedTimer>
#include <algorithm>
#include <compare>
#include <cstddef>
#include <deque>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <list>
#include <queue>
#include <vector>

#if defined(USE_BOOST_FIBONACCI_HEAP)
#include <boost/heap/fibonacci_heap.hpp>
#define USE_BOOST_HEAP
#elif defined(USE_BOOST_BINOMIAL_HEAP)
#define USE_BOOST_HEAP
#include <boost/heap/binomial_heap.hpp>
#elif defined(USE_BOOST_D_ARY_HEAP)
#define USE_BOOST_HEAP
#ifndef ARITY
#define ARITY 4
#endif
#include <boost/heap/d_ary_heap.hpp>
#elif defined(USE_BOOST_PRIORITY_QUEUQ)
#include <boost/heap/priority_queue.hpp>
#define USE_BOOST_HEAP
#ifdef USE_BOOST_ORDERED_ITERATOR
#undef USE_BOOST_ORDERED_ITERATOR
#endif
#else
#ifdef USE_BOOST_ORDERED_ITERATOR
#undef USE_BOOST_ORDERED_ITERATOR
#endif
#endif

#ifdef QT_DEBUG
#define DEBUG_SPFA
#define DEBUG_DIJKSTRA
#endif

class Graph {
public:
  enum class FindPathMode {
    SumOfEdges,
    MaximumEdges,
    MFEPMode,
  };
  enum class FindPathAlgorithm {
    Dijkstra,
    SPFA,
  };
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
    std::vector<bool> mVisitedNodes;
    std::vector<size_t> mPathNodes;
    std::vector<double> mDistances;
    void dump() const;
  };
  Graph();
  Graph(const size_t numNodes, bool directed = false);
  bool setEdge(size_t source, size_t destination, double weight = 1.0);
  bool setEdges(const std::vector<Edge> &edges);
  void sortByWeight();
  bool getEdge(size_t source, size_t destination, double &weight) const;
  void printGraph(std::ostream &os) const;
  void summary() const;
  size_t totalEdges() const;
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
  std::vector<std::deque<Node>> mHead;
  bool setEdgeHelper(size_t source, size_t destination, double weight = 1.0);
  void DFSHelper(size_t i, std::vector<bool> &visited,
                 std::function<void(const Node &)> func) const;
};

template <typename DistanceType>
Graph::FindPathResult Graph::Dijkstra(
    size_t start, size_t end, const DistanceType &dist_start,
    const DistanceType &dist_infinity,
    std::function<DistanceType(DistanceType, double)> calc_new_dist) const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  using std::make_pair;
  using std::priority_queue;
  typedef std::pair<DistanceType, size_t> DistNodePair;
  std::vector<bool> visited(mNumNodes, false);
  std::vector<size_t> previous(mNumNodes);
  priority_queue<DistNodePair, std::vector<DistNodePair>,
                 std::greater<DistNodePair>>
      pq;
  std::vector<DistanceType> distances(mNumNodes);
  for (size_t i = 0; i < mNumNodes; ++i) {
    distances[i] = (i == start) ? dist_start : dist_infinity;
    previous[i] = mNumNodes;
  }
  pq.push(make_pair(dist_start, start));
  size_t loop = 0;
  QElapsedTimer timer;
  timer.start();
  while (!pq.empty()) {
#ifdef DEBUG_DIJKSTRA
    qDebug() << "Loop " << loop << " ============= ";
    debug_priority_queue(pq, "Current priority search queue:");
    qDebug() << "Distance from" << start << ":" << distances;
    qDebug() << "Previous visited vertices array:" << previous;
    qDebug() << "Visited vertices:" << visited;
#endif
    const size_t to_visit = pq.top().second;
#ifdef DEBUG_DIJKSTRA
    qDebug() << "Visiting neighbor vertices of vertex" << to_visit << ":";
#endif
    pq.pop();
    auto neighbor_node = std::next(mHead[to_visit].cbegin(), 1);
    while (neighbor_node != mHead[to_visit].cend()) {
      const size_t neighbor_index = neighbor_node->mIndex;
      if (visited[neighbor_index] == false) {
#ifdef DEBUG_DIJKSTRA
        qDebug() << "Neighbor vertex" << neighbor_index << "is not visited";
#endif
        const DistanceType new_distance =
            calc_new_dist(distances[to_visit], neighbor_node->mWeight);
#ifdef DEBUG_DIJKSTRA
        qDebug() << "Current distance of vertex " << neighbor_index << "is"
                 << distances[neighbor_index];
#endif
        if (new_distance < distances[neighbor_index]) {
#ifdef DEBUG_DIJKSTRA
          qDebug() << "Distance is updated to" << new_distance;
#endif
          distances[neighbor_index] = new_distance;
          previous[neighbor_index] = to_visit;
          pq.push(make_pair(distances[neighbor_index], neighbor_index));
        }
      } else {
#ifdef DEBUG_DIJKSTRA
        qDebug() << "Neighbor vertex" << neighbor_index
                 << "is already visited. Skip it";
#endif
      }
      std::advance(neighbor_node, 1);
    }
    if (to_visit == end) {
      break;
    }
    visited[to_visit] = true;
    ++loop;
#ifdef DEBUG_DIJKSTRA
    qDebug() << "===============================================";
#endif
  }
  qDebug() << "Dijkstra's algorithm takes" << timer.elapsed()
           << "milliseconds; total number of loops:" << loop;
  std::vector<size_t> path;
  size_t target = end;
  while (previous[target] != mNumNodes) {
    path.push_back(target);
    target = previous[target];
  }
  if (path.back() != end)
    path.push_back(start);
  std::reverse(path.begin(), path.end());
  std::vector<double> res_distance(distances.size());
  for (size_t i = 0; i < distances.size(); ++i) {
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
  qDebug() << "Calling" << Q_FUNC_INFO;
  std::vector<DistanceType> distances(mNumNodes);
  std::vector<bool> visited(mNumNodes, false);
  std::vector<std::deque<size_t>> paths(mNumNodes);
  std::vector<bool> in_search_queue(mNumNodes, false);
  for (size_t i = 0; i < mNumNodes; ++i) {
    distances[i] = (i == start) ? dist_start : dist_infinity;
  }
  paths[start].push_back(start);
  std::deque<size_t> search_queue;
  search_queue.push_back(start);
  in_search_queue[start] = true;
  size_t loop = 0;
  QElapsedTimer timer;
  timer.start();
  DistanceType new_distance = DistanceType();
  while (!search_queue.empty()) {
#ifdef DEBUG_SPFA
    qDebug() << "==================== Loop" << loop << "====================";
    qDebug() << "Current search queue:" << search_queue;
    qDebug() << "Visited vertices:" << visited;
#endif
    const size_t to_visit = search_queue.front();
    search_queue.pop_front();
    in_search_queue[to_visit] = false;
#ifdef DEBUG_SPFA
    qDebug() << "Vertex being visited:" << to_visit;
#endif
    auto neighbor_node = std::next(mHead[to_visit].cbegin(), 1);
    while (neighbor_node != mHead[to_visit].cend()) {
      const size_t neighbor_index = neighbor_node->mIndex;
#ifdef DEBUG_SPFA
      qDebug() << "Visiting neighbor vertex:" << neighbor_index;
#endif
      new_distance = calc_new_dist(distances[to_visit], neighbor_node->mWeight);
#ifdef DEBUG_SPFA
      qDebug() << "Current distance:" << distances[neighbor_index];
      qDebug() << "Current path:" << paths[neighbor_index];
      qDebug() << "Path of the previous vertex:" << paths[to_visit];
      qDebug() << "Distance of previous vertex from start:"
               << distances[to_visit];
      qDebug() << "Calculated new distance:" << new_distance;
#endif
      if (new_distance < distances[neighbor_index]) {
#ifdef DEBUG_SPFA
        qDebug() << "Update new distance at" << neighbor_index
                 << " ; new distance = " << new_distance;
#endif
        distances[neighbor_index] = new_distance;
        paths[neighbor_index] = paths[to_visit];
        paths[neighbor_index].push_back(neighbor_index);
#ifdef DEBUG_SPFA
        qDebug() << "New path =" << paths[neighbor_index];
#endif
        if (in_search_queue[neighbor_index] == false) {
          search_queue.push_back(neighbor_index);
          in_search_queue[neighbor_index] = true;
        }
      }
      std::advance(neighbor_node, 1);
    }
    visited[to_visit] = true;
    ++loop;
#ifdef DEBUG_SPFA
    qDebug() << "===============================================";
#endif
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
  qDebug() << "SPFA takes" << timer.elapsed()
           << "milliseconds; total number of loops:" << loop;
  std::vector<size_t> path;
  for (const auto &i : paths[end]) {
    path.push_back(i);
  }
  std::vector<double> res_distance(distances.size());
  for (size_t i = 0; i < distances.size(); ++i) {
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
#ifdef USE_BOOST_HEAP
  MFEPDistance(const MFEPDistance &rhs);
  void operator=(const MFEPDistance &rhs);
#endif

private:
#if defined(USE_BOOST_FIBONACCI_HEAP)
  boost::heap::fibonacci_heap<double> mDistance;
#elif defined(USE_BOOST_BINOMIAL_HEAP)
  boost::heap::binomial_heap<double> mDistance;
#elif defined(USE_BOOST_D_ARY_HEAP)
  boost::heap::d_ary_heap<double, boost::heap::arity<ARITY>> mDistance;
#elif defined(USE_BOOST_PRIORITY_QUEUQ)
  boost::heap::priority_queue<double> mDistance;
#else
  std::priority_queue<double> mDistance;
#endif
};

std::ostream &operator<<(std::ostream &os, const MFEPDistance &rhs);
QDebug operator<<(QDebug dbg, const MFEPDistance &rhs);
auto operator<=>(const MFEPDistance &lhs, const MFEPDistance &rhs);

#endif // GRAPH_H
