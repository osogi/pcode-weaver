#include "parse/ast_print.hh"

#include <ostream>

namespace ast::print {

static void id(std::ostream &os, const Id &x) {
  // choose one; depends on your Id semantics
  os << x.getName(); // or: os << x.getName() << "#" << x.getNum();
}

static void varnodeVar(std::ostream &os, const VarnodeVar &v) { id(os, v.id); }
static void varnodeConst(std::ostream &os, const VarnodeConst &c) {
  os << "#" << c.value;
};
static void pnodeVar(std::ostream &os, const PnodeVar &p) { id(os, p.id); }
static void bbVar(std::ostream &os, const BasicBlockVar &b) { id(os, b.id); }

template <class T>
static void
box(std::ostream &os, const Box<T> &b, void (*fn)(std::ostream &, const T &)) {
  if (!b) {
    os << "<null>";
    return;
  }
  fn(os, *b);
}

static void size(std::ostream &os, const Size &sz) {
  std::visit(
      util::overloaded{
          [&](const Id &ident) { id(os, ident); },
          [&](ghidra::int4 s) { os << s; }
      },
      sz
  );
}

static void varnodeType(std::ostream &os, const VarnodeType &vt) {
  os << "(";
  size(os, vt.size);
  os << ", ";
  bbVar(os, vt.declarationBB);
  os << ")";
}

static void inVarnodeConditionsArray(
    std::ostream &os, const InVarnodeConditionsArray &cnds
) {
  os << "[";
  if (cnds.array.size() > 0) {
    auto it = cnds.array.begin();
    size(os, it->size);
    ++it;

    for (; it != cnds.array.end(); ++it) {
      os << ", ";
      size(os, it->size);
    }
  }
  os << "]";
}

static void
inVarnodeConditions(std::ostream &os, const InVarnodeConditions &cnds) {
  std::visit(
      util::overloaded{
          [&](const InVarnodeConditionsArray &arr) {
            inVarnodeConditionsArray(os, arr);
          },
          [&](const InVarnodeConditionsSpecial &) { os << "*"; }
      },
      cnds
  );
}

static void outVarnodeConditionDefault(
    std::ostream &os, const OutVarnodeConditionDefault &cnd
) {
  size(os, cnd.size);
}

static void
outVarnodeCondition(std::ostream &os, const OutVarnodeCondition &cnd) {
  std::visit(
      util::overloaded{
          [&](const OutVarnodeConditionDefault &cndDef) {
            outVarnodeConditionDefault(os, cndDef);
          },
          [&](const OutVarnodeConditionNoOutOr &cndOr) {
            os << "NoOurOr(";
            outVarnodeConditionDefault(os, cndOr.alt);
            os << ")";
          },
          [&](const OutVarnodeConditionNoOut &) { os << "NoOut"; },

      },
      cnd
  );
}

static void opTypeScheme(std::ostream &os, const OpTypeScheme &opt) {
  os << "(";
  inVarnodeConditions(os, opt.inVarnodeConds);
  os << ", ";
  outVarnodeCondition(os, opt.outVarnodeCond);
  os << ", " << opt.isMultiequal << ")";
}

static void opType(std::ostream &os, const OpType &opt) {
  os << opt.opName;
  // opTypeScheme(os, opt.scheme);
}

static void pnodeType(std::ostream &os, const PnodeType &pt) {
  os << "(";
  opType(os, pt.optype);
  os << ", ";
  bbVar(os, pt.bb);
  os << ")";
}

/* ----- forward print functions for recursion ----- */
static void varnodePattern(std::ostream &, const VarnodePattern &);
static void pnodePattern(std::ostream &, const PnodePattern &);
static void bbPattern(std::ostream &, const BasicBlockPattern &);
static void varnodeAction(std::ostream &, const VarnodeAction &);
static void pnodeAction(std::ostream &, const PnodeAction &);
static void rulePattern(std::ostream &, const RulePattern &);
static void ruleAction(std::ostream &, const RuleAction &);

static void varnodeTerm(std::ostream &os, const VarnodeTerm &t) {
  std::visit(
      util::overloaded{
          [&](const VarnodeVar &v) { varnodeVar(os, v); },
          [&](const VarnodeVarWithType &vt) {
            varnodeVar(os, vt.var);
            varnodeType(os, vt.vntype);
          },
          [&](const VarnodeEmpty &) { os << "EMPTY"; },
          [&](const VarnodeConst &c) { varnodeConst(os, c); }
      },
      t
  );
}

static void pnodeTerm(std::ostream &os, const PnodeTerm &t) {
  std::visit(
      util::overloaded{
          [&](const PnodeVar &p) { pnodeVar(os, p); },
          [&](const PnodeVarWithType &pt) {
            pnodeVar(os, pt.var);
            pnodeType(os, pt.ptype);
          },
          [&](const PnodeEmpty &) { os << "EMPTY"; }
      },
      t
  );
}

/* ----- patterns ----- */
static void varnodeDefedBy(std::ostream &os, const VarnodeDefedBy &n) {
  pnodePattern(os, n.pp);
  os << " -> ";
  varnodeTerm(os, n.v);
}

static void
pnodeThatTakeAsNthArg(std::ostream &os, const PnodeThatTakeAsNthArg &n) {
  varnodePattern(os, n.vp);
  os << " ->(" << n.num << ") ";
  pnodeTerm(os, n.p);
}

// static void
// pnodeThatTakeAsSomeArg(std::ostream &os, const PnodeThatTakeAsSomeArg &n) {
//   varnodePattern(os, n.vp);
//   os << " -> ";
//   pnodeTerm(os, n.p);
// }

static void bbDominatedBy(std::ostream &os, const BasicBlockDominatedBy &n) {
  bbPattern(os, n.bbp);
  os << " <= ";
  bbVar(os, n.bb);
}

static void varnodePattern(std::ostream &os, const VarnodePattern &vp) {
  std::visit(
      util::overloaded{
          [&](const VarnodeTerm &t) { varnodeTerm(os, t); },
          [&](const Box<VarnodeDefedBy> &b) { box(os, b, varnodeDefedBy); }
      },
      vp
  );
}

static void pnodePattern(std::ostream &os, const PnodePattern &pp) {
  std::visit(
      util::overloaded{
          [&](const PnodeTerm &p) { pnodeTerm(os, p); },
          [&](const Box<PnodeThatTakeAsNthArg> &b) {
            box(os, b, pnodeThatTakeAsNthArg);
          },
          // [&](const Box<PnodeThatTakeAsSomeArg> &b) {
          //   box(os, b, pnodeThatTakeAsSomeArg);
          // }
      },
      pp
  );
}

static void bbPattern(std::ostream &os, const BasicBlockPattern &bbp) {
  std::visit(
      util::overloaded{
          [&](const BasicBlockVar &b) { bbVar(os, b); },
          [&](const Box<BasicBlockDominatedBy> &b) {
            box(os, b, bbDominatedBy);
          }
      },
      bbp
  );
}

static void rulePattern(std::ostream &os, const RulePattern &rp) {
  std::visit(
      util::overloaded{
          [&](const VarnodePattern &x) { varnodePattern(os, x); },
          [&](const PnodePattern &x) { pnodePattern(os, x); },
          [&](const BasicBlockPattern &x) { bbPattern(os, x); }
      },
      rp
  );
}

/* ----- actions  ----- */
static void varnodeActionTerm(std::ostream &os, const VarnodeActionTerm &t) {
  std::visit(
      util::overloaded{
          [&](const VarnodeVar &v) { varnodeVar(os, v); },
          [&](const VarnodeEmpty &) { os << "EMPTY"; },
          [&](const VarnodeSpecSize &n) {
            varnodeVar(os, n.newVar);
            os << "(";
            size(os, n.size);
            os << ")";
          },
          [&](const VarnodeConst &c) { varnodeConst(os, c); }
      },
      t
  );
}

static void pnodeActionTerm(std::ostream &os, const PnodeActionTerm &t) {
  std::visit(
      util::overloaded{
          [&](const PnodeVar &p) { pnodeVar(os, p); },
          [&](const PnodeSpecTypeAndLoc &n) {
            pnodeVar(os, n.newVar);
            os << "(";
            opType(os, n.opType);
            os << (n.isInsertBefore ? " BEFORE " : " AFTER ");
            pnodeVar(os, n.oldVar);
            os << ")";
          }
      },
      t
  );
}

static void
varnodeSetAsPnodeOut(std::ostream &os, const VarnodeSetAsPnodeOut &n) {
  // pnode_action ->> varnode
  pnodeAction(os, n.pa);
  os << " ->> ";
  varnodeActionTerm(os, n.v);
}

static void pnodeSetNthArg(std::ostream &os, const PnodeSetNthArg &n) {
  // varnode_action ->>(N) pnode
  varnodeAction(os, n.va);
  os << " ->>(" << n.num << ") ";
  pnodeActionTerm(os, n.p);
}

static void
emptyActionDeletePnode(std::ostream &os, const EmptyActionDeletePnode &act) {
  os << "DELETE ";
  pnodeVar(os, act.targetPnode);
}

static void varnodeAction(std::ostream &os, const VarnodeAction &a) {
  std::visit(
      util::overloaded{
          [&](const VarnodeActionTerm &t) { varnodeActionTerm(os, t); },
          [&](const Box<VarnodeSetAsPnodeOut> &b) {
            box(os, b, varnodeSetAsPnodeOut);
          }
      },
      a
  );
}

static void pnodeAction(std::ostream &os, const PnodeAction &a) {
  std::visit(
      util::overloaded{
          [&](const PnodeActionTerm &t) { pnodeActionTerm(os, t); },
          [&](const Box<PnodeSetNthArg> &b) { box(os, b, pnodeSetNthArg); }
      },
      a
  );
}

static void emptyAction(std::ostream &os, const EmptyAction &ea) {
  std::visit(
      util::overloaded{
          [&](const EmptyActionDeletePnode &x) {
            emptyActionDeletePnode(os, x);
          },
      },
      ea
  );
}

static void ruleAction(std::ostream &os, const RuleAction &ra) {
  std::visit(
      util::overloaded{
          [&](const VarnodeAction &x) { varnodeAction(os, x); },
          [&](const PnodeAction &x) { pnodeAction(os, x); },
          [&](const EmptyAction &x) { emptyAction(os, x); },
      },
      ra
  );
}

/* ----- public entry ----- */
void rule(std::ostream &os, const Rule &r) {
  for (size_t i = 0; i < r.patterns.size(); ++i) {
    if (i)
      os << ";\n";
    rulePattern(os, r.patterns[i]);
  }
  os << "\n--\n";
  for (size_t i = 0; i < r.actions.size(); ++i) {
    if (i)
      os << ";\n";
    ruleAction(os, r.actions[i]);
  }
}

} // namespace ast::print

namespace ast {
std::ostream &operator<<(std::ostream &os, const Rule &r) {
  ast::print::rule(os, r);
  return os;
}

std::ostream &operator<<(std::ostream &os, const Id &id) {
  ast::print::id(os, id);
  return os;
}
} // namespace ast
