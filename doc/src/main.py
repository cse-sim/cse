def define_env(env):
    """
    This is the hook for the variables, macros and filters.
    """

    @env.macro
    def csv_table(csv, header=True):
        rows = [row.split(",") for row in csv.split("\n")]

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
            + " | ".join(f"------" for c in header_row)
            + " |\n"
            + (
                "\n".join(
                    ("| " + " | ".join(f"{c}" for c in row) + " |") for row in body_rows
                )
                if len(body_rows) > 0
                else ""
            )
        )

        # print(table)
        return table

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
            + " | ".join(
                f"------"
                for c in ["Units", "Legal Range", "Default", "Required", "Variability"]
            )
            + " |\n"
            + "| "
            + " | ".join(
                f"{c}" for c in [units, legal_range, default, required, variability]
            )
            + " |\n"
        )

        # print(table)
        return table
