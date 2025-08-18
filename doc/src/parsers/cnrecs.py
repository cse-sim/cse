import re

from pathlib import Path
from typing import Literal, List, Set, Dict, TypedDict

from .base import BaseParser
from .cnfields import FieldsResult
from .cndefns import DefinitionsResult


# TODO:Better definition for Field
class Field(TypedDict, total=False):
    name: str
    type: str
    # could make this a union of string literals
    # Can this be derived from FIELD_VARIABILITY_DIRECTIVES?
    variability_directives: List[str]
    # Can hide/noname be derived from bool directives constant?
    hide: bool
    noname: bool
    # Can array/est be derived from kvp directives constant?
    array: str
    nest: str


# class FieldGroup()
class FieldGroup(TypedDict, total=False):
    members: List[Field]
    comments: List[str]


type RecordType = Literal["RAT", "SUBSTRUCT"]


class Record(TypedDict, total=False):
    id: str
    name: str
    type: RecordType
    field_groups: List[FieldGroup]
    record_comments: List[str]


type RecordsResult = Dict[str, Record]


RECORD_BLOCK_PATTERN = re.compile(
    r"^\s*(RECORD)\s+([a-zA-Z_0-9-]+)\s+" r"(.*?)" r"^\s*\*END.*?$",
    re.MULTILINE | re.DOTALL,
)

RECORD_HEADER_PATTERN = re.compile(
    r"""
    ^RECORD\s+
    (?P<id>[a-zA-Z_0-9]+)\s+
    "(?P<name>.*?)"\s+
    \*(?P<type>RAT|SUBSTRUCT)\b
    (?P<rest>.*)
    """,
    re.VERBOSE | re.IGNORECASE,
)

RECORD_HEADER_BOOL_DIRECTIVES = {"hideall"}
RECORD_HEADER_KVP_DIRECTIVES = {"baseclass"}

RECORD_BOOL_DIRECTIVES = {"excon", "exdes", "ovrcopy"}
RECORD_KVP_DIRECTIVES = {"prefix"}

FIELD_BOOL_DIRECTIVES = {"hide", "noname"}
FIELD_KVP_DIRECTIVES = {
    "array",
    "nest",
}
FIELD_VARIABILITY_DIRECTIVES = {
    "s",
    "h",
    "mh",
    "d",
    "m",
    "r",
    "y",
    "e",
    "p",
    "f",
    "i",
    "z",
}


# Inline block means it is a block comment that starts and ends on the same line.
# Example: This line has /* block comment */ an inline block comment.
COMMENT_TOKEN_PATTERN = re.compile(
    r"(?P<inline_block>/\*.*?\*/)|(?P<inline_start>//)|(?P<block_start>/\*)|(?P<block_end>\*/)"
)


def parse_content_and_comments(line: str, in_block_comment=False):
    content = []
    comments = []

    if line.strip() == "":
        return [], [], in_block_comment

    for match in COMMENT_TOKEN_PATTERN.finditer(line):
        if in_block_comment:
            if match.group("inline_block") or match.group("block_end"):
                leading_comment = line[: match.end() - 2]

                # Don't append blank lines, so the if-statement uses .strip().
                # But for now, we do want to preserve comment white space,
                # so the actual value (without .strip()) is stored.
                if leading_comment.strip():
                    comments += [leading_comment]
                nested_content, nested_comments, in_block_comment = (
                    parse_content_and_comments(line[match.end() :])
                )
                comments += nested_comments
                content += nested_content
                in_block_comment = False

            else:
                comments += [line]
                break

        else:
            if match.group("inline_start"):
                leading_content = line[: match.start()].strip()
                trailing_comment = line[match.start() + 2 :]

                if leading_content:
                    content += [leading_content]

                if trailing_comment.strip():
                    comments += [trailing_comment]
                break
            elif match.group("inline_block"):
                leading_content = line[: match.start()].strip()
                middle_comment = line[match.start() + 2 : match.end() - 2]

                if leading_content:
                    content += [leading_content]

                if middle_comment.strip():
                    comments += [middle_comment]

                nested_content, nested_comments, in_block_comment = (
                    parse_content_and_comments(line[match.end() :])
                )
                content += nested_content
                comments += nested_comments
            elif match.group("block_start"):
                leading_content = line[: match.start()].strip()
                trailing_comment = line[match.start() + 2 :]

                if leading_content:
                    content += [leading_content]

                if trailing_comment.strip():
                    comments += [trailing_comment]
                in_block_comment = True
            elif match.group("block_end"):
                raise Exception(
                    "End block comment token cannot appear first if not already in a block comment."
                )
        break  # ensure that if there are no matches from pattern.find_iter that we go to else block
    else:
        if in_block_comment and line.strip():
            comments += [line]
        elif line.strip():
            content += [line.strip()]

    return content, comments, in_block_comment


def make_field_dict_from_pending_tokens(
    type_name_pair,
    pending_variability_directives,
    pending_boolean_directives,
    pending_kvp_directives,
):
    result = {}

    if list(type_name_pair.keys())[0]:
        result["type"] = list(type_name_pair.keys())[0]

    if list(type_name_pair.values())[0]:
        result["name"] = list(type_name_pair.values())[0]

    if len(pending_variability_directives) > 0:
        result["variability_directives"] = pending_variability_directives
    if len(pending_boolean_directives) > 0:
        for boolean_directive in pending_boolean_directives:
            result[boolean_directive] = True
    if len(pending_kvp_directives) > 0:
        for kvp in pending_kvp_directives:
            result[list(kvp.keys())[0]] = list(kvp.values())[0]

    return result


def parse_field_directives(
    line: str, field_pattern: re.Pattern, member_types: Set[str], first_line_of_record
):
    field_match = field_pattern.match(line)

    if field_match:
        field_data = {}
        tokens = [t for t in line.split(" ") if t.strip()]

        variability_directives = []
        boolean_directives = []
        kvp_directives = []

        field_array = []

        i = 0
        while i < len(tokens):
            token = tokens[i]
            next_token = tokens[i + 1] if i + 1 < len(tokens) else None

            if (
                token.startswith("*")
                and token[1:].lower() in FIELD_VARIABILITY_DIRECTIVES
            ):
                # could use set, but would prefer to maintain order in list. Probably an ordered set is a thing in python?
                if token[1:].lower() in variability_directives:
                    i += 1
                    continue

                variability_directives.append(token[1:].lower())

            elif token.startswith("*") and token[1:].lower() in FIELD_BOOL_DIRECTIVES:
                boolean_directives.append(token[1:].lower())

            elif token.startswith("*") and token[1:].lower() in FIELD_KVP_DIRECTIVES:
                if next_token is None:
                    raise Exception(
                        "A kvp directive appeared without a corresponding value."
                    )

                kvp_directives.append({token[1:].lower(): next_token})
                i += 1

            elif token.upper() in member_types:
                field_data["type"] = token.upper()
                if next_token is None:
                    raise Exception(
                        "A field type appeared without a corresponding field name."
                    )

                field_array.append(
                    make_field_dict_from_pending_tokens(
                        {token.upper(): next_token},
                        variability_directives,
                        boolean_directives,
                        kvp_directives,
                    )
                )
                variability_directives = []
                boolean_directives = []
                kvp_directives = []

                i += 1

            else:
                field_array.append(
                    make_field_dict_from_pending_tokens(
                        {"": token},
                        variability_directives,
                        boolean_directives,
                        kvp_directives,
                    )
                )
                variability_directives = []
                boolean_directives = []
                kvp_directives = []

            i += 1

        if any(
            [
                len(variability_directives) > 0,
                len(boolean_directives) > 0,
                len(kvp_directives) > 0,
            ]
        ):
            raise Exception(
                f"If a line containes variability, boolean, or kvp directives, it must also contain a field name and/or type.\n{line}\n{first_line_of_record}"
            )

        return field_array


def get_field_start_pattern(field_types: Set[str]) -> re.Pattern:
    # Combine known directives and member types into a single list
    directives = FIELD_BOOL_DIRECTIVES.union(
        FIELD_KVP_DIRECTIVES, FIELD_VARIABILITY_DIRECTIVES
    )

    # Prefix directives with literal *
    directive_patterns = [r"\*" + re.escape(d) for d in directives]
    member_patterns = [re.escape(m) for m in field_types]

    # Combine and build the full pattern
    all_patterns = directive_patterns + member_patterns
    pattern = r"^\s*(?:" + "|".join(all_patterns) + r")\b"

    return re.compile(pattern, re.IGNORECASE)


def remove_multiline_comments(text: str):
    """Remove all multiline comments (i.e., /*...*/)."""
    pattern = re.compile(r"/\*.*?\*/", re.DOTALL)

    return pattern.sub(" ", text)


def remove_inline_comments(text: str):
    cleaned_lines = [line.split("//", 1)[0].rstrip() for line in text.splitlines()]
    return "\n".join(cleaned_lines)


def remove_comments(text: str):
    return remove_inline_comments(remove_multiline_comments(text))


def parse_directives(
    text: str, bool_directives: Set[str], kvp_directives: Set[str]
) -> dict:
    directives = {}

    tokens = re.findall(r"\*([a-zA-Z0-9_]+)(?:\s+([a-zA-Z0-9_]+))?", text)

    for key, value in tokens:
        key_lower = key.lower()
        if key_lower in bool_directives:
            directives[key_lower] = True
        elif key_lower in kvp_directives:
            directives[key_lower] = value.strip() if value else None

    return directives if len(tokens) > 0 else None


def parse_declare_directive(text: str):
    match = re.match(r'\s*\*declare\s+"(.*?)"', text)

    if match:
        return {"declare": match.group(1)}


def parse_define_statement(text: str):
    match = re.match(r"#define\s+(.*)", text)

    if match:
        return {"define": match.group(1)}


def parse_record(text, field_types: Set[str]):
    data = {}

    lines = text.splitlines()
    index = 0
    in_block_comment = False
    pending_comments = []
    pending_fields = []

    elapsed_lines = 0

    while index < len(lines):
        line = lines[index]

        content, comments, in_block_comment = parse_content_and_comments(
            line, in_block_comment
        )

        merged_line_content = " ".join(content)

        header_match = RECORD_HEADER_PATTERN.match(merged_line_content)
        if header_match:
            data["id"] = header_match.group("id")
            data["name"] = header_match.group("name")
            data["type"] = header_match.group("type")

            parsed_record_header_directives = parse_directives(
                merged_line_content,
                RECORD_HEADER_BOOL_DIRECTIVES,
                RECORD_HEADER_KVP_DIRECTIVES,
            )

            data = {**data, **parsed_record_header_directives}

            if len(comments) > 0:
                data["record_comments"] = comments

            next_in_block_comment = in_block_comment
            while index + 1 < len(lines):
                next_line_content, next_line_comments, next_in_block_comment = (
                    parse_content_and_comments(lines[index + 1], next_in_block_comment)
                )

                # Capture all comments until either a blank line or line with real content is encountered.
                if len(next_line_content) == 0 and len(next_line_comments) > 0:
                    if "record_comments" in data:
                        data["record_comments"] += next_line_comments
                    else:
                        data["record_comments"] = next_line_comments
                    in_block_comment = next_in_block_comment
                    index += 1
                else:
                    break

        if len(content) == 0 and len(comments) > 0:
            pending_comments += comments
            index += 1
            elapsed_lines = 0
            continue

        if len(content) == 0 and len(comments) == 0:
            #
            if "field_groups" not in data:
                data["field_groups"] = []

            if len(pending_fields) > 0:
                field_group = {"members": pending_fields}

                if len(pending_comments) > 0:
                    field_group["comments"] = pending_comments

                data["field_groups"] += [field_group]
            pending_comments = []
            pending_fields = []

        # If a pending comment is more than one line away when
        # the next parsed field directive appearsh, do not assume
        # it is related.
        if elapsed_lines > 1:
            pending_comments = []
            elapsed_lines = 0

        record_directive = parse_directives(
            merged_line_content, RECORD_BOOL_DIRECTIVES, RECORD_KVP_DIRECTIVES
        )

        if record_directive and len(record_directive.keys()) > 1:
            raise Exception(
                "Parser expects at most one record directive (or declare directive) per line"
            )

        if record_directive:
            if len(pending_comments) > 0 and len(pending_fields) == 0:
                pending_comments = []

            next_in_block_comment = in_block_comment
            while index + 1 < len(lines):
                next_line_content, next_line_comments, next_in_block_comment = (
                    parse_content_and_comments(lines[index + 1], next_in_block_comment)
                )

                # Capture all comments until either a blank line or line with real content is encountered.
                if len(next_line_content) == 0 and len(next_line_comments) > 0:
                    comments += next_line_comments
                    in_block_comment = next_in_block_comment
                    index += 1
                else:
                    break

            key = next(iter(record_directive))
            value = record_directive[key]
            if len(comments) > 0:
                data = {**data, key: {"value": value, "comments": comments}}
            else:
                data = {**data, **record_directive}

        define_statement = parse_define_statement(merged_line_content)

        if define_statement:
            next_in_block_comment = in_block_comment
            while index + 1 < len(lines):
                next_line_content, next_line_comments, next_in_block_comment = (
                    parse_content_and_comments(lines[index + 1], next_in_block_comment)
                )

                # Capture all comments until either a blank line or line with real content is encountered.
                if len(next_line_content) == 0 and len(next_line_comments) > 0:
                    comments += next_line_comments
                    in_block_comment = next_in_block_comment
                    index += 1
                else:
                    break

            if len(comments) > 0:
                define_statement["comments"] = comments

            pending_fields.append(define_statement)

        # Fields

        declare_directive = parse_declare_directive(line)
        if declare_directive:
            next_in_block_comment = in_block_comment
            while index + 1 < len(lines):
                next_line_content, next_line_comments, next_in_block_comment = (
                    parse_content_and_comments(lines[index + 1], next_in_block_comment)
                )

                # Capture all comments until either a blank line or line with real content is encountered.
                if len(next_line_content) == 0 and len(next_line_comments) > 0:
                    comments += next_line_comments
                    in_block_comment = next_in_block_comment
                    index += 1
                else:
                    break

            if len(comments) > 0:
                declare_directive["comments"] = comments

            pending_fields.append(declare_directive)

        field_pattern = get_field_start_pattern(field_types)

        field_directives = parse_field_directives(
            merged_line_content, field_pattern, field_types, lines[0]
        )
        if len(field_directives or []) > 0:
            next_in_block_comment = in_block_comment
            while index + 1 < len(lines):
                next_line_content, next_line_comments, next_in_block_comment = (
                    parse_content_and_comments(lines[index + 1], next_in_block_comment)
                )

                # Capture all comments until either a blank line or line with real content is encountered.
                if len(next_line_content) == 0 and len(next_line_comments) > 0:
                    comments += next_line_comments
                    in_block_comment = next_in_block_comment
                    index += 1
                else:
                    break

            if len(comments) > 0:
                for directive in field_directives:
                    directive["comments"] = comments

            pending_fields += field_directives

        # If line is empty or we are at end of record block, attach any pending
        # fields as a new field group.
        if (len(content) == 0 and len(comments) == 0) or (
            index == len(lines) - 1 and len(pending_fields) > 0
        ):
            #
            if "field_groups" not in data:
                data["field_groups"] = []

            if len(pending_fields) > 0:
                field_group = {"members": pending_fields}

                if len(pending_comments) > 0:
                    field_group["comments"] = pending_comments

                data["field_groups"] += [field_group]
            pending_comments = []
            pending_fields = []

        index += 1

    return data


class RecordsParser(BaseParser[RecordsResult]):
    def __init__(
        self, path: Path, definitions: DefinitionsResult, fields: FieldsResult
    ):
        super().__init__(path)
        self.definition_flags = definitions["bool"]
        self.field_types = {field["typename"] for field in fields}

    def _prepare_clean_text(self):
        text = re.sub(r"#if\s+0\b.*?#endif\b", "", self.raw_text, flags=re.DOTALL)

        pattern = re.compile(
            r"""(?msx)
            ^\s* \#                             
            (?:
                ifdef \s+ (?P<ifdef_def>[A-Za-z_][A-Za-z0-9_]*)
            | if \s+ defined \s* \( \s* (?P<defined_def>[A-Za-z_][A-Za-z0-9_]*) \s* \)
            )
            [^\n]* \n                           
            (?P<if_block> .*? )                
            (?: ^\s* \#else[^\n]* \n
                (?P<else_block> .*? )
            )?
            ^\s* \#endif[^\n]* \n?
            """,
            re.MULTILINE | re.DOTALL | re.VERBOSE,
        )

        output: str = text
        offset = 0

        for match in pattern.finditer(text):
            def_name = match.group("ifdef_def") or match.group("defined_def")
            if_block = match.group("if_block")
            else_block = match.group("else_block") or ""

            # Case-insensitive "startswith" matching
            if not any(
                def_name.lower().startswith(allowed.lower())
                for allowed in self.definition_flags
            ):
                replacement = else_block
            else:
                replacement = if_block

            start, end = match.start(), match.end()
            output = output[: start - offset] + replacement + output[end - offset :]
            offset += end - start - len(replacement)

        return output

    def _parse(self):
        records = RECORD_BLOCK_PATTERN.findall(self.clean_text)

        result: RecordsResult = {}
        for record in records:
            record_parsed = parse_record(" ".join(record), self.field_types)

            result[record[1]] = record_parsed

        return result
