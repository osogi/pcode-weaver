#include "validate/solvers.hh"

namespace solvers {

bool EqualitySolver::unite(const Term &x, const Term &y) {
  Term root_x = find(x);
  Term root_y = find(y);

  // Already in same set
  if (root_x == root_y) {
    return true;
  }

  if (std::holds_alternative<Value>(root_x) &&
      std::holds_alternative<Value>(root_y)) {
    return std::get<Value>(root_x) == std::get<Value>(root_y);
  } else {
    if (root_x < root_y) {
      this->parent[root_y] = root_x;
    } else {
      this->parent[root_x] = root_y;
    }
    return true;
  }
}

Term EqualitySolver::find(const Term &x) {
  // Initialize if not exists
  if (parent.find(x) == parent.end()) {
    parent[x] = x;
  }

  // Path compression
  if (parent[x] != x) {
    parent[x] = find(parent[x]);
  }

  return parent[x];
}

std::optional<Term> EqualitySolver::findopt(const Term &x) {
  if (parent.find(x) == parent.end()) {
    return std::nullopt;
  } else {
    return find(x);
  }
}

// Add equation: term1 = term2
Errorable<bool>
EqualitySolver::addEquation(const Term &left, const Term &right) {
  // if already equal return false
  if (isEqual(left, right)) {
    return false;
  } else {
    // if there isn't coflicts return true
    if (unite(left, right)) {
      return true;
    } else {
      std::ostringstream s;
      s << "Can't unite " << left << " and " << right;
      return err(s.str());
    }
  }
}

bool EqualitySolver::isEqual(const Term &x, const Term &y) {
  return find(x) == find(y);
}

void PartialOrderSolver::rebuild() {
  if (!dirty)
    return;

  // 1. Collect all IDs mentioned
  std::unordered_set<Id> nodes;
  for (auto &[a, b] : inequalities) {
    nodes.insert(a);
    nodes.insert(b);
  }

  // 2. Build SCCs (Kosaraju)
  // > Maybe latter will be better change it to some incremental version
  std::unordered_map<Id, std::vector<Id>> adj;
  for (auto &[a, b] : inequalities)
    adj[a].push_back(b);

  std::unordered_map<Id, bool> visited;
  std::vector<Id> order;
  std::function<void(Id)> dfs1 = [&](Id u) {
    visited[u] = true;
    for (Id v : adj[u])
      if (!visited[v])
        dfs1(v);
    order.push_back(u);
  };
  for (Id u : nodes)
    if (!visited[u])
      dfs1(u);

  std::unordered_map<Id, std::vector<Id>> rev_adj;
  for (auto &[a, b] : inequalities)
    rev_adj[b].push_back(a);

  std::unordered_map<Id, Id> scc_root;
  std::unordered_map<Id, std::vector<Id>> scc_members;

  std::function<void(Id, Id)> dfs2 = [&](Id u, Id root) {
    scc_root[u] = root;
    scc_members[root].push_back(u);
    for (Id v : rev_adj[u])
      if (scc_root.find(v) == scc_root.end())
        dfs2(v, root);
  };

  std::reverse(order.begin(), order.end());
  for (Id u : order) {
    if (scc_root.find(u) == scc_root.end())
      dfs2(u, u);
  }

  // 3. Choose smallest ID in each SCC as the representative
  std::unordered_map<Id, Id> scc_min; // old root -> min ID
  for (auto &[root, members] : scc_members) {
    Id min_id = *std::min_element(members.begin(), members.end());
    scc_min[root] = min_id;
  }

  // 4. Set union-find parent to the min representative
  parent.clear();
  for (auto &[id, root] : scc_root) {
    parent[id] =
        scc_min[root]; // all IDs in the same SCC point to the same min ID
  }

  // 5. Build DAG of representatives (using min IDs)
  dag.clear();
  for (auto &[a, b] : inequalities) {
    Id ra = find(a); // returns the min representative
    Id rb = find(b);
    if (ra != rb) {
      auto &vec = dag[ra];
      if (std::find(vec.begin(), vec.end(), rb) == vec.end())
        vec.push_back(rb);
    }
  }

  dirty = false;
}

Id PartialOrderSolver::find(Id x) {
  if (parent.find(x) == parent.end()) {
    parent[x] = x; // isolated node is its own smallest rep
  }
  if (parent[x] != x) {
    parent[x] = find(parent[x]);
  }
  return parent[x];
}

std::optional<Term> PartialOrderSolver::findopt(const Id &x) {
  if (parent.find(x) == parent.end()) {
    return std::nullopt;
  } else {
    return find(x);
  }
}

bool PartialOrderSolver::reachable(Id from, Id to) {
  std::unordered_set<Id> visited;
  std::function<bool(Id)> dfs = [&](Id u) {
    if (u == to)
      return true;
    visited.insert(u);
    auto it = dag.find(u);
    if (it == dag.end())
      return false;
    for (Id v : it->second) {
      if (!visited.count(v) && dfs(v))
        return true;
    }
    return false;
  };
  return dfs(from);
}

bool PartialOrderSolver::addLessOrEqual(Id a, Id b) {
  if (isLessOrEqual(a, b))
    return false;
  inequalities.emplace_back(a, b);
  dirty = true;
  return true;
}

bool PartialOrderSolver::isEqual(Id a, Id b) {
  rebuild();
  return find(a) == find(b);
}

bool PartialOrderSolver::isLessOrEqual(Id a, Id b) {
  rebuild();
  Id ra = find(a);
  Id rb = find(b);
  if (ra == rb)
    return true;
  return reachable(ra, rb);
}

} // namespace solvers

std::ostream &operator<<(std::ostream &os, const solvers::Term &term) {
  std::visit(
      util::overloaded{
          [&](auto &t) { os << t; },
      },
      term
  );

  return os;
}