from pathlib import Path
from io import StringIO
import csv


def define_env(env):
    """
    This is the hook for the variables, macros and filters.
    """

    @env.macro
    def csv_table(csv_string, header=False):
        rows = list(csv.reader(StringIO(csv_string)))

        max_length = max(len(row) for row in rows)
        padded_rows = [row + [""] * (max_length - len(row)) for row in rows]

        header_row = padded_rows[0] if header else ([""] * max_length)
        body_rows = padded_rows[1:] if header else padded_rows

        table = (
            ('<div class="no-header-table"></div>\n' if not header else "")
            + "| "
            + " | ".join(header_row)
            + " |\n"
            + "| "
            + " | ".join("------" for _ in header_row)
            + " |\n"
            + (
                "\n".join(
                    ("| " + " | ".join(f"{c}" for c in row) + " |") for row in body_rows
                )
                if len(body_rows) > 0
                else ""
            )
        )

        return table

    @env.macro
    def csv_table_from_file(file_path, header=False):
        full_path = Path(__file__).parent.resolve().joinpath(file_path)
        csv_content = ""

        try:
            with open(full_path, "r") as file:
                csv_content = file.read()
        except FileNotFoundError:
            print(f"Error: File not found at path: {file_path}")
        except Exception as e:
            print(f"An error occurred: {e}")

        return csv_table(csv_content, header)

    @env.macro
    def member_table(args):
        fallback = "&mdash;"
        units = args.get("units", fallback)
        legal_range = args.get("legal_range", fallback)
        default = args.get("default", fallback)
        required = args.get("required", "No")
        variability = args.get("variability", "constant")

        table = (
            '<div class="member-table-sibling"></div>\n'
            + "| "
            + " | ".join(
                f"{c}"
                for c in ["Units", "Legal Range", "Default", "Required", "Variability"]
            )
            + " |\n"
            + "| "
            + " | ".join("------" for _ in 5 * [""])
            + " |\n"
            + "| "
            + " | ".join(
                f"{c if c is not None else fallback}"
                for c in [units, legal_range, default, required, variability]
            )
            + " |\n"
        )

        return table
