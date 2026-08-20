#!/usr/bin/env python3
"""Generate the new Halide IR-route mutation operators from libHalide's symbols.

Mappings are derived from the symbol table rather than written by hand: a swap
op1->op2 is emitted only for parameter encodings where BOTH overloads exist, so
no mapping can name a symbol libHalide does not define.

That constraint is not cosmetic. HalideReplacement falls back to
Module::getOrInsertFunction when the target is absent, and the linker resolves
by name alone -- so a mapping across two different parameter encodings (say a
by-value Expr to a const Expr&) would produce a call that links successfully and
then passes arguments under the wrong ABI. Requiring an identical suffix makes
that unrepresentable.
"""
import re, sys, collections, json

SYMS = sys.argv[1]
OUT = sys.argv[2]

OPCODES = ["pl","mi","ml","dv","rm","lt","gt","le","ge","eq","ne","aa","oo",
           "an","or","eo","ls","rs","pL","mI","mL","dV","co","ng","nt"]
ALT = "|".join(sorted(OPCODES, key=len, reverse=True))
FREE = re.compile(r"^(_ZN6Halide)(" + ALT + r")(E.*)$")

forms = collections.defaultdict(set)


def decompose(sym):
    m = FREE.match(sym)
    if m:
        return m.groups()
    m = re.match(r"^_ZN6Halide(\d+)(.*)$", sym)
    if not m:
        return None
    digits, rest = m.groups()
    n = int(digits)
    if len(rest) < n:
        return None
    name, tail = rest[:n], rest[n:]
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
        return None
    m2 = re.match(r"^(" + ALT + r")(E.*)$", tail)
    if not m2:
        return None
    return ("_ZN6Halide" + digits + name, m2.group(1), m2.group(2))


symbols = {l.strip() for l in open(SYMS) if l.strip()}
for s in symbols:
    d = decompose(s)
    if d:
        forms[(d[0], d[2])].add(d[1])


def op_pairs(a, b):
    out = []
    for (prefix, suffix), ops in sorted(forms.items()):
        if a in ops and b in ops:
            out.append((prefix + a + suffix, prefix + b + suffix))
    return out


def name_pairs(a, b):
    """For plain functions like min/max, keyed on the length-prefixed name."""
    out = []
    for suffix in ("ENS_4ExprES0_", "ENS_4ExprEi", "EiNS_4ExprE"):
        fa = f"_ZN6Halide{len(a)}{a}{suffix}"
        fb = f"_ZN6Halide{len(b)}{b}{suffix}"
        if fa in symbols and fb in symbols:
            out.append((fa, fb))
    return out


def camel(s):
    return "".join(p.capitalize() for p in s.split("_"))


# id_suffix, from_op, to_op, human description
OPS = [
    # relational
    ("lt_to_ge", "lt", "ge", "< to >="),
    ("lt_to_le", "lt", "le", "< to <="),
    ("le_to_gt", "le", "gt", "<= to >"),
    ("le_to_lt", "le", "lt", "<= to <"),
    ("gt_to_ge", "gt", "ge", "> to >="),
    ("gt_to_le", "gt", "le", "> to <="),
    ("ge_to_gt", "ge", "gt", ">= to >"),
    ("ge_to_lt", "ge", "lt", ">= to <"),
    ("eq_to_ne", "eq", "ne", "== to !="),
    ("ne_to_eq", "ne", "eq", "!= to =="),
    # logical
    ("logical_and_to_or", "aa", "oo", "&& to ||"),
    ("logical_or_to_and", "oo", "aa", "|| to &&"),
    # bitwise
    ("and_to_or", "an", "or", "& to |"),
    ("or_to_and", "or", "an", "| to &"),
    ("xor_to_or", "eo", "or", "^ to |"),
    ("lshift_to_rshift", "ls", "rs", "<< to >>"),
    ("rshift_to_lshift", "rs", "ls", ">> to <<"),
    # arithmetic
    ("rem_to_div", "rm", "dv", "% to /"),
    # compound assignment
    ("add_assign_to_sub_assign", "pL", "mI", "+= to -="),
    ("sub_assign_to_add_assign", "mI", "pL", "-= to +="),
    ("mul_assign_to_div_assign", "mL", "dV", "*= to /="),
    ("div_assign_to_mul_assign", "dV", "mL", "/= to *="),
    # unary -- all three share the signature Expr(Expr)
    ("not_to_negate", "nt", "ng", "! to unary -"),
    ("not_to_bitwise_not", "nt", "co", "! to ~"),
    ("negate_to_not", "ng", "nt", "unary - to !"),
    ("negate_to_bitwise_not", "ng", "co", "unary - to ~"),
    ("bitwise_not_to_not", "co", "nt", "~ to !"),
    ("bitwise_not_to_negate", "co", "ng", "~ to unary -"),
]

NAMED = [
    ("min_to_max", "min", "max", "min to max"),
    ("max_to_min", "max", "min", "max to min"),
]

table = []
for ident, a, b, desc in OPS:
    table.append((ident, desc, op_pairs(a, b)))
for ident, a, b, desc in NAMED:
    table.append((ident, desc, name_pairs(a, b)))

live = [t for t in table if t[2]]
dead = [t for t in table if not t[2]]

with open(OUT, "w") as fh:
    json.dump([{"id": i, "desc": d, "mappings": m} for i, d, m in live], fh, indent=1)

print(f"{len(live)} operators with at least one safe mapping, "
      f"{sum(len(m) for _, _, m in live)} mappings total")
if dead:
    print("NO SAFE MAPPING (not emitted):")
    for i, d, _ in dead:
        print(f"  {i:<30} {d}")
print()
print(f"{'operator':<30}{'swap':<16}{'mappings':>9}")
print("-" * 56)
for i, d, m in live:
    print(f"{i:<30}{d:<16}{len(m):>9}")
