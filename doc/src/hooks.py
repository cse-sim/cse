# Note: Edits to hooks do not get captured by MkDocs' built-in reloading mechanism.
# If you change the hooks, you must manually stop and restart the server.
import re
import yaml

from pathlib import Path
from slugify import slugify
from bs4 import BeautifulSoup


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
    "exclude_subheadings_of": ["Built-in Functions"],
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

    excluded_level = None
    for line in text.splitlines():
        match = heading_re.match(line)
        if not match:
            continue

        level = len(match.group(1))
        heading = match.group(2).strip()

        if heading in AUTOREF_CONFIG["exclude_headings"]:
            continue

        if excluded_level:
            if level > excluded_level:
                continue
            else:
                excluded_level = None

        if heading in AUTOREF_CONFIG["exclude_subheadings_of"]:
            excluded_level = level

        slug = slugify(heading)
        text_to_slug[heading] = slug
        slug_levels[slug] = level

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


DIRECTIVE = "@nested-dl"
DIRECTIVE_REGEX = re.compile(r"^\s*@nested-dl\s*$")


def on_page_content(html, page, config, files):
    soup = BeautifulSoup(html, "html.parser")
    directive_paragraphs = soup.find_all("p")

    for directive_p in directive_paragraphs:
        if directive_p.string and DIRECTIVE_REGEX.match(directive_p.string):
            start_heading = directive_p.find_next(
                lambda tag: tag.name in {"h1", "h2", "h3", "h4", "h5", "h6"}
            )
            if not start_heading:
                continue

            current_level = int(start_heading.name[1])

            directive_p.decompose()

            sibling = start_heading.find_next_sibling()
            while sibling:
                if sibling.name in {"h1", "h2", "h3", "h4", "h5", "h6"}:
                    sibling_level = int(sibling.name[1])
                    if sibling_level <= current_level:
                        break  # Stop at heading of same or lower level

                if sibling.name == "dl":
                    existing = sibling.get("class", [])
                    sibling["class"] = existing + ["nested-dl"]

                sibling = sibling.find_next_sibling()

    return str(soup)
