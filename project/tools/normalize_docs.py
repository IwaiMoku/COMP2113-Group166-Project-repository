import glob

# Normalise typographic punctuation in the markdown documents to plain ASCII,
# so the files are safe in any editor and on the Linux server.
REPLACEMENTS = [
    ("\u2014", "-"),
    ("\u2013", "-"),
    ("\u00a7", "section "),
    ("\u2018", "'"),
    ("\u2019", "'"),
    ("\u201c", '"'),
    ("\u201d", '"'),
    ("\u00b7", "-"),
    ("\u2026", "..."),
]

for path in glob.glob("*.md"):
    with open(path, encoding="utf-8") as handle:
        text = handle.read()
    updated = text
    for old, new in REPLACEMENTS:
        updated = updated.replace(old, new)
    if updated != text:
        with open(path, "w", encoding="utf-8", newline="") as handle:
            handle.write(updated)
        print("normalised", path)

print("---- final scan ----")
targets = (glob.glob("*.md") + glob.glob("*.cpp") + glob.glob("*.h")
           + glob.glob("data/*.txt") + glob.glob("tests/in/*.txt") + ["Makefile"])
for path in sorted(targets):
    data = open(path, "rb").read()
    bad = [byte for byte in data if byte > 127]
    print("%-26s non-ascii bytes: %d" % (path, len(bad)))
