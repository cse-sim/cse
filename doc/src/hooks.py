# Note: Edits to hooks do not get captured by MkDocs' built-in reloading mechanism.
# If you change the hooks, you must manually stop and restart the server.
import copy
import re

from bs4 import BeautifulSoup
from pathlib import Path
from slugify import slugify
from typing import Optional, TypedDict, List

from parsers.cndefns import DefinitionsParser
from parsers.cnfields import FieldsParser
from parsers.cnrecs import Field, FieldGroup, Record, RecordsParser


class RecordPrefixClass(TypedDict, total=False):
    value: str


type RecordPrefix = str | RecordPrefixClass | None


path_to_cndefns = Path(__file__).parent.parent.parent / "src" / "cndefns.h"
path_to_cnfields = Path(__file__).parent.parent.parent / "src" / "CNFIELDS.DEF"
path_to_cnrecs = Path(__file__).parent.parent.parent / "src" / "CNRECS.DEF"

# Key: alias; value: record id for an RAT in CNRECS
RECORD_ALIASES = {
    "door": "surface",
    "window": "surface",
    "export": "report",
    "exportCol": "reportCol",
    "exportFile": "reportFile",
    "DHWLoopHeater": "DHWHeater",
    "weatherNextHour": "weather",
}


FOUND_CHOICE_TYPES = set()
UNMAPPED_TYPES = set()
TYPE_MAP = {
    "FLOAT": "number",
    "FRAC": "number",
    "INT": "number",
    "POWER": "number",
    "SI": "number",
}


SHARED_DESCRIPTION = {
    "b": "Start of run",
    "e": "End of run",
    "p": "Post-calc phase of run",
}
DESCRIPTION_MAP = {
    "s": {
        "b": "Start of subhour",
        "e": "End of subhour",
        "p": "Post-calc phase of subhour",
    },
    "h": {"b": "Start of hour", "e": "End of hour", "p": "Post-calc phase of hour"},
    "mh": {"b": "Start of monthly-hourly", "e": "End of monthly-hourly"},
    "d": {"b": "Start of day", "e": "End of day", "p": "Post-calc phase of day"},
    "r": SHARED_DESCRIPTION,
    "y": SHARED_DESCRIPTION,
    "f": {"b": "Start of phase", "e": "End of phase"},
    "i": {"b": "input time"},
    "z": {"b": "constant"},
}


class SourcePaths(TypedDict):
    cndefns: Path
    cnfields: Path
    cnrecs: Path


def get_visible_data_members_by_group(group: FieldGroup):
    members = group.get("members", [])

    return list(
        filter(
            lambda m: not any([key in m for key in ["define", "declare", "hide"]]),
            members,
        )
    )


def get_resolved_probe_name(member, prefix: Optional[str] = None):
    # TODO: Fix at parser level.
    clean_name = member["name"].replace(";", "")

    return f"{prefix + '.' if prefix else ''}{clean_name}{'[index]' if 'array' in member else ''}"


def get_resolved_comment_string(comments: list[str]):
    if len(comments) == 0:
        return ""
    elif len(comments) == 1:
        return comments[0].strip()
    else:
        items = "\n".join(f"<li>{comment.strip()}</li>" for comment in comments)
        return f"<ul>\n{items}\n</ul>"


def is_visible_noname_nest_member(member):
    return (
        member.get("nest", None) is not None
        and member.get("noname", False)
        and not member.get("hide", False)
    )


def is_visible_named_nest_member(member):
    return (
        member.get("nest", None) is not None
        and not member.get("noname", False)
        and not member.get("hide", False)
    )


def attach_forward_variability_directives(member, forward_variability_directives):
    if "hide" in member:
        return None

    elif "declare" in member or "define" in member:
        return member

    elif is_visible_noname_nest_member(member):
        raise Exception("how did we get here?")

    else:
        new_variability_directives = set(member.get("variability_directives", []))
        new_variability_directives.update(forward_variability_directives)

        member["variability_directives"] = list(new_variability_directives)
        return member


class ProbeWriter:
    def __init__(self, paths: SourcePaths, destination_dir: Path):
        self.cndefns = DefinitionsParser(paths["cndefns"]).parse()
        self.cnfields = FieldsParser(paths["cnfields"]).parse()
        self.cnrecs = RecordsParser(
            paths["cnrecs"], self.cndefns, self.cnfields
        ).parse()
        self.process_cnrecs()

        self.destination_dir = destination_dir
        self.destination_dir.mkdir(parents=True, exist_ok=True)
        self.types = set()
        self.variability_combinations = set()

        # This refers to no frequency directive AFTER any forwarded/inherited
        # frequency directives are applied.
        self.members_with_no_frequency_directive = set()

    def strip_prefix(self, name: str, prefix: RecordPrefix):
        if not prefix:
            return name

        resolved_prefix = prefix if isinstance(prefix, str) else prefix["value"]

        return (
            name[len(resolved_prefix) :] if name.startswith(resolved_prefix) else name
        )

    def process_cnrecs(self):
        """Strips all record prefixes."""
        records = self.cnrecs.values()

        for record in records:
            prefix = record.get("prefix", None)

            field_groups = record.get("field_groups", [])

            for group in field_groups:
                data_members = get_visible_data_members_by_group(group)

                for member in data_members:
                    if "name" not in member:
                        raise Exception(
                            f"Could not find name for data member: {member}"
                        )

                    member["name"] = self.strip_prefix(member["name"], prefix)

    def get_field_groups_by_record(
        self, record: Record, forward_variability_directives: Optional[str] = []
    ) -> List[FieldGroup]:
        if not record:
            return []

        new_field_groups: List[FieldGroup] = []

        if record.get("baseclass"):
            baseclass_id = record.get("baseclass")
            baseclass = copy.deepcopy(self.cnrecs.get(baseclass_id))

            if not baseclass:
                raise Exception(f"Could not find baseclass: {baseclass_id}")

            new_field_groups += self.get_field_groups_by_record(
                baseclass, forward_variability_directives
            )

        field_groups = record.get("field_groups", [])
        for field_group in field_groups:
            # TODO: Do we only need to get data members?
            members = field_group.get("members")

            # The nest field groups will be injected, causing a split to current field group
            if any([is_visible_noname_nest_member(member) for member in members]):
                new_members = []
                for member in members:
                    if is_visible_noname_nest_member(member):
                        if len(new_members) > 0:
                            new_field_group = {"members": new_members}

                            if "comments" in field_group:
                                new_field_group["comments"] = field_group["comments"]

                            new_field_groups.append(new_field_group)
                            new_members = []

                        nest_record_id = member.get("nest")
                        nest_record = copy.deepcopy(self.cnrecs.get(nest_record_id))

                        # New forwarded directives include any previous forwards plus any
                        # directives attached to the current noname nest record
                        merged_variability_directives = set(
                            member.get("variability_directives", [])
                        )
                        merged_variability_directives.update(
                            forward_variability_directives
                        )

                        new_field_groups += self.get_field_groups_by_record(
                            nest_record, list(merged_variability_directives)
                        )

                    else:
                        if "hide" in member:
                            continue
                        else:
                            enriched_member = attach_forward_variability_directives(
                                member, forward_variability_directives
                            )
                            new_members.append(enriched_member)

                if len(new_members) > 0:
                    new_field_group = {"members": new_members}

                    if "comments" in field_group:
                        new_field_group["comments"] = field_group["comments"]

                    new_field_groups.append(new_field_group)

            # Normal field group, but we still need to forward any variability directives to each data member.
            else:
                new_members = []
                for member in members:
                    if "hide" in member:
                        continue
                    else:
                        enriched_member = attach_forward_variability_directives(
                            member, forward_variability_directives
                        )
                        new_members.append(enriched_member)

                new_field_group: FieldGroup = {"members": new_members}

                if "comments" in field_group:
                    new_field_group["comments"] = field_group["comments"]

                new_field_groups.append(new_field_group)

        return new_field_groups

    def get_flat_defines(self, record: Record):
        if not record:
            return []

        field_groups = self.get_field_groups_by_record(record)

        defines = []
        for field_group in field_groups:
            members = field_group.get("members", [])
            new_defines = [m for m in members if "define" in m]
            defines += new_defines

        return defines

    def get_array_length_from_define_statement(self, definition, value):
        pattern = rf"{re.escape(value)}\s+(\d+)"
        match = re.search(pattern, definition)
        if not match:
            raise Exception("could not parse the define statement for an array length.")

        return int(match.group(1))

    def get_resolved_array_length(self, member, record_id):
        if "array" not in member:
            raise Exception(
                "To get resolved array length, the data member must be an array."
            )

        try:
            return int(member["array"])
        except Exception:
            value = member["array"]

            if value in self.cndefns["kvp"].keys():
                return self.cndefns["kvp"][value]

            record = copy.deepcopy(self.cnrecs.get(record_id))
            defines = self.get_flat_defines(record)
            if len(defines) > 0:  # added to make debugging easier.
                for item in defines:
                    if value in item["define"]:
                        return self.get_array_length_from_define_statement(
                            item["define"], value
                        )

        # If we couldn't parse the value as a number or find the corresponding definiton in the data member's defines or
        # in the definitions in cndefn.h, simply return the string that is present.
        return member["array"]

    def get_resolved_probe_type(self, member, record_id):
        if not any(k in member for k in ("type", "nest")):
            raise Exception("This data member does not have a type or nest type.")

        base_probe_type = member["type"] if "type" in member else member["nest"]

        if TYPE_MAP.get(base_probe_type, None):
            mapped_probe_type = TYPE_MAP.get(base_probe_type, base_probe_type)
        elif isinstance(base_probe_type, str) and base_probe_type.endswith("CH"):
            FOUND_CHOICE_TYPES.add(base_probe_type)
            mapped_probe_type = f"choice:{base_probe_type[:-2]}"
        else:
            UNMAPPED_TYPES.add(base_probe_type)
            mapped_probe_type = base_probe_type

        if is_visible_named_nest_member(member):
            mapped_probe_type = ""

        self.types.add(base_probe_type or "")

        resolved_array_length = (
            self.get_resolved_array_length(member, record_id)
            if "array" in member
            else None
        )

        return f"{mapped_probe_type}{" " if mapped_probe_type and resolved_array_length else ""}{f'Array [{resolved_array_length}]' if 'array' in member else ''}"

    def get_appended_record(
        self,
        data_member: Field,
        probe_name_prefix,
    ):
        nested_record_id = data_member.get("nest", None)
        nested_record = copy.deepcopy(self.cnrecs.get(nested_record_id, None))

        if not nested_record:
            return ""

        nested_field_groups = self.get_field_groups_by_record(
            nested_record, data_member.get("variability_directives", [])
        )

        if len(nested_field_groups) == 0:
            return ""

        return f"""<tr >
            <td colspan="4" style="border-top: 0px">
                <details class="note">
                    <summary>{data_member["nest"]}</summary>
                    <div>
{"\n".join([self.get_field_group_table_string(group, nested_record_id, probe_name_prefix) for group in nested_field_groups])}
                    </div>
                </details>
            </td>
        </tr>"""

    def resolve_directive(
        self, args: list[str], options: list[str], fallback: str | None = None
    ) -> str:
        for opt in options:
            if opt in args:
                return opt

        return fallback

    def get_resolved_variability_directives_string(self, member: Field) -> str:
        FREQUENCY_DIRECTIVES = [
            "s",
            "h",
            "d",
            "mh",
            "m",
            "y",
            "r",
            "f",
            "i",
            "z",
        ]

        AVAILABILITY_DIRECTIVES = ["p", "e", "b"]

        directives = member.get("variability_directives", [])

        if is_visible_named_nest_member(member):
            return ""

        resolved_frequency = self.resolve_directive(directives, FREQUENCY_DIRECTIVES)
        resolved_availability = self.resolve_directive(
            directives, AVAILABILITY_DIRECTIVES, fallback="b"
        )

        if not resolved_frequency:
            self.members_with_no_frequency_directive.add(
                f"{member['name'], member['type']}"
            )

        self.variability_combinations.add(
            f"{resolved_frequency},{resolved_availability}"
        )

        try:
            return DESCRIPTION_MAP[resolved_frequency][resolved_availability]
        except KeyError:
            return "Unknown"

    def get_data_member_row(
        self, member: Field, record_id, probe_name_prefix: Optional[str]
    ):

        resolved_name = get_resolved_probe_name(member, probe_name_prefix)
        resolved_type = self.get_resolved_probe_type(member, record_id)
        resolved_variability_directives_string = (
            self.get_resolved_variability_directives_string(member)
        )
        resolved_comment_string = get_resolved_comment_string(
            member.get("comments", [])
        )

        appended_record = self.get_appended_record(member, resolved_name)

        return (
            f"""<tr>
                <td>{resolved_name}</td>
                <td>{resolved_type}</td>
                <td>{resolved_variability_directives_string}</td>
                <td>{resolved_comment_string}</td>
            </tr>"""
            + appended_record
        )

    def get_field_group_table_string(
        self, group, record_id, probe_name_prefix: Optional[str]
    ):
        visible_data_members = get_visible_data_members_by_group(group)

        if len(visible_data_members) == 0:
            return ""

        result = f"""
{get_resolved_comment_string(group.get("comments", []))}
<div class="probe-table-sibling"></div>
<table>
    <thead>
        <tr>
            <th>Name</th>
            <th>Type</th>
            <th>Variability</th>
            <th>Description/Comments</th>
        </tr>
    </thead>
    <tbody>
        {"\n".join([self.get_data_member_row(member, record_id, probe_name_prefix) for member in visible_data_members])}
    </tbody>
</table>"""

        return result

    def write_probes_by_record(self, record: Record):
        result = [f"---\ntitle: {record['name']}\n---"]
        result += [f"[](){{ #p_{record["name"].lower()} }}"]

        resolved_field_groups = self.get_field_groups_by_record(record, [])
        result += [
            self.get_field_group_table_string(group, record["id"], None)
            for group in resolved_field_groups
        ]

        filename = f"{record["name"]}.md"
        path = self.destination_dir / filename

        new_content = "".join(result)
        current_content = ""
        try:
            with open(path, "r") as f:
                current_content = f.read()
        except FileNotFoundError:
            # File doesn't exist, so write the new content
            pass
        except Exception:
            # print(f"Error reading file {path}: {e}")
            return

        if new_content == current_content:
            # TODO: Add logging, so this can be added to a debug-level output.
            # print(f"Content of {filename} is already identical, no write performed.")
            pass
        else:
            with open(path, "w") as f:
                f.write(new_content)

    def write_all_probes(self):
        records = list(self.cnrecs.values())

        for alias, name in RECORD_ALIASES.items():
            for record in records:
                if record["name"] == name:
                    record_copy = copy.deepcopy(record)
                    record_copy["name"] = alias
                    records.append(record_copy)
                    break

        for record in filter(
            lambda r: r["type"] == "RAT" and "hideall" not in r, records
        ):
            self.write_probes_by_record(record)


def on_config(config, **kwargs):
    docs_dir = Path(__file__).parent / "docs"
    probes_out_dir = docs_dir / "probe-definitions"

    writer = ProbeWriter(
        {
            "cndefns": path_to_cndefns,
            "cnfields": path_to_cnfields,
            "cnrecs": path_to_cnrecs,
        },
        probes_out_dir,
    )

    writer.write_all_probes()

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
    "TOP": slugify("Top members"),
    "END": slugify("END and ENDxxxx"),
    "ENDxxxx": slugify("END and ENDxxxx"),
    "FROZEN": slugify("FREEZE"),
    "TYPE": slugify("DEFTYPE"),
    # TODO: Eliminate "clause" from heading? Then, we don't need to prepopulate.
    "LIKE": slugify("LIKE clause"),
    "COPY": slugify("COPY clause"),
    "USETYPE": slugify("USETYPE clause"),
    # Stacked headers
    "dcTauB": slugify("dcTauB, dcTauD"),
    "dcTauD": slugify("dcTauB, dcTauD"),
    "dcEbnSlrNoon": slugify("dcEbnSlrNoon, dcEdhSlrNoon"),
    "dcEdhSlrNoon": slugify("dcEbnSlrNoon, dcEdhSlrNoon"),
    "ahWzCzns": slugify("ahWzCzns, ahCzCzns"),
    "ahCzCzns": slugify("ahWzCzns, ahCzCzns"),
    "chAuxOnMtr": slugify("chAuxOnMtr, chAuxOffMtr, chAuxFullOffMtr, chAuxOnAtAllMtr"),
    "chAuxOffMtr": slugify("chAuxOnMtr, chAuxOffMtr, chAuxFullOffMtr, chAuxOnAtAllMtr"),
    "chAuxFullOffMtr": slugify(
        "chAuxOnMtr, chAuxOffMtr, chAuxFullOffMtr, chAuxOnAtAllMtr"
    ),
    "chAuxOnAtAllMtr": slugify(
        "chAuxOnMtr, chAuxOffMtr, chAuxFullOffMtr, chAuxOnAtAllMtr"
    ),
    "rsLoadMtr": slugify("rsLoadMtr, rsHtgLoadMtr, rsClgLoadMtr"),
    "rsHtgLoadMtr": slugify("rsLoadMtr, rsHtgLoadMtr, rsClgLoadMtr"),
    "rsClgLoadMtr": slugify("rsLoadMtr, rsHtgLoadMtr, rsClgLoadMtr"),
    "rsSrcSideLoadMtr": slugify(
        "rsSrcSideLoadMtr, rsHtgSrcSideLoadMtr, rsClgSrcSideLoadMtr"
    ),
    "rsHtgSrcSideLoadMtr": slugify(
        "rsSrcSideLoadMtr, rsHtgSrcSideLoadMtr, rsClgSrcSideLoadMtr"
    ),
    "rsClgSrcSideLoadMtr": slugify(
        "rsSrcSideLoadMtr, rsHtgSrcSideLoadMtr, rsClgSrcSideLoadMtr"
    ),
    "sfExCTGrnd": slugify(
        "sfExCTGrnd, sfExCTaDbAvg07, sfExCTaDbAvg14, sfExCTaDbAvg31, sfExCTaDbAvgYr"
    ),
    "sfExCTaDbAvg07": slugify(
        "sfExCTGrnd, sfExCTaDbAvg07, sfExCTaDbAvg14, sfExCTaDbAvg31, sfExCTaDbAvgYr"
    ),
    "sfExCTaDbAvg14": slugify(
        "sfExCTGrnd, sfExCTaDbAvg07, sfExCTaDbAvg14, sfExCTaDbAvg31, sfExCTaDbAvgYr"
    ),
    "sfExCTaDbAvg31": slugify(
        "sfExCTGrnd, sfExCTaDbAvg07, sfExCTaDbAvg14, sfExCTaDbAvg31, sfExCTaDbAvgYr"
    ),
    "sfExCTaDbAvgYr": slugify(
        "sfExCTGrnd, sfExCTaDbAvg07, sfExCTaDbAvg14, sfExCTaDbAvg31, sfExCTaDbAvgYr"
    ),
    "cpStage": slugify("cpStageN"),
    "hpStage": slugify("hpStageN"),
    "whFAdjElec": slugify("whFAdjElec, whFAdjFuel"),
    "whFAdjFuel": slugify("whFAdjElec, whFAdjFuel"),
    "whInHtSupply": slugify("whInHtSupply, whInHtLoopRet"),
    "whInHtLoopRet": slugify("whInHtSupply, whInHtLoopRet"),
    "rsFChgC": slugify("rsFChgC, rsFChg"),
    "rsFChg": slugify("rsFChgC, rsFChg"),
    "wsFaucetCount": slugify(
        "wsFaucetCount, wsShowerCount, wsBathCount, wsCWashrCount, wsDWashrCount"
    ),
    "wsShowerCount": slugify(
        "wsFaucetCount, wsShowerCount, wsBathCount, wsCWashrCount, wsDWashrCount"
    ),
    "wsBathCount": slugify(
        "wsFaucetCount, wsShowerCount, wsBathCount, wsCWashrCount, wsDWashrCount"
    ),
    "wsCWashrCount": slugify(
        "wsFaucetCount, wsShowerCount, wsBathCount, wsCWashrCount, wsDWashrCount"
    ),
    "wsDWashrCount": slugify(
        "wsFaucetCount, wsShowerCount, wsBathCount, wsCWashrCount, wsDWashrCount"
    ),
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


def on_serve(server, config, **kwargs):
    docs_dir = Path(__file__).parent / "docs"
    server.unwatch(docs_dir)

    for item in docs_dir.iterdir():
        if item.is_file() or item.name != "probe-definitions":
            server.watch(docs_dir / item)

    server.watch(docs_dir / "probe-definitions" / ".nav.yml")
    server.watch(docs_dir / "probe-definitions" / "index.md")

    return server
