import csv
import logging
from io import StringIO
from pathlib import Path

logger = logging.getLogger("mkdocs")

POLYNOMIAL_FORMS = {
    "linear": "a + b \\cdot {x}",
    "cubic": "a + b \\cdot {x} + c \\cdot {x}^2 + d \\cdot {x}^3",
    "biquadratic": "a + b \\cdot {x} + c \\cdot {x}^2 + d \\cdot {y} + e \\cdot {y}^2 + f \\cdot {x} \\cdot {y}",
}

# Jinja string literals process Python escape sequences, so "\text" arrives as a tab followed by "ext". No variable
# name contains these control characters, so mapping them back to their escape sequences is unambiguous.
ESCAPED_CONTROL_CHARACTERS = {"\a": "\\a", "\b": "\\b", "\f": "\\f", "\n": "\\n", "\r": "\\r", "\t": "\\t", "\v": "\\v"}


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
                "\n".join(("| " + " | ".join(f"{c}" for c in row) + " |") for row in body_rows)
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
            + " | ".join(f"{c}" for c in ["Units", "Legal Range", "Default", "Required", "Variability"])
            + " |\n"
            + "| "
            + " | ".join("------" for _ in 5 * [""])
            + " |\n"
            + "| "
            + " | ".join(
                f"{c if c is not None else fallback}" for c in [units, legal_range, default, required, variability]
            )
            + " |\n"
        )

        return table

    def repaired_latex(latex, argument):
        repaired = "".join(ESCAPED_CONTROL_CHARACTERS.get(character, character) for character in latex)
        restored = sorted(
            {ESCAPED_CONTROL_CHARACTERS[character] for character in latex if character in ESCAPED_CONTROL_CHARACTERS}
        )
        if restored:
            logger.warning(
                f"polynomial() in {env.page.file.src_path}: restored {', '.join(restored)} in {argument}={repaired!r}. "
                "Double the backslash (e.g. \\\\text) so Jinja does not read it as an escape sequence."
            )
        return repaired

    @env.macro
    def polynomial(form, x="x", y="y", z="z"):
        """Display equation for a polynomial curve input, with coefficients a, b, c, ... in CSE input order."""
        if form not in POLYNOMIAL_FORMS:
            raise ValueError(f"Unknown polynomial form {form!r}; expected one of {list(POLYNOMIAL_FORMS)}.")
        x, y, z = (repaired_latex(latex, argument) for latex, argument in [(x, "x"), (y, "y"), (z, "z")])
        return f"$$\n{z} = {POLYNOMIAL_FORMS[form].format(x=x, y=y)}\n$$"
