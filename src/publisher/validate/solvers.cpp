#include "validate/solvers.hh"
#include "parse/ast_print.hh"

namespace solvers {

Errorable<void> SizeSolver::unite(const SizeTerm &x, const SizeTerm &y) {
  SizeTerm root_x = find(x);
  SizeTerm root_y = find(y);

  // Already in same set
  if (root_x == root_y) {
    return {};
  }

  specvalues::ConcreateSize *cs_x_ptr =
      std::get_if<specvalues::ConcreateSize>(std::get_if<SizeValue>(&root_x));
  specvalues::ConcreateSize *cs_y_ptr =
      std::get_if<specvalues::ConcreateSize>(std::get_if<SizeValue>(&root_y));

  if (cs_x_ptr != nullptr && cs_y_ptr != nullptr) {
    if (*cs_x_ptr != *cs_y_ptr) {
      std::ostringstream s;
      s << "Can't unite " << *cs_x_ptr << " and " << *cs_y_ptr;
      return err(s.str());
    }
  } else {
    if (root_x < root_y) {
      this->parent[root_y] = root_x;
    } else {
      this->parent[root_x] = root_y;
    }
  }
  return {};
}

void BBSolver::rebuild() {
  if (!dirty)
    return;

  // 1. Collect all IDs mentioned
  std::unordered_set<BBTerm> nodes;
  for (auto &[a, b] : inequalities) {
    nodes.insert(a);
    nodes.insert(b);
  }

  // 2. Build SCCs (Kosaraju)
  // > Maybe latter will be better change it to some incremental version
  std::unordered_map<BBTerm, std::vector<BBTerm>> adj;
  for (auto &[a, b] : inequalities)
    adj[a].push_back(b);

  std::unordered_map<BBTerm, bool> visited;
  std::vector<BBTerm> order;
  std::function<void(BBTerm)> dfs1 = [&](BBTerm u) {
    visited[u] = true;
    for (BBTerm v : adj[u])
      if (!visited[v])
        dfs1(v);
    order.push_back(u);
  };
  for (BBTerm u : nodes)
    if (!visited[u])
      dfs1(u);

  std::unordered_map<BBTerm, std::vector<BBTerm>> rev_adj;
  for (auto &[a, b] : inequalities)
    rev_adj[b].push_back(a);

  std::unordered_map<Id, Id> scc_root;
  std::unordered_map<Id, std::vector<Id>> scc_members;

  parent.clear();
  std::function<void(BBTerm, BBTerm)> dfs2 = [&](BBTerm u, BBTerm root) {
    EqualitySolver::addEquation(u, root);
    for (BBTerm v : rev_adj[u])
      if (!EqualitySolver::contains(v))
        dfs2(v, root);
  };

  std::reverse(order.begin(), order.end());
  for (BBTerm u : order) {
    if (!EqualitySolver::contains(u))
      dfs2(u, u);
  }

  // 5. Build DAG of representatives (using min IDs)
  dag.clear();
  for (auto &[a, b] : inequalities) {
    BBTerm ra = EqualitySolver::find(a); // returns the min representative
    BBTerm rb = EqualitySolver::find(b);
    if (ra != rb) {
      auto &vec = dag[ra];
      if (std::find(vec.begin(), vec.end(), rb) == vec.end())
        vec.push_back(rb);
    }
  }

  dirty = false;
}

bool BBSolver::reachable(BBTerm from, BBTerm to) {
  std::unordered_set<BBTerm> visited;
  std::function<bool(BBTerm)> dfs = [&](BBTerm u) {
    if (u == to)
      return true;
    visited.insert(u);
    auto it = dag.find(u);
    if (it == dag.end())
      return false;
    for (BBTerm v : it->second) {
      if (!visited.count(v) && dfs(v))
        return true;
    }
    return false;
  };
  return dfs(from);
}

bool BBSolver::addLessOrEqual(BBTerm a, BBTerm b) {
  if (isLessOrEqual(a, b))
    return false;
  inequalities.emplace_back(a, b);
  dirty = true;
  return true;
}

bool BBSolver::isLessOrEqual(BBTerm a, BBTerm b) {
  BBTerm ra = find(a);
  BBTerm rb = find(b);
  if (ra == rb)
    return true;
  rebuild();
  return reachable(ra, rb);
}

Errorable<void> BBSolver::unite(const BBTerm &x, const BBTerm &y) {
  BBTerm root_x = find(x);
  BBTerm root_y = find(y);

  // Already in same set
  if (root_x == root_y) {
    return {};
  }

  if (root_x < root_y) {
    this->parent[root_y] = root_x;
  } else {
    this->parent[root_x] = root_y;
  }

  return {};
}

BBTerm BBSolver::find(const BBTerm &x) {
  if (!EqualitySolver::contains(x)) {
    dirty = true;
  }
  return EqualitySolver::find(x);
}

bool BBSolver::isEqual(const BBTerm &x, const BBTerm &y) {
  rebuild();
  return EqualitySolver::isEqual(x, y);
}

bool BBSolver::contains(const BBTerm &x) {
  rebuild();
  return EqualitySolver::contains(x);
}

std::ostream &operator<<(std::ostream &os, const SizeTerm &term) {
  return printVariant(os, term);
}

std::ostream &operator<<(std::ostream &os, const BBTerm &term) {
  return printVariant(os, term);
}

} // namespace solvers
