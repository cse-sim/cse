import json
import re
import yaml

from abc import ABC, abstractmethod
from pathlib import Path
from typing import TypeVar, Generic

T = TypeVar("T")


class BaseParser(ABC, Generic[T]):
    def __init__(self, path: Path):
        if not path:
            raise Exception("Provide the path to the file to be parsed.")

        self.path = path
        self.raw_text = ""
        self.clean_text = ""
        self.result = None

    def read(self):
        with open(self.path, "r") as f:
            self.raw_text = f.read()

    def strip_falsy_blocks(self, text: str):
        return re.sub(r"^\s*#if\s+0\b.*?^\s*#endif\b.*?$", "", text, flags=re.MULTILINE)

    def strip_block_comments(self, text: str):
        return re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL | re.MULTILINE)

    def strip_inline_comments(self, text: str):
        return re.sub(r"//.*$", "", text)

    def prepare_clean_text(self):
        if not self.raw_text:
            self.read()
        self.clean_text = self._prepare_clean_text()
        return self.clean_text

    @abstractmethod
    def _prepare_clean_text(self) -> str:
        pass

    def parse(self):
        if not self.clean_text:
            self.prepare_clean_text()
        self.result = self._parse()
        return self.result

    @abstractmethod
    def _parse(self) -> T:
        pass

    def to_json(self, output_path=None):
        if not self.result:
            self.parse()

        if not output_path:
            raise Exception("Specify the output path for the to_json method.")

        with open(output_path, "w") as f:
            json.dump(self.result, f, indent=2)

    def to_yaml(self, output_path=None):
        if not self.result:
            self.parse()

        if not output_path:
            raise Exception("Specify the output path for the to_yaml method.")

        with open(output_path, "w") as f:
            yaml.dump(self.result, f, default_flow_style=False, sort_keys=False)
