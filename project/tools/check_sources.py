import re, os, glob, sys

ROOT = r"C:\Users\lenovo\Documents\deepseek-harness\default-workspace\nyx9"
os.chdir(ROOT)

headers = sorted(glob.glob("*.h"))
sources = sorted(glob.glob("*.cpp"))
problems = []

def strip_comments_strings(text):
    out = []
    i, n = 0, len(text)
    state = "code"
    while i < n:
        c = text[i]
        if state == "code":
            if c == '/' and i + 1 < n and text[i+1] == '/':
                state = "line"; i += 2; continue
            if c == '/' and i + 1 < n and text[i+1] == '*':
                state = "block"; i += 2; continue
            if c == '"':
                state = "str"; out.append(' '); i += 1; continue
            if c == "'":
                state = "chr"; out.append(' '); i += 1; continue
            out.append(c); i += 1
        elif state == "line":
            if c == '\n':
                state = "code"; out.append('\n')
            i += 1
        elif state == "block":
            if c == '*' and i + 1 < n and text[i+1] == '/':
                state = "code"; i += 2; continue
            if c == '\n': out.append('\n')
            i += 1
        elif state in ("str", "chr"):
            if c == '\\': i += 2; continue
            if (state == "str" and c == '"') or (state == "chr" and c == "'"):
                state = "code"
            i += 1
    return ''.join(out)

def balance(name, text):
    pairs = {'{':'}', '(':')', '[':']'}
    stack = []
    for i, c in enumerate(text):
        if c in pairs:
            stack.append((c, i))
        elif c in pairs.values():
            if not stack:
                problems.append(f"{name}: stray '{c}' at offset {i}")
                return
            op, _ = stack.pop()
            if pairs[op] != c:
                problems.append(f"{name}: mismatch '{op}' vs '{c}' at offset {i}")
                return
    if stack:
        line = text[:stack[-1][1]].count('\n') + 1
        problems.append(f"{name}: unclosed '{stack[-1][0]}' from line {line}")

for f in headers + sources:
    raw = open(f, encoding="utf-8").read()
    balance(f, strip_comments_strings(raw))
    if '\t' in raw:
        problems.append(f"{f}: contains tab (Makefile only)")

# include guards
for f in headers:
    raw = open(f, encoding="utf-8").read()
    if "#ifndef" not in raw or "#define" not in raw or "#endif" not in raw:
        problems.append(f"{f}: missing include guard")

# includes resolve
for f in headers + sources:
    for line in open(f, encoding="utf-8"):
        m = re.match(r'\s*#include\s+"([^"]+)"', line)
        if m and not os.path.exists(m.group(1)):
            problems.append(f"{f}: missing include {m.group(1)}")
        m2 = re.match(r'\s*#include\s+<([^>]+)>', line)
        if m2:
            known = {'iostream','fstream','sstream','string','vector','map','set','stack','queue','unordered_map','istream','ostream','memory','random','cstddef','cstdlib','cctype','ctime','utility','algorithm','limits','cassert','iterator','new',
                     'memory','random','cstddef','cstdlib','cctype','ctime','utility','algorithm'}
            if m2.group(1) not in known:
                problems.append(f"{f}: unusual system include <{m2.group(1)}>")

def class_bodies(text):
    """Yield (class_name, body_text) for `class X ... { ... };`."""
    res = []
    for m in re.finditer(r'\b(?:class|struct)\s+(\w+)\s*(?::[^{;]*)?\{', text):
        name = m.group(1)
        i = m.end() - 1
        depth = 0
        start = i + 1
        while i < len(text):
            if text[i] == '{': depth += 1
            elif text[i] == '}':
                depth -= 1
                if depth == 0:
                    res.append((name, text[start:i]))
                    break
            i += 1
    return res

def strip_inline_bodies(body):
    out = []
    depth = 0
    for c in body:
        if c == '{':
            depth += 1
            out.append(' ')   # inline definition: drop body
        elif c == '}':
            depth -= 1
            out.append(' ')
        elif depth == 0:
            out.append(c)
    return ''.join(out)

declared = {}       # class -> {method names}
defined = {}        # class -> {method names}

declared = {}       # class -> {method names}
defined = {}        # class -> {method names}
pure = {}           # class -> {pure virtual names}

for h in headers:
    text = strip_comments_strings(open(h, encoding="utf-8").read())
    for cname, body in class_bodies(text):
        flat = strip_inline_bodies(body)
        for stmt in flat.split(';'):
            stmt = ' '.join(stmt.split())
            if '(' not in stmt:
                continue
            # method name = identifier immediately before the first '('
            m = re.search(r'([~A-Za-z_]\w*)\s*\(', stmt)
            if not m:
                continue
            name = m.group(1)
            if name in ('return','if','while','for','switch','sizeof','throw'):
                continue
            declared.setdefault(cname, set()).add(name)
            if '= 0' in stmt:
                pure.setdefault(cname, set()).add(name)

for c in sources:
    text = strip_comments_strings(open(c, encoding="utf-8").read())
    for m in re.finditer(r'\b(\w+)::([~A-Za-z_]\w*)\s*\(', text):
        cname, mname = m.group(1), m.group(2)
        if cname == 'std':
            continue
        defined.setdefault(cname, set()).add(mname)

for cname, names in sorted(declared.items()):
    have = defined.get(cname, set())
    missing = sorted(names - have - pure.get(cname, set()))
    if missing:
        problems.append(f"{cname}: declared but never defined -> {missing}")
    extra = sorted(have - names)
    if extra:
        problems.append(f"{cname}: defined but not declared -> {extra}")

for cname in sorted(defined):
    if cname not in declared:
        problems.append(f"{cname}: definitions found but no class declaration")

# C++14/17 feature scan
banned = [
    (r'\bmake_unique\b', 'std::make_unique (C++14)'),
    (r'\bstd::optional\b', 'std::optional (C++17)'),
    (r'\bstd::variant\b', 'std::variant (C++17)'),
    (r'\bstd::string_view\b', 'string_view (C++17)'),
    (r'\bstd::filesystem\b', 'filesystem (C++17)'),
    (r'\bstd::byte\b', 'std::byte (C++17)'),
    (r'\bauto\s+\w+\s*\([^)]*\)\s*\{', 'auto return deduction (C++14)'),
    (r"\b\d+'\d+", 'digit separator (C++14)'),
    (r'\b0b[01]+', 'binary literal (C++14)'),
    (r'\[\[', 'attributes (C++17 style)'),
]
for f in headers + sources:
    raw = open(f, encoding="utf-8").read()
    for pattern, label in banned:
        if re.search(pattern, raw):
            problems.append(f"{f}: uses {label}")

print(f"files: {len(headers)} headers, {len(sources)} sources")
print(f"classes checked: {len(declared)}")
if problems:
    print("\nPROBLEMS:")
    for p in problems:
        print("  -", p)
else:
    print("No structural problems found.")
