from pathlib import Path
import re
from typing import Dict, Any

import yaml

RECORD_HEADER_BOOL_DIRECTIVES = ["hideall"]
RECORD_HEADER_KVP_DIRECTIVES = ["baseclass"]

RECORD_BOOL_DIRECTIVES = ["excon", "exdes", "ovrcopy"]
RECORD_KVP_DIRECTIVES = ["prefix"]


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


def get_record_header_directives_string(record):
    directives_str = ""

    bool_directives = [
        f"*{directive}"
        for directive in RECORD_HEADER_BOOL_DIRECTIVES
        if directive in record
    ]

    kvp_directives = [
        f"*{key} {record[key]}" for key in RECORD_HEADER_KVP_DIRECTIVES if key in record
    ]

    directives_str += " ".join(bool_directives + kvp_directives)

    return directives_str


def get_record_directives_string(record):
    directives_str = ""

    bool_directives = []
    for directive in RECORD_BOOL_DIRECTIVES:
        if directive not in record:
            continue

        value = record[directive]

        if isinstance(value, bool):
            bool_directives.append(f"*{directive}")
        else:
            value_as_str = f"*{directive}" + "\n".join(
                [f" //{comment}" for comment in value["comments"]]
            )
            bool_directives.append(value_as_str)

    kvp_directives = []
    for directive in RECORD_KVP_DIRECTIVES:
        if directive not in record:
            continue

        value = record[directive]

        if isinstance(value, str):
            kvp_directives.append(f"*{directive} {value}")
        else:
            value_as_str = f"*{directive} {value['value']}" + "\n".join(
                [f" //{comment}" for comment in value["comments"]]
            )
            kvp_directives.append(value_as_str)

    directives_str += "\n\t".join(kvp_directives + bool_directives)

    return directives_str


def get_data_member_string(member: Dict):
    data_member_string = ""

    keys = member.keys()

    if "variability_directives" in keys:
        if any(
            [
                directive not in FIELD_VARIABILITY_DIRECTIVES
                for directive in member["variability_directives"]
            ]
        ):
            raise Exception("Unrecognized variabilty directive")

        data_member_string = " ".join(
            [
                f"*{variability_directive}"
                for variability_directive in member["variability_directives"]
            ]
        )

    bool_directives = []
    for directive in FIELD_BOOL_DIRECTIVES:
        if directive not in member:
            continue

        value = member[directive]

        if isinstance(value, bool):
            bool_directives.append(f"*{directive}")

    kvp_directives = []
    for directive in FIELD_KVP_DIRECTIVES:
        if directive not in member:
            continue

        value = member[directive]

        kvp_directives.append(f"*{directive} {value}")

    if len(bool_directives + kvp_directives) > 0:
        data_member_string += " " + " ".join(bool_directives + kvp_directives)

    if "type" in keys:
        data_member_string += " " + member["type"]

    if "name" in keys:
        data_member_string += " " + member["name"]

    if "comments" in keys:
        data_member_string += "\n\t\t\t\t".join(
            [f" //{comment}" for comment in member["comments"]]
        )

    data_member_string += "\n"

    return data_member_string


def get_field_group_string(field_group: Dict):
    field_group_string = ""

    if "comments" in field_group:
        field_group_string += "\n".join(
            f" //{comment}" for comment in field_group["comments"]
        )

    field_group_string += "\n"

    if "members" in field_group:
        for member in field_group["members"]:
            if "declare" in member:
                field_group_string += f'\t*declare "{member["declare"]}"'

                if "comments" in member:
                    field_group_string += "\n".join(
                        f" //{comment}" for comment in member["comments"]
                    )

                field_group_string += "\n"

            elif "define" in member:
                field_group_string += f"\t#define {member['define']}"

                if "comments" in member:
                    field_group_string += "\n".join(
                        f" //{comment}" for comment in member["comments"]
                    )

                field_group_string += "\n"
            else:
                field_group_string += "\t" + get_data_member_string(member)

    return field_group_string


def remove_whitespace_and_blank_lines(text: str):
    return "\n".join(
        re.sub(r"[ \t]+", " ", line).strip().lower()
        for line in text.splitlines()
        if (line.strip() and not line.startswith("//===="))
    )


if __name__ == "__main__":
    root_dir = Path(__file__).parent.parent
    out_dir = root_dir / "example_out"

    with open(out_dir / "new_parser_parsed_cnrecs.yml", "r") as file:
        parsed_cnrecs: Dict[str, Any] = yaml.safe_load(file)

    new_str = []

    for record in parsed_cnrecs.values():
        new_str += ["//" + "=" * 40]
        new_str += [
            f'RECORD {record["id"]} "{record["name"]}" *{record["type"]} {get_record_header_directives_string(record)} {"\t//" + record["record_comments"][0] if "record_comments" in record else ""}'
        ]
        if "record_comments" in record and len(record["record_comments"]) > 1:
            new_str += [
                "\n".join(["//" + comment for comment in record["record_comments"][1:]])
            ]
        new_str += [""]
        new_str += ["\t" + get_record_directives_string(record)]
        new_str += [""]

        if "field_groups" in record:
            for field_group in record["field_groups"]:
                new_str += [get_field_group_string(field_group)]

        new_str += [f"*END\t\t// {record['id']}"]

    target_path = out_dir / "rebuilt_cnrecs_new.def"
    target_path_no_white_space = out_dir / "rebuilt_cnrecs_new_no-whitespace.def"

    with_whitespace = "\n".join(new_str)
    without_whitespace = remove_whitespace_and_blank_lines(with_whitespace)

    with open(target_path, "w") as file:
        file.write("\n".join(new_str))

    with open(target_path_no_white_space, "w") as file:
        file.write(without_whitespace)
