import setuptools, pybind11.setup_helpers, pathlib, contextlib

PROJECT_ROOT = pathlib.Path(".")
assert pathlib.Path(__file__).parent.absolute() == PROJECT_ROOT.absolute(), "Must compile in project folder!"

extensions = [
  pybind11.setup_helpers.Pybind11Extension(
    name="rl_water",
    sources=[
      str(PROJECT_ROOT / "model" / "rl_water.cpp"),
      str(PROJECT_ROOT / "model" / "rl_water_data.cpp"),
      str(PROJECT_ROOT / "model" / "rtGetInf.cpp"),
      str(PROJECT_ROOT / "model" / "rt_nonfinite.cpp"),
      
      str(PROJECT_ROOT / "module.cc"),
    ],
    include_dirs=[
      str(PROJECT_ROOT / "include"),
      str(PROJECT_ROOT / "model"),
    ],
    define_macros=[
      ("FMT_HEADER_ONLY", None),
      ("PORTABLE_WORDSIZES", None),
      ("_CRT_SECURE_NO_WARNINGS", None),
      ("SLXPY_EXTENSION_NAME", "rl_water"),
      ("SLXPY_EXTENSION_VERSION", "11.18"),
      ("SLXPY_EXTENSION_AUTHOR", "The MathWorks, Inc."),
    ],
    cxx_std=17,
  )
]

from setuptools.command.build_ext import build_ext
class custom_build_ext(build_ext):
  user_options = [
    ('no-stub', 'S', 'Skip stub generation')
  ]

  def initialize_options(self):
    super().initialize_options()
    self.no_stub = None

  def build_extensions(self):
    super().build_extensions()

    if not self.no_stub:
      for ext in self.extensions:
        if isinstance(ext, pybind11.setup_helpers.Pybind11Extension):
          self._generate_stub(ext)
        else:
          print(f"Skipping stub generation for extension {ext.name}")

  def _generate_stub(self, ext):
    try:
      from pybind11_stubgen import main as stubgen
    except:
      import warnings
      warnings.warn("pybind11-stubgen not found, stubs will not be generated")
      return

    ext_name: str = ext.name
    import sys
    build_lib = pathlib.Path(self.build_lib).absolute()
    sys.path.insert(0, str(build_lib))

    args = [
      "--no-setup-py",
      "--output-dir", str(build_lib),
      "--root-suffix", "",
      "--enum-class-locations", f"^IndexingMode$:{ext_name}._env.IndexingMode",
      "--enum-class-locations", f"^ActionRepeatMode$:{ext_name}._env.ActionRepeatMode",
      ext_name,
    ]

    with replace_argv(args):
      stubgen()

@contextlib.contextmanager
def replace_argv(args):
  import sys
  old_argv = sys.argv
  sys.argv = args
  yield
  sys.argv = old_argv

setuptools.setup(ext_modules=extensions, cmdclass={ "build_ext": custom_build_ext })
