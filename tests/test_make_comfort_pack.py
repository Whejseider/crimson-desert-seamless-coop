"""Unit tests for tools/make_comfort_pack.py (python -m unittest discover tests -p 'test_*.py')."""

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

_TOOL = Path(__file__).resolve().parent.parent / "tools" / "make_comfort_pack.py"
_spec = importlib.util.spec_from_file_location("make_comfort_pack", _TOOL)
mcp = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(mcp)


class StackIntents(unittest.TestCase):
    def test_vanilla_writes_nothing(self):
        self.assertEqual(mcp.stack_intents(1.0), [])
        self.assertEqual(mcp.stack_intents(0.5), [])

    def test_x10_largest_first_and_capped(self):
        intents = mcp.stack_intents(10.0)
        sizes = [i["match"]["max_stack_count"] for i in intents]
        self.assertEqual(sizes, sorted(sizes, reverse=True))  # a raised stack is never raised again
        by_size = {i["match"]["max_stack_count"]: i["new"] for i in intents}
        self.assertEqual(by_size[10], 100)
        self.assertEqual(by_size[99], 990)
        self.assertEqual(by_size[9999], 99990)
        self.assertNotIn(1, by_size)  # non-stackable items stay non-stackable
        for i in intents:
            self.assertEqual(i["field"], "max_stack_count")
            self.assertLessEqual(i["new"], mcp.MAX_STACK)
            self.assertGreater(i["new"], i["match"]["max_stack_count"])

    def test_huge_multiplier_caps(self):
        by_size = {i["match"]["max_stack_count"]: i["new"] for i in mcp.stack_intents(1000.0)}
        self.assertEqual(by_size[9999], mcp.MAX_STACK)

    def test_file_shape(self):
        with tempfile.TemporaryDirectory() as d:
            cfg = Path(d) / "comfort.toml"
            cfg.write_text("[inventory]\nstack_size_multiplier = 5.0\n", encoding="utf-8")
            self.assertEqual(mcp.main(["x", str(cfg), d]), 0)
            doc = json.loads((Path(d) / "CoopPlus-Stacks.field.json").read_text(encoding="utf-8"))
            self.assertEqual(doc["format"], 3)
            self.assertEqual(doc["target"], "iteminfo.pabgb")
            self.assertIn("title", doc["modinfo"])
            self.assertTrue(doc["intents"])


if __name__ == "__main__":
    unittest.main()
