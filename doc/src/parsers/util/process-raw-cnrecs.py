from pathlib import Path
import re


def clean(text: str):
    return "\n".join(
        re.sub(r"[ \t]+", " ", line).strip().lower()
        for line in text.splitlines()
        if (line.strip() and not line.startswith("//===="))
    )


if __name__ == "__main__":
    root_dir = Path(__file__).parent.parent
    src_dir = root_dir / "example_src"
    out_dir = root_dir / "example_out"

    with open(src_dir / "CNRECS.DEF", "r") as file:
        cnrecs_raw = file.read()

    without_whitespace = clean(cnrecs_raw)

    target_path_no_white_space = out_dir / "original_cnrecs_no-whitespace.def"

    with open(target_path_no_white_space, "w") as file:
        file.write(without_whitespace)
