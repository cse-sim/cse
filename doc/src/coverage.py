import argparse
import copy
import re
import subprocess
from pathlib import Path
from typing import Any, Dict, List, Optional, Set

SKIP_LINE_COUNT = 4


class CoverageCheck:
    """
    A class for checking coverage between the output of "cse -c",
    and the markdown content in docs/input-data (or shared).
    """

    IGNORED_HEADERS = ["related probes", "inverse"]

    # Files in input-data directory that should not be analyzed for coverage.
    IGNORED_INPUT_DATA_FILE_PATHS = [".nav.yml", "index.md"]

    # The key is a record name that will be enriched with data members from the content
    # in the markdown file in the shared directory.
    SHARED = {
        "dhwheater": "dhwheater-doc.md",
        "dhwloopheater": "dhwheater-doc.md",
    }

    # cullist outputs enumerated data members. Docs aliases these with just the "N" postfix.
    # For example, cullist outputs "cpStage1", etc. while docs uses "cpStageN".
    ALIASES = {
        "cpstagen": [
            "cpstage1",
            "cpstage2",
            "cpstage3",
            "cpstage4",
            "cpstage5",
            "cpstage6",
            "cpstage7",
        ],
        "hpstagen": [
            "hpstage1",
            "hpstage2",
            "hpstage3",
            "hpstage4",
            "hpstage5",
            "hpstage6",
            "hpstage7",
        ],
    }

    @staticmethod
    def md_header(line: str, levels: List[int]) -> Optional[str]:
        """
        Checks whether the given line is a heading at the level specified by levels.

        Args:
            line: A line of markdown text
            levels: List of heading levels (1-6) to match. Defaults to [1].

        Returns:
            The heading value if found and level matches, None otherwise
        """
        match = re.match(r"^(#+)\s+(.*)$", line)
        if match:
            hash_count = len(match.group(1))
            if hash_count in levels:
                return match.group(2).strip()
        return None

    @staticmethod
    def record_header(line: str) -> str | None:
        """
        Thin wrapper of md_header for finding h1 record headers.

        Args:
            line: A line of markdown text

        Returns:
            The record header if found and not ignored, None otherwise
        """

        heading = CoverageCheck.md_header(line, [1])

        return heading.strip() if heading and not CoverageCheck.ignore(heading) else None

    @staticmethod
    def data_item_header(line: str) -> List[str]:
        """
        Thin wrapper of md_header for finding h3 data item headers.

        Args:
            line: A line of markdown text

        Returns:
            The data item header(s) if found and not ignored, None otherwise
        """

        heading = CoverageCheck.md_header(line, [3])

        if not heading:
            return []

        return [result.strip() for result in heading.split(",") if not CoverageCheck.ignore(result)]

    @staticmethod
    def ignore(header: str) -> bool:
        """
        Return True if the given header should be ignored.

        Args:
            header: Header string to check

        Returns:
            True if header should be ignored, False otherwise
        """
        h = header.lower()
        return h in CoverageCheck.IGNORED_HEADERS

    @staticmethod
    def parse_record_document(content: str, expect_orphan_data_items: Optional[bool] = False) -> Dict[str, Set[str]]:
        """
        Given the content of a markdown document of a record and its input, return
        a RecordInputSet for the records and input fields found in that document.

        Args:
            content: The markdown document content

        Returns:
            Dictionary mapping record names to sets of input field names
        """
        output: Dict[str, Set[str]] = {}
        current_header = None

        for line in content.splitlines():
            stripped_line = line.rstrip()
            h = CoverageCheck.record_header(stripped_line)
            if h and not CoverageCheck.ignore(h):
                current_header = h
                if current_header not in output:
                    output[current_header] = set()
            else:
                input_hdr = CoverageCheck.data_item_header(stripped_line)

                if len(input_hdr) == 0:
                    continue

                if not current_header and "orphan_members" not in output:
                    output["orphan_members"] = set()

                for ih in input_hdr:
                    ih_lower_strip = ih.strip().lower()
                    resolved_heading = (
                        CoverageCheck.ALIASES[ih_lower_strip]
                        if ih_lower_strip in CoverageCheck.ALIASES
                        else ih_lower_strip
                    )
                    if type(resolved_heading) is list:
                        for item in resolved_heading:
                            output[current_header or "orphan_members"].add(item)
                    elif type(resolved_heading) is str:
                        output[current_header or "orphan_members"].add(resolved_heading)

        if not expect_orphan_data_items and "orphan_members" in output:
            print(output["orphan_members"])
            raise Exception("Found orphan members but expect_orphan_data_items=False")

        return output

    @staticmethod
    def read_record_document(path: Path, expect_orphan_data_items: Optional[bool] = False) -> Dict[str, Set[str]]:
        """
        Given a file path to a markdown document containing record documentation,
        return a RecordInputSet for the records and input fields found in that
        document.

        Args:
            path: Path to the markdown file

        Returns:
            Dictionary mapping record names to sets of input field names
        """
        with open(path, "r", encoding="utf-8") as f:
            content = f.read()

        return CoverageCheck.parse_record_document(content, expect_orphan_data_items)

    @staticmethod
    def read_all_record_documents(paths: List[Path]) -> Dict[str, Set[str]]:
        """
        Given an array of file paths to record documentation, consecutively
        construct a RecordInputSet from all of the documents listed. Note: it is an
        error for a document to define the same Record Name, Record Input Field
        combination.

        Args:
            paths: List of file paths to markdown documents

        Returns:
            Dictionary mapping record names to sets of input field names

        Raises:
            ValueError: If duplicate keys are found
        """
        output = {}
        for path in paths:
            new_output = CoverageCheck.read_record_document(path)
            for k in new_output.keys():
                if k in output:
                    raise ValueError(f"Duplicate keys found: {k} at {path}")
            output.update(new_output)
        return output

    @staticmethod
    def parse_cul_list(content: str) -> Dict[str, Set[str]]:
        """
        Given the string content of the results of `cse -c > cullist.txt`,
        parse that into a RecordInputSet.

        Args:
            content: Content of the cullist.txt file

        Returns:
            Dictionary mapping record names to sets of field names
        """
        output: Dict[str, Set[str]] = {}
        current_record = None

        lines = content.splitlines()
        for idx, line in enumerate(lines):
            if idx < SKIP_LINE_COUNT:  # Skip first 4 lines
                continue

            stripped_line = line.rstrip()
            record_match = re.match(r"^(\S+).*$", stripped_line)
            if record_match:
                current_record = record_match.group(1).strip()
                if current_record not in output:
                    output[current_record] = set()
            elif current_record is not None:
                field_match = re.match(r"^   (\S+).*$", stripped_line)
                if field_match:
                    output[current_record].add(field_match.group(1).strip())

        return output

    @staticmethod
    def set_differences(
        s1: Set[str],
        s2: Set[str],
        case_matters: bool = False,
        keys_to_ignore: Optional[Set[str]] = None,
    ) -> Optional[Dict[str, Set[str]]]:
        """
        Given two sets and an optional flag that controls case sensitivity,
        check for differences between the contents of set 1 and set 2.

        Args:
            s1: First set
            s2: Second set
            case_matters: If True compare with case, else case-insensitive
            keys_to_ignore: Set of keys to ignore

        Returns:
            Dictionary with 'in_1st_not_2nd' and 'in_2nd_not_1st' keys, or None if no differences
        """
        if keys_to_ignore is None:
            keys_to_ignore = set()

        if s1 == s2:
            return None

        if case_matters:
            s1_ = s1 - keys_to_ignore
            s2_ = s2 - keys_to_ignore
            if s1_ == s2_:
                return None
            return {"in_1st_not_2nd": s1_ - s2_, "in_2nd_not_1st": s2_ - s1_}
        else:
            keys_to_ignore_dc = {k.lower() for k in keys_to_ignore}
            m1 = {item.lower(): item for item in s1}
            m2 = {item.lower(): item for item in s2}
            s1_ = set(m1.keys()) - keys_to_ignore_dc
            s2_ = set(m2.keys()) - keys_to_ignore_dc

            if s1_ == s2_:
                return None

            return {
                "in_1st_not_2nd": {m1[k] for k in (s1_ - s2_ - keys_to_ignore_dc)},
                "in_2nd_not_1st": {m2[k] for k in (s2_ - s1_ - keys_to_ignore_dc)},
            }

    @staticmethod
    def adjust_map(m: Dict[str, Set[str]]) -> Dict[str, Set[str]]:
        """
        Given a map from string to set of string, adjust the map so that fields
        with spacing in them get collapsed to the record defined by the first word
        through set union.

        Args:
            m: Dictionary to adjust

        Returns:
            Adjusted dictionary
        """
        n: Dict[str, Set[str]] = {}
        for k, v in m.items():
            new_k = k.split()[0] if " " in k else k
            if new_k in n:
                n[new_k] = n[new_k].union(v)
            else:
                n[new_k] = v.copy()
        return n

    @staticmethod
    def downcase_record_input_set(ris: Dict[str, Set[str]]) -> Dict[str, Set[str]]:
        """
        Downcase a record input set.

        Args:
            ris: Record input set

        Returns:
            Downcased record input set
        """
        n = {}
        for k, vs in ris.items():
            n[k.lower()] = {v.lower() for v in vs}
        return n

    @staticmethod
    def record_input_set_differences(
        ris1: Dict[str, Set[str]],
        ris2: Dict[str, Set[str]],
        case_matters: bool = False,
        records_to_ignore: Optional[List[str]] = None,
        record_fields_to_ignore: Optional[Dict[str, Set[str]]] = None,
    ) -> Optional[Dict[str, Any]]:
        """
        Given two RecordInputSet objects, compare them and return any differences.

        Args:
            ris1: First record input set
            ris2: Second record input set
            case_matters: Whether case matters in comparison
            records_to_ignore: Set of record names to ignore
            record_fields_to_ignore: Map of record names to sets of fields to ignore

        Returns:
            Dictionary of differences or None if no differences
        """
        if records_to_ignore is None:
            records_to_ignore = []
        if record_fields_to_ignore is None:
            record_fields_to_ignore = {}

        if not case_matters:
            # Downcase everything if case doesn't matter
            ris1 = {k.lower(): {v.lower() for v in vs} for k, vs in ris1.items()}
            ris2 = {k.lower(): {v.lower() for v in vs} for k, vs in ris2.items()}
            records_to_ignore_set = {k.lower() for k in records_to_ignore}
            record_fields_to_ignore = {k.lower(): v for k, v in record_fields_to_ignore.items()}

        if ris1 == ris2:
            return None

        ks1 = set(ris1.keys())
        ks2 = set(ris2.keys())
        key_diffs = CoverageCheck.set_differences(ks1, ks2, case_matters, records_to_ignore_set)

        out: Optional[Dict[str, Any]] = None
        if key_diffs:
            out = {
                "records_in_1st_not_2nd": key_diffs["in_1st_not_2nd"],
                "records_in_2nd_not_1st": key_diffs["in_2nd_not_1st"],
            }

        # Check fields
        ks = ks1 & ks2
        for k in ks:
            if k in records_to_ignore:
                continue

            field_ignores = record_fields_to_ignore.get(k, set())
            diffs = CoverageCheck.set_differences(ris1.get(k, set()), ris2.get(k, set()), case_matters, field_ignores)

            if diffs:
                if out is None:
                    out = {
                        "records_in_1st_not_2nd": None,
                        "records_in_2nd_not_1st": None,
                    }
                if "field_set_differences" not in out:
                    out["field_set_differences"] = {}
                out["field_set_differences"][k] = diffs

        return out

    @staticmethod
    def diffs_to_string(  # noqa: PLR0913
        diffs: Optional[Dict[str, Set[str]]],
        name: Optional[str],
        k1: str,
        k2: str,
        a: str,
        b: str,
        indent: int = 0,
    ) -> str:
        """
        Given a map of differences, format them as a string report.

        Args:
            diffs: Dictionary of differences
            name: Name of what the map represents
            k1, k2: Keys into the differences map
            a, b: Names for the two variations being compared
            indent: Number of spaces to indent

        Returns:
            Formatted string report
        """
        if diffs is None:
            return ""

        ind = " " * indent
        s = ""

        if name and diffs and (diffs.get(k1) or diffs.get(k2)):
            s += f"{name}:\n"

        for k, x, y in [(k1, a, b), (k2, b, a)]:
            if diffs.get(k) and diffs[k]:
                s += f"{ind}in {x} but not in {y}:\n"
                for r in sorted(diffs[k]):
                    s += f"{ind}- {r}\n"

        return s

    def __init__(self, path_to_cse: Path, path_to_input_data: Path):
        self.path_to_cse = path_to_cse
        self.path_to_input_data = path_to_input_data

        cullist_raw = subprocess.run([self.path_to_cse, "-c"], capture_output=True, text=True, check=True).stdout

        self.cullist = self.parse_cul_list(cullist_raw)
        self.check()

    def drop_name_fields(
        self,
        ris: Dict[str, Set[str]],
        case_sensitive: bool = False,
    ) -> Dict[str, Set[str]]:
        """
        Given a record input set, drop all fields
        that end with "Name" UNLESS they also appear in cullist.

        Args:
            ris: Record input set to process
            ref: Reference record input set
            case_sensitive: Whether to compare case-sensitively

        Returns:
            Processed record input set
        """
        if not case_sensitive:
            ref = self.downcase_record_input_set(self.cullist)

        name_suffix = "Name" if case_sensitive else "name"
        n: Dict[str, Set[str]] = {}

        for k, vs in ris.items():
            kk = k if case_sensitive else k.lower()
            n[k] = set()
            for v in vs:
                w = v if case_sensitive else v.lower()
                if w.endswith(name_suffix) and not (kk in ref and w in ref[kk]):
                    continue  # Skip this field
                n[k].add(v)

        return n

    def check(self):
        all_file_paths_in_input_data = [
            path
            for path in self.path_to_input_data.iterdir()
            if path.is_file() and path.name not in self.IGNORED_INPUT_DATA_FILE_PATHS
        ]

        input_data_headings = self.read_all_record_documents(all_file_paths_in_input_data)

        shared_map = {}
        for k_shared, v_shared in self.SHARED.items():
            path = self.path_to_input_data.parent / "shared" / v_shared
            headings = self.read_record_document(path, expect_orphan_data_items=True)
            shared_map[k_shared] = headings["orphan_members"]

        docs_headings = copy.deepcopy(input_data_headings)
        for k_map, v_map in shared_map.items():
            if k_map not in docs_headings:
                docs_headings[k_map] = set()
            for h in v_map:
                docs_headings[k_map].add(h)

        docs_headings = self.drop_name_fields(self.adjust_map(docs_headings))
        self.diffs = self.record_input_set_differences(
            self.cullist, docs_headings, records_to_ignore=self.IGNORED_HEADERS
        )

        self.passed = self.diffs is None

        return self.passed

    def record_input_set_differences_to_string(self) -> str:
        """
        Given the output of record_input_set_differences, format it and return as a string.

        Args:
            diffs: Output from record_input_set_differences
            a: Name for first set (default: "First")
            b: Name for second set (default: "Second")

        Returns:
            Formatted string report
        """
        a = "cullist"
        b = "input-data"

        self.check()
        diffs = self.diffs

        if diffs is None:
            return f"No changes detected between {a} and {b}"

        s = ""

        if diffs.get("records_in_1st_not_2nd") or diffs.get("records_in_2nd_not_1st"):
            s += "RECORD INCONSISTENCIES:\n"
            s += CoverageCheck.diffs_to_string(diffs, None, "records_in_1st_not_2nd", "records_in_2nd_not_1st", a, b, 4)

        if diffs.get("field_set_differences"):
            ks = sorted(diffs["field_set_differences"].keys())
            if ks:
                s += "DATA FIELD INCONSISTENCIES:\n"
                for k in ks:
                    s += CoverageCheck.diffs_to_string(
                        diffs["field_set_differences"][k],
                        k,
                        "in_1st_not_2nd",
                        "in_2nd_not_1st",
                        a,
                        b,
                        4,
                    )

        return s


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Coverage Checker for CSE Documenation built with Mkdocs")
    parser.add_argument(
        "--path_to_cse",
        type=str,
        help="The path to the CSE executable.",
    )

    parser.add_argument(
        "--path_to_input_data",
        type=str,
        help="The path to the documentation's input-data folder.",
        default=((Path(__file__).parent / "docs" / "input-data").resolve().absolute()),
    )

    args = parser.parse_args()

    coverage_checker = CoverageCheck(Path(args.path_to_cse), Path(args.path_to_input_data))

    if coverage_checker.passed:
        print("Passed!")
    else:
        print(coverage_checker.record_input_set_differences_to_string())
