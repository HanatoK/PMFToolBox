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

#include "graph.h"

Graph::Graph() : mNumNodes(0), mIsDirected(false), mHead(0) {}

Graph::Graph(const size_t numNodes, bool directed)
    : mNumNodes(numNodes), mIsDirected(directed), mHead(mNumNodes) {
  for (size_t i = 0; i < mNumNodes; ++i) {
    mHead[i].push_back(Node{i, 0.0});
  }
}

bool Graph::setEdge(size_t source, size_t destination, double weight) {
  bool success = setEdgeHelper(source, destination, weight);
  if (!success)
    return success;
  if (mIsDirected == false) {
    success = success && setEdgeHelper(destination, source, weight);
  }
  return success;
}

bool Graph::setEdges(const std::vector<Edge> &edges) {
  for (size_t i = 0; i < edges.size(); ++i) {
    if (!setEdge(edges[i].mSource, edges[i].mDestination, edges[i].mWeight)) {
      return false;
    }
  }
  return true;
}

bool Graph::getEdge(size_t source, size_t destination, double &weight) const {
  if (source >= mNumNodes || destination >= mNumNodes || source == destination)
    return false;
  const auto &headList = mHead[source];
  auto it = headList.cbegin();
  while (it != headList.cend()) {
    if (it->mIndex == destination) {
      weight = it->mWeight;
      return true;
    }
    ++it;
  }
  weight = 0;
  return false;
}

void Graph::printGraph(std::ostream &os) const {
  for (size_t i = 0; i < mNumNodes; ++i) {
    const auto &current = mHead[i];
    os << "This node is " << current.cbegin()->mIndex;
    if (current.size() < 2)
      continue;
    os << ", linked to ";
    auto it_current = current.cbegin();
    std::advance(it_current, 1);
    while (it_current != current.cend()) {
      os << "(" << it_current->mIndex << " weight " << it_current->mWeight
         << ") ";
      std::advance(it_current, 1);
    }
    os << "\n";
  }
}

void Graph::summary() const {
  qDebug() << "Summary of the graph:";
  qDebug() << "Number of nodes:" << mHead.size();
  qDebug() << "Number of edges:" << totalEdges();
  qDebug() << "Is directed?" << mIsDirected;
}

size_t Graph::totalEdges() const {
  size_t count = 0;
  for (size_t i = 0; i < mHead.size(); ++i) {
    for (size_t j = 1; j < mHead[i].size(); ++j) {
      ++count;
    }
  }
  return count;
}

void Graph::DFS(size_t start, std::function<void(const Node &)> func) const {
  std::vector<bool> visited(mNumNodes, false);
  DFSHelper(start, visited, func);
}

Graph::FindPathResult Graph::Dijkstra(size_t start, size_t end,
                                      Graph::FindPathMode mode) {
  switch (mode) {
  case Graph::FindPathMode::SumOfEdges: {
    const double dist_inf = std::numeric_limits<double>::max();
    return Dijkstra<double>(
        start, end, 0, dist_inf,
        [](const double &x, const double &y) { return x + y; });
    break;
  }
  case Graph::FindPathMode::MaximumEdges: {
    const double dist_inf = std::numeric_limits<double>::max();
    return Dijkstra<double>(
        start, end, findMaxSumWeight() + 1.0, dist_inf,
        [](const double &x, const double &y) { return std::max(x, y); });
    break;
  }
  case Graph::FindPathMode::MFEPMode: {
    const MFEPDistance dist_start;
    const MFEPDistance dist_inf({std::numeric_limits<double>::max()});
    return Dijkstra<MFEPDistance>(
        start, end, dist_start, dist_inf,
        [](const MFEPDistance &x, const double &weight) { return x + weight; });
  }
  default: {
    return FindPathResult();
    break;
  }
  }
}

Graph::FindPathResult Graph::SPFA(size_t start, size_t end,
                                  Graph::FindPathMode mode) {
  switch (mode) {
  case Graph::FindPathMode::SumOfEdges: {
    const double dist_inf = std::numeric_limits<double>::max();
    return SPFA<double>(start, end, 0, dist_inf,
                        [](const double &x, const double &y) { return x + y; });
    break;
  }
  case Graph::FindPathMode::MaximumEdges: {
    const double dist_inf = std::numeric_limits<double>::max();
    return SPFA<double>(
        start, end, findMaxSumWeight() + 1.0, dist_inf,
        [](const double &x, const double &y) { return std::max(x, y); });
    break;
  }
  case Graph::FindPathMode::MFEPMode: {
    const MFEPDistance dist_start;
    const MFEPDistance dist_inf({std::numeric_limits<double>::max()});
    return SPFA<MFEPDistance>(
        start, end, dist_start, dist_inf,
        [](const MFEPDistance &x, const double &weight) { return x + weight; });
  }
  default: {
    return FindPathResult();
    break;
  }
  }
}

double Graph::findMaxSumWeight() const {
  double result = 0;
  for (size_t i = 0; i < mNumNodes; ++i) {
    auto this_node = mHead[i].cbegin();
    while (this_node != mHead[i].cend()) {
      result += std::abs(this_node->mWeight);
      std::advance(this_node, 1);
    }
  }
  return result;
}

bool Graph::setEdgeHelper(size_t source, size_t destination, double weight) {
  if (source >= mNumNodes || destination >= mNumNodes || source == destination)
    return false;
  // access the source vertex
  auto &headList = mHead[source];
  // iterate over the list
  auto it = headList.begin();
  while (it != headList.end()) {
    if (it->mIndex == destination) {
      // if the edge is already in the list
      // just set the weight
      it->mWeight = weight;
      break;
    }
    ++it;
  }
  if (it == headList.end()) {
    // the destination vertex is not in the list
    // create a new edge
    headList.push_back(Node{destination, weight});
  }
  return true;
}

void Graph::DFSHelper(size_t i, std::vector<bool> &visited,
                      std::function<void(const Node &)> func) const {
  // mark this vertex as visited
  visited[i] = true;
  auto it = mHead[i].cbegin();
  func(*it);
  while (it != mHead[i].end()) {
    const size_t to_visit = it->mIndex;
    if (!visited[to_visit]) {
      // if this node is never visited, visit it
      DFSHelper(to_visit, visited, func);
    }
    ++it;
  }
}

void Graph::sortByWeight() {
  for (size_t i = 0; i < mNumNodes; ++i) {
    auto &current_list = mHead[i];
    // no need to sort if this is an isolated vertex
    if (current_list.size() < 2)
      continue;
    auto it_sort_start = current_list.begin();
    auto it_sort_end = current_list.end();
    std::advance(it_sort_start, 1);
    std::sort(it_sort_start, it_sort_end, [](const Node &a, const Node &b) {
      return a.mWeight < b.mWeight;
    });
  }
}

MFEPDistance::MFEPDistance() {}

MFEPDistance::MFEPDistance(const std::initializer_list<double> &l) {
  for (const auto &it : l) {
    mDistance.push(it);
  }
}

#ifdef USE_BOOST_HEAP
MFEPDistance::MFEPDistance(const MFEPDistance &rhs) {
  this->mDistance.clear();
  for (auto it = rhs.mDistance.begin(); it != rhs.mDistance.end(); ++it) {
    this->mDistance.push(*it);
  }
}
#endif

MFEPDistance MFEPDistance::operator+(const double &rhs) const {
  MFEPDistance res(*this);
  res.mDistance.push(rhs);
  return res;
}

#ifdef USE_BOOST_HEAP
void MFEPDistance::operator=(const MFEPDistance &rhs) {
  this->mDistance.clear();
  for (auto it = rhs.mDistance.begin(); it != rhs.mDistance.end(); ++it) {
    this->mDistance.push(*it);
  }
}
#endif

MFEPDistance::operator double() const {
  if (mDistance.empty())
    return 0;
  else
    return mDistance.top();
}

std::ostream &operator<<(std::ostream &os, const MFEPDistance &rhs) {
  auto tmpRhsQueue(rhs.mDistance);
  while (!tmpRhsQueue.empty()) {
    os << tmpRhsQueue.top() << ' ';
    tmpRhsQueue.pop();
  }
  return os;
}

auto operator<=>(const MFEPDistance &lhs, const MFEPDistance &rhs) {
  const size_t l_size = lhs.mDistance.size();
  const size_t r_size = rhs.mDistance.size();
#if defined(USE_BOOST_ORDERED_ITERATOR)
  auto it_lhs = lhs.mDistance.ordered_begin();
  auto it_rhs = rhs.mDistance.ordered_begin();
  auto it_lhs_end = lhs.mDistance.ordered_end();
  auto it_rhs_end = rhs.mDistance.ordered_end();
  double lhs_last_element = 0;
  double rhs_last_element = 0;
  if (l_size == r_size) {
    if (l_size == 0)
      return std::partial_ordering::equivalent;
    while (it_lhs != it_lhs_end) {
      if ((*it_lhs) == (*it_rhs)) {
        ++it_lhs;
        ++it_rhs;
        continue;
      } else if ((*it_lhs) < (*it_rhs)) {
        return std::partial_ordering::less;
      } else {
        return std::partial_ordering::greater;
      }
    }
    return std::partial_ordering::equivalent;
  } else if (l_size < r_size) {
    if (l_size == 0)
      return std::partial_ordering::less;
    while (it_lhs != it_lhs_end) {
      lhs_last_element = *it_lhs;
      if ((*it_lhs) == (*it_rhs)) {
        ++it_lhs;
        ++it_rhs;
        continue;
      } else if ((*it_lhs) < (*it_rhs)) {
        return std::partial_ordering::less;
      } else {
        return std::partial_ordering::greater;
      }
    }
    while (it_rhs != it_rhs_end) {
      rhs_last_element = *it_rhs;
      ++it_rhs;
    }
    if (lhs_last_element == rhs_last_element)
      return std::partial_ordering::less;
    return lhs_last_element <=> rhs_last_element;
  } else if (l_size > r_size) {
    if (r_size == 0)
      return std::partial_ordering::greater;
    while (it_rhs != it_rhs_end) {
      rhs_last_element = *it_rhs;
      if ((*it_lhs) == (*it_rhs)) {
        ++it_lhs;
        ++it_rhs;
        continue;
      } else if ((*it_lhs) < (*it_rhs)) {
        return std::partial_ordering::less;
      } else {
        return std::partial_ordering::greater;
      }
    }
    while (it_lhs != it_lhs_end) {
      lhs_last_element = *it_lhs;
      ++it_lhs;
    }
    if (lhs_last_element == rhs_last_element)
      return std::partial_ordering::greater;
    return lhs_last_element <=> rhs_last_element;
  }
  return std::partial_ordering::equivalent;
#else
  auto tmpLhsQueue(lhs.mDistance);
  auto tmpRhsQueue(rhs.mDistance);
  // TODO: simplify this!
  double lhsElement = 0;
  double rhsElement = 0;
  if (l_size == r_size) {
    if (l_size == 0)
      return std::partial_ordering::equivalent;
    while (!tmpLhsQueue.empty()) {
      lhsElement = tmpLhsQueue.top();
      rhsElement = tmpRhsQueue.top();
      if (lhsElement == rhsElement) {
        tmpLhsQueue.pop();
        tmpRhsQueue.pop();
      } else if (lhsElement < rhsElement) {
        return std::partial_ordering::less;
      } else {
        return std::partial_ordering::greater;
      }
    }
    // now both the queues should be empty
    return std::partial_ordering::equivalent;
  } else if (l_size < r_size) {
    if (l_size == 0)
      return std::partial_ordering::less;
    while (!tmpLhsQueue.empty()) {
      lhsElement = tmpLhsQueue.top();
      rhsElement = tmpRhsQueue.top();
      if (lhsElement == rhsElement) {
        tmpLhsQueue.pop();
        tmpRhsQueue.pop();
      } else if (lhsElement < rhsElement) {
        return std::partial_ordering::less;
      } else {
        return std::partial_ordering::greater;
      }
    }
    while (!tmpRhsQueue.empty()) {
      rhsElement = tmpRhsQueue.top();
      tmpRhsQueue.pop();
    }
    if (lhsElement == rhsElement)
      return std::partial_ordering::less;
    return lhsElement <=> rhsElement;
  } else if (l_size > r_size) {
    if (r_size == 0)
      return std::partial_ordering::greater;
    while (!tmpRhsQueue.empty()) {
      lhsElement = tmpLhsQueue.top();
      rhsElement = tmpRhsQueue.top();
      if (lhsElement == rhsElement) {
        tmpLhsQueue.pop();
        tmpRhsQueue.pop();
      } else if (lhsElement < rhsElement) {
        return std::partial_ordering::less;
      } else {
        return std::partial_ordering::greater;
      }
    }
    while (!tmpLhsQueue.empty()) {
      lhsElement = tmpLhsQueue.top();
      tmpLhsQueue.pop();
    }
    if (lhsElement == rhsElement)
      return std::partial_ordering::greater;
    return lhsElement <=> rhsElement;
  }
  return std::partial_ordering::equivalent;
#endif
}

void Graph::FindPathResult::dump() const {
  qDebug() << "Calling" << Q_FUNC_INFO;
  qDebug() << "Dump the result of the path finder:";
  qDebug() << "Number of loops:" << mNumLoops;
  qDebug() << "Path:" << mPathNodes;
  qDebug() << "Distance (only show the maximum energy barrier for MFEP):";
  for (size_t i = 0; i < mDistances.size(); ++i) {
    qDebug() << QString("Node[%1]:").arg(i) << mDistances[i];
  }
}

QDebug operator<<(QDebug dbg, const MFEPDistance &rhs) {
  auto tmpRhsQueue(rhs.mDistance);
  QString debug_string;
  while (!tmpRhsQueue.empty()) {
    debug_string += QString::number(tmpRhsQueue.top()) + ' ';
    tmpRhsQueue.pop();
  }
  dbg << debug_string;
  return dbg;
}
