#include "graph.h"

Graph::Graph() : mNumNodes(0), mIsDirected(false), mHead(0) {}

Graph::Graph(const size_t numNodes, bool directed)
    : mNumNodes(numNodes), mIsDirected(directed), mHead(mNumNodes) {
  for (size_t i = 0; i < mNumNodes; ++i) {
    mHead[i].append(Node{i, 0.0});
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

bool Graph::setEdges(const QVector<Graph::Edge> &edges) {
  for (int i = 0; i < edges.size(); ++i) {
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
    for (int j = 1; j < current.size(); ++j) {
      os << "(" << current[j].mIndex << " weight " << current[j].mWeight
         << ") ";
    }
    os << "\n";
  }
}

void Graph::DFS(size_t start, std::function<void(const Node &)> func) const {
  QVector<bool> visited(mNumNodes, false);
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

Graph::FindPathResult Graph::SPFA(size_t start, size_t end, Graph::FindPathMode mode)
{
  switch (mode) {
  case Graph::FindPathMode::SumOfEdges: {
    const double dist_inf = std::numeric_limits<double>::max();
    return SPFA<double>(
        start, end, 0, dist_inf,
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

double Graph::findMaxSumWeight() const
{
  double result = 0;
  for (size_t i = 0; i < mNumNodes; ++i) {
    auto this_node = mHead[i].begin();
    while (this_node != mHead[i].end()) {
      result += std::abs(this_node->mWeight);
      this_node = this_node + 1;
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
    headList.append(Node{destination, weight});
  }
  return true;
}

void Graph::DFSHelper(size_t i, QVector<bool> &visited,
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
    // TODO: std::sort
    std::sort(
        current_list.begin() + 1, current_list.end(),
        [](const Node &a, const Node &b) { return a.mWeight < b.mWeight; });
  }
}

MFEPDistance::MFEPDistance() {}

MFEPDistance::MFEPDistance(const std::initializer_list<double> &l) {
  for (const auto &it : l) {
    mDistance.push(it);
  }
}

MFEPDistance MFEPDistance::operator+(const double &rhs) const {
  MFEPDistance res(*this);
  res.mDistance.push(rhs);
  return res;
}

MFEPDistance::operator double() const {
  if (mDistance.empty())
    return 0;
  else
    return mDistance.top();
}

std::ostream &operator<<(std::ostream &os, const MFEPDistance &rhs) {
  std::priority_queue<double> tmpRhsQueue(rhs.mDistance);
  while (!tmpRhsQueue.empty()) {
    os << tmpRhsQueue.top() << ' ';
    tmpRhsQueue.pop();
  }
  return os;
}

auto operator<=>(const MFEPDistance &lhs, const MFEPDistance &rhs) {
  const size_t l_size = lhs.mDistance.size();
  const size_t r_size = rhs.mDistance.size();
  std::priority_queue<double> tmpLhsQueue(lhs.mDistance);
  std::priority_queue<double> tmpRhsQueue(rhs.mDistance);
  // TODO: simplify this!
  // FIXME: uninitialized variables!
  double lhsElement;
  double rhsElement;
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
}

void Graph::FindPathResult::dump() const
{
  using std::cout;
  using std::endl;
  using std::vector;
  cout << "Path: ";
  for (const auto& i : mPathNodes) {
    cout << i << " ";
  }
  cout << endl;
  cout << "Distance " << endl;
  for (int i = 0; i < mDistances.size(); ++i) {
    cout << "Node [" << i << "]: " << " distance = " << mDistances[i] << endl;
  }
  cout << "Number of loops in Dijkstra's algorithm: " << mNumLoops << endl;
}

QDebug operator<<(QDebug dbg, const MFEPDistance &rhs)
{
  std::priority_queue<double> tmpRhsQueue(rhs.mDistance);
  QString debug_string;
  while (!tmpRhsQueue.empty()) {
    debug_string += QString::number(tmpRhsQueue.top()) + ' ';
    tmpRhsQueue.pop();
  }
  dbg << debug_string;
  return dbg;
}
