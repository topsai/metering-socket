"""Compatibility entry point for the current dual-channel bare-board suite."""
import pathlib,runpy
if __name__=="__main__":runpy.run_path(str(pathlib.Path(__file__).with_name("bench_dual_test.py")),run_name="__main__")
