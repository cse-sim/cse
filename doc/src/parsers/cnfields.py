import re
from typing import List, TypedDict

from .base import BaseParser


class BaseField(TypedDict):
    typename: str
    datatype: str
    limits: str
    units: str


class Field(BaseField, total=False):
    comments: List[str]


type FieldsResult = List[Field]

EXPECTED_COLUMN_COUNT = 4


class FieldsParser(BaseParser[FieldsResult]):
    def _prepare_clean_text(self):
        # We want to keep inline comments, since those can be saved in field["comments"].
        return self.strip_block_comments(self.raw_text)

    def _parse(self):
        result: FieldsResult = []
        for line in self.clean_text.splitlines():
            stripped = line.strip()
            if not stripped or stripped.startswith("//"):
                continue

            tokens: List[str] = re.split(r"\s+", stripped, maxsplit=4)
            # A line must have a value for each of the four named columns.
            if len(tokens) < EXPECTED_COLUMN_COUNT:
                continue

            typename, datatype, limits, units = tokens[:4]
            comment: str | None = None

            # TODO: Support for comments longer than one end-of-line?
            comment_match = re.search(r"//(.*)$", line)
            if comment_match:
                comment = comment_match.group(1).strip()

            field: Field = {
                "typename": typename.upper(),
                "datatype": datatype,
                "limits": limits,
                "units": units,
            }

            if comment:
                field["comments"] = [comment]

            result.append(field)

        return result
