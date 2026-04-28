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

Errorable<void> BBSolver::unite(const BBTerm &x, const BBTerm &y) {
  BBTerm root_x = find(x);
  BBTerm root_y = find(y);

  // Already in same set
  if (root_x == root_y) {
    return {};
  }

  BBTerm newRep = root_x;
  BBTerm oldRep = root_y;
  if (root_y < root_x) {
    newRep = root_y;
    oldRep = root_x;
  }

  parent[oldRep] = newRep;
  mergeEdgesOnUnion(oldRep, newRep);

  return {};
}
void BBSolver::mergeEdgesOnUnion(const BBTerm &oldRep, const BBTerm &newRep) {
  if (oldRep == newRep)
    return;

  // Merge lessEdges[oldRep] into lessEdges[newRep]
  if (lessEdges.contains(oldRep)) {
    for (auto &target : lessEdges[oldRep]) {
      if (target != newRep) {
        lessEdges[newRep].insert(target);
      }
      // fix reverse edges
      greaterEdges[target].erase(oldRep);
      if (target != newRep) {
        greaterEdges[target].insert(newRep);
      }
    }
    lessEdges.erase(oldRep);
  }

  // Merge greaterEdges[oldRep] into greaterEdges[newRep]
  if (greaterEdges.contains(oldRep)) {
    for (auto &source : greaterEdges[oldRep]) {
      if (source != newRep) {
        greaterEdges[newRep].insert(source);
      }
      // fix forward edges
      lessEdges[source].erase(oldRep);
      if (source != newRep) {
        lessEdges[source].insert(newRep);
      }
    }
    greaterEdges.erase(oldRep);
  }

  // Remove self-loops
  lessEdges[newRep].erase(newRep);
  greaterEdges[newRep].erase(newRep);
}

// BFS/DFS reachability in the DAG (following lessEdges)
// Returns true if `from` can reach `to` via ≤ edges
bool BBSolver::canReach(const BBTerm &from, const BBTerm &to) {
  if (from == to)
    return true;

  std::unordered_set<BBTerm> visited;
  std::queue<BBTerm> queue;
  queue.push(from);
  visited.insert(from);

  while (!queue.empty()) {
    auto curr = queue.front();
    queue.pop();

    if (!lessEdges.contains(curr))
      continue;

    for (const auto &next : lessEdges.at(curr)) {
      BBTerm repNext = find(next); // normalize to rep
      if (repNext == to)
        return true;
      if (!visited.contains(repNext)) {
        visited.insert(repNext);
        queue.push(repNext);
      }
    }
  }
  return false;
}

// Collapses all nodes 'v' such that (source <= v <= target)
void BBSolver::collapsePath(const BBTerm &source, const BBTerm &target) {
  BBTerm rTarget = find(target);
  BBTerm rSource = find(source);

  std::vector<BBTerm> cycleNodes;
  std::queue<BBTerm> q;
  std::unordered_set<BBTerm> visited;

  q.push(rSource);
  visited.insert(rSource);

  while (!q.empty()) {
    BBTerm curr = q.front();
    q.pop();

    // If this node can reach the target, it's part of the cycle/path
    if (canReach(curr, rTarget)) {
      cycleNodes.push_back(curr);
    }

    if (lessEdges.contains(curr)) {
      for (auto &next : lessEdges.at(curr)) {
        BBTerm rNext = find(next);
        if (!visited.contains(rNext)) {
          visited.insert(rNext);
          q.push(rNext);
        }
      }
    }
  }

  for (BBTerm node : cycleNodes) {
    unite(rTarget, node);
    rTarget = find(rTarget);
  }
}

/// @brief Add inequality: a ≤ b
/// @return true if something changed, false if already known
Errorable<bool> BBSolver::addLessOrEqual(BBTerm a, BBTerm b) {
  BBTerm ra = find(a);
  BBTerm rb = find(b);

  if (ra == rb || canReach(ra, rb))
    return false;

  if (canReach(rb, ra)) {
    // Cycle detected! Collapse everything between rb and ra
    collapsePath(rb, ra);
    return true;
  }

  // Normal case
  lessEdges[ra].insert(rb);
  greaterEdges[rb].insert(ra);
  return true;
}

/// @brief Check if a ≤ b (directly or transitively via equalities)
bool BBSolver::isLessOrEqual(BBTerm a, BBTerm b) {
  BBTerm ra = find(a);
  BBTerm rb = find(b);

  // Equal terms satisfy ≤ (reflexivity)
  if (ra == rb)
    return true;

  // Check reachability in DAG
  return canReach(ra, rb);
}

Errorable<bool> BBSolver::addEquation(const BBTerm &left, const BBTerm &right) {
  BBTerm rl = find(left);
  BBTerm rr = find(right);

  if (rl == rr)
    return false;

  // 1. If there's a path rl -> ... -> rr, collapse it
  if (canReach(rl, rr)) {
    // This is exactly the cycle logic from addLessOrEqual!
    collapsePath(rl, rr); // Collapses everything on path from rl to rr
  }
  // 2. If there's a path rr -> ... -> rl, collapse it
  else if (canReach(rr, rl)) {
    collapsePath(rr, rl); // Collapses everything on path from rr to rl
  }

  // 3. Finally, ensure the two actual endpoints are unified
  unite(rl, rr);

  return true;
}

std::ostream &operator<<(std::ostream &os, const SizeTerm &term) {
  return printVariant(os, term);
}

std::ostream &operator<<(std::ostream &os, const BBTerm &term) {
  return printVariant(os, term);
}

} // namespace solvers
