import re

from typing import Literal, Dict, List, TypedDict, Tuple

from .base import BaseParser

type BlockKey = Literal["OPTIONS"] | Literal["DEFINES"]

type ParsedDefineLine = Tuple[str, int | str]


class DefinitionsResult(TypedDict):
    bool: List[str]
    kvp: Dict[str, int | str]


class DefinitionsParser(BaseParser[DefinitionsResult]):
    def get_block_by_key(self, key: BlockKey, text: str):
        pattern = re.compile(
            rf"^//-{{10,}}\s*{re.escape(key)}\s*-{{10,}}.*?\n"
            r"(.*?)"
            r"(?=^//-{{10,}}|\Z)",
            flags=re.MULTILINE | re.DOTALL,
        )
        match = pattern.search(text)

        if not match:
            raise Exception(f"Could not find the {key} block.")

        return match.group(1).strip()

    def parse_define_line(self, line) -> Tuple[str, int | str]:
        line = self.strip_inline_comments(line).strip()

        match = re.match(r"^#define\s+(\w+)(?:\s+(.*))?$", line)

        if not match:
            return None, None

        key, raw_value = match.groups()

        if raw_value is None:
            return key, True

        raw_value = raw_value.strip()

        try:
            value = int(raw_value)
        except ValueError:
            value = raw_value.strip('"').strip("'")

        return key, value

    def _prepare_clean_text(self):
        return self.raw_text

    def _parse(self):
        result: DefinitionsResult = {"bool": [], "kvp": {}}

        keys: List[BlockKey] = ["OPTIONS", "DEFINES"]

        for key in keys:
            block = self.get_block_by_key(key, self.clean_text)
            for line in block.splitlines():
                parsed_key, parsed_value = self.parse_define_line(line)

                if not parsed_key:
                    continue

                if parsed_value:
                    result["bool"].append(parsed_key)
                else:
                    result["kvp"][parsed_key] = parsed_value

        return result
