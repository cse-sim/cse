# Note: Edits to hooks do not get captured by MkDocs' built-in reloading mechanism.
# If you change the hooks, you must manually stop and restart the server.
import re
import yaml

from pathlib import Path
from slugify import slugify


# # TODO: Get probes from CNRECS.DEF
probes_yml = Path("..", "util", "probes.yml")


def on_config(config, **kwargs):
    with open(probes_yml, "r") as f:
        temp = yaml.safe_load(f)

    for key, value in temp.items():
        fields_as_csv = "\n".join(
            [
                f"{field.get(':name', '')}, {field.get(':input', '')}, {field.get(':runtime', '')}, {field.get(':type', '')}, {field.get(':variability', '')}, {field.get(':description', '')}"
                for field in value[":fields"]
            ]
        )

        file_path = Path("docs", "probe-definitions", f"{key}.md")
        try:
            if file_path.exists():
                return

            with open(file_path, "w") as f:
                f.write(
                    "[](){ #p_"
                    + key.lower()
                    + " }\n\n"
                    + "{"
                    + '{{ csv_table("Name, Input?, Runtime?, Type, Variability, Description\n{0}", True) }}'.format(
                        fields_as_csv
                    )
                    + "}"
                )
        except Exception:
            print(f"unable to dump key: {key}")

    return config


AUTOREF_CONFIG = {
    "exclude_files": [
        "docs/output-reports.md",
    ],
    "exclude_headings": [
        "Introduction",
        "Units",
    ],
}


docs_dir = Path(__file__).parent / "docs"
heading_re = re.compile(r"^(#{1,6})\s+(.*)$")
slug_levels = {}

# Prepopulate any custom keywords (key) to any custom autoref slugs (value)
text_to_slug = {
    "END": slugify("END and ENDxxxx"),
    "ENDxxxx": slugify("END and ENDxxxx"),
    "FROZEN": slugify("FREEZE"),
    "TYPE": slugify("DEFTYPE"),
    # TODO: Eliminate "clause" from heading? Then, we don't need to prepopulate.
    "LIKE": slugify("LIKE clause"),
    "COPY": slugify("COPY clause"),
    "USETYPE": slugify("USETYPE clause"),
    # TODO: Handling stacked headers?
    # "dcTauB": slugify("dcTauB, dcTauD"),
    # "dcTauD": slugify("dcTauB, dcTauD"),
}

# Walk all docs *.md files to find headings
for md in docs_dir.rglob("*.md"):
    rel = md.relative_to(Path(__file__).parent).as_posix()
    if rel in AUTOREF_CONFIG["exclude_files"]:
        continue

    text = md.read_text(encoding="utf-8")
    text = re.sub(r"<!--[\s\S]*?-->", "", text)

    for line in text.splitlines():
        match = heading_re.match(line)
        if not match:
            continue

        heading = match.group(2).strip()
        if heading in AUTOREF_CONFIG["exclude_headings"]:
            continue

        slug = slugify(heading)
        text_to_slug[heading] = slug
        slug_levels[slug] = len(match.group(1))

# Sort and compile all headings found by walking docs *.md files.
escaped = sorted((re.escape(text) for text in text_to_slug), key=len, reverse=True)
combined_pattern = re.compile(
    rf"""
    (?<![`])          # not in inline code
    (?<!\])           # not already in [...]
    \b
({"|".join(escaped)})       # group(1) = base heading
    # TODO: Also allow trailing (s)?
    # TODO: Do we even need to capture this? Maybe we can simply remove word boundary, then only the keyword has that anchor?
    # (i.e., then the link [ABCs][abc] would be updated to [ABC][abc]s, so the ending 's' is not part of the link.)
    ((?:s|ing|))?           # group(2) = optional trailing 's' or 'ing'
    \b
    (?!\])            # not followed by ]
    (?!\()            # not followed by (
    """,
    re.VERBOSE | re.MULTILINE,
)


_splitter = re.compile(
    r"("
    r"```[\s\S]*?```"
    r"|`[^`]*`"
    r"|(?:^(?: {4}|\t).*(?:\n|$))+"
    r"|\[[^\]]+\]\([^)]+\)"
    r"|\[[^\]]+\]\[[^\]]+\]"
    r")",
    re.MULTILINE,
)


def on_page_markdown(markdown, page, config, files):
    chunks = _splitter.split(markdown)
    out = []
    current_path = []
    for chunk in chunks:
        if (
            chunk.startswith("```")
            or chunk.startswith("`")
            or (chunk.startswith("    ") or chunk.startswith("\t"))
            or (chunk.startswith("[") and ("](" in chunk or "][" in chunk))
        ):
            out.append(chunk)
            continue

        processed = []
        for line in chunk.splitlines(keepends=True):
            m = heading_re.match(line)
            if m:
                heading = m.group(2).strip()
                slug = text_to_slug.get(heading, slugify(heading))
                level = slug_levels.get(slug, len(m.group(1)))
                current_path = current_path[: level - 1] + [slug]
                processed.append(line)
                continue

            def replace(m):
                text = m.group(1) + (m.group(2) or "")
                slug = text_to_slug.get(m.group(1), slugify(m.group(1)))
                if slug in current_path:
                    return text
                return f"[{text}][{slug}]"

            new_line = combined_pattern.sub(replace, line)
            processed.append(new_line)
        out.append("".join(processed))
    return "".join(out)
