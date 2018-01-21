#include "enumerate_ccp.hpp"

#include <set>

#include "utils/assert.hpp"

namespace opossum {

EnumerateCcp::EnumerateCcp(size_t num_vertices, std::vector<std::pair<size_t, size_t>> edges):
  _num_vertices(num_vertices),
  _edges(std::move(edges))
{
#if IS_DEBUG
  // Test the input data for validity, i.e. whether all mentioned vertex indices in the edges are smaller than
  // _num_vertices
  for (const auto& edge : _edges) {
    Assert(edge.first < _num_vertices && edge.second < _num_vertices, "Vertex Index out of range");
  }
#endif
}

std::vector<std::pair<boost::dynamic_bitset<>, boost::dynamic_bitset<>>> EnumerateCcp::operator()() {
  for (size_t reverse_vertex_idx = 0; reverse_vertex_idx < _num_vertices; ++reverse_vertex_idx) {
    const auto forward_vertex_idx = _num_vertices - reverse_vertex_idx - 1;

    auto start_vertex_set = boost::dynamic_bitset<>(_num_vertices);
    start_vertex_set.set(forward_vertex_idx);
    _enumerate_cmp(start_vertex_set);

    std::vector<boost::dynamic_bitset<>> csgs;
    _enumerate_csg_recursive(csgs, start_vertex_set, _exclusion_set(forward_vertex_idx));
    for (const auto& csg : csgs) {
      _enumerate_cmp(csg);
    }
  }

#if IS_DEBUG
  // Assert that algorithm didn't create duplicates
  std::set<std::pair<boost::dynamic_bitset<>, boost::dynamic_bitset<>>> csg_cmp_pair_set;
  for (const auto& csg_cmp_pair : _csg_cmp_pairs) {
    Assert(csg_cmp_pair_set.emplace(csg_cmp_pair).second, "Duplicate CCP was generated");
  }
#endif

  return _csg_cmp_pairs;
}

void EnumerateCcp::_enumerate_csg_recursive(std::vector<boost::dynamic_bitset<>>& csgs, const boost::dynamic_bitset<>& vertex_set, const boost::dynamic_bitset<>& exclusion_set) {
  const auto neighbourhood = _neighbourhood(vertex_set, exclusion_set);
  const auto neighbourhood_subsets = _non_empty_subsets(neighbourhood);
  const auto extended_exclusion_set = exclusion_set | neighbourhood;

  for (const auto& subset : neighbourhood_subsets) {
    csgs.emplace_back(subset | vertex_set);
  }

  for (const auto& subset : neighbourhood_subsets) {
    _enumerate_csg_recursive(csgs, subset | vertex_set, extended_exclusion_set);
  }
}

void EnumerateCcp::_enumerate_cmp(const boost::dynamic_bitset<>& vertex_set) {
  const auto exclusion_set = _exclusion_set(vertex_set.find_first()) | vertex_set;
  const auto neighbourhood = _neighbourhood(vertex_set, exclusion_set);
  const auto extended_exclusion_set = exclusion_set | neighbourhood;

  if (neighbourhood.none()) return;

  std::vector<size_t> reverse_vertex_indices;
  auto current_vertex_idx = neighbourhood.find_first();

  do {
    reverse_vertex_indices.emplace_back(current_vertex_idx);
  } while((current_vertex_idx = neighbourhood.find_next(current_vertex_idx)) != boost::dynamic_bitset<>::npos);

  for (auto iter = reverse_vertex_indices.rbegin(); iter != reverse_vertex_indices.rend(); ++iter) {
    auto cmp_vertex_set = boost::dynamic_bitset<>(_num_vertices);
    cmp_vertex_set.set(*iter);

    _csg_cmp_pairs.emplace_back(std::make_pair(vertex_set, cmp_vertex_set));

    std::vector<boost::dynamic_bitset<>> csgs;
    _enumerate_csg_recursive(csgs, cmp_vertex_set, extended_exclusion_set);
    for (const auto& csg : csgs) {
      _csg_cmp_pairs.emplace_back(std::make_pair(vertex_set, csg));
    }
  }
}

boost::dynamic_bitset<> EnumerateCcp::_exclusion_set(const size_t vertex_idx) const {
  boost::dynamic_bitset<> exclusion_set(_num_vertices);
  for (size_t exclusion_vertex_idx = 0; exclusion_vertex_idx < vertex_idx; ++exclusion_vertex_idx) {
    exclusion_set.set(exclusion_vertex_idx);
  }
  return exclusion_set;
}

boost::dynamic_bitset<> EnumerateCcp::_neighbourhood(const boost::dynamic_bitset<>& vertex_set, const boost::dynamic_bitset<>& exclusion_set) const {
  boost::dynamic_bitset<> neighbourhood(_num_vertices);

  auto current_vertex_idx = vertex_set.find_first();
  Assert(current_vertex_idx != boost::dynamic_bitset<>::npos, "Cannot find neighbourhood of empty vertex set");

  for (const auto &edge : _edges) {
    if (vertex_set[edge.first] && !vertex_set[edge.second] && !exclusion_set[edge.second]) neighbourhood.set(edge.second);
    if (vertex_set[edge.second] && !vertex_set[edge.first] && !exclusion_set[edge.first]) neighbourhood.set(edge.first);
  }

  return neighbourhood;
}

std::vector<boost::dynamic_bitset<>> EnumerateCcp::_non_empty_subsets(const boost::dynamic_bitset<> &vertex_set) const {
  if (vertex_set.none()) return {};

  std::vector<boost::dynamic_bitset<>> subsets;

  const auto s = vertex_set.to_ulong();
  auto s1 = s & -s;

  while (s1 != s) {
    subsets.emplace_back(_num_vertices, s1);
    s1 = s & (s1 - s);
  }
  subsets.emplace_back(vertex_set);

  return subsets;
}

}  // namespace opossum
