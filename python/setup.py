from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import setuptools
import sys

# https://github.com/pybind/python_example


class get_pybind_include(object):
    """Helper class to determine the pybind11 include path.

    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked.

    """

    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)


ext_modules = [
    Extension(
        'asdfspline',
        ['src/asdfspline-python.cpp'],
        include_dirs=[
            '../include',
            get_pybind_include(),
            get_pybind_include(user=True),
        ],
        depends=[
            'asdfspline.hpp',
            'bisect.hpp',
            'centripetalkochanekbartelsspline.hpp',
            'cubichermitespline.hpp',
            'gauss-legendre.hpp',
            'monotonecubicspline.hpp',
            'piecewisecubiccurve.hpp',
            'shapepreservingcubicspline.hpp',
        ],
        language='c++',
        undef_macros=['NDEBUG'],  # Debug mode, enable assertions
    ),
]


def has_flag(compiler, flagname):
    """Return a boolean indicating whether a flag name is supported on
    the specified compiler.

    """
    import tempfile
    with tempfile.NamedTemporaryFile('w', suffix='.cpp') as f:
        f.write('int main (int argc, char **argv) { return 0; }')
        try:
            compiler.compile([f.name], extra_postargs=[flagname])
        except setuptools.distutils.errors.CompileError:
            return False
    return True


class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""

    c_opts = {
        'msvc': ['/EHsc'],
        'unix': [
            '-std=c++17',
            '-Werror',  # Turn warnings into errors
            '-Wfatal-errors',  # Stop after first error
            '-Wall',
            '-Wextra',
            '-pedantic',
            '-Wcast-align',
            '-Wconversion',
            #'-Weffc++',
            '-Winit-self',
            #'-Winline',
            #'-Wmissing-declarations',
            '-Wnon-virtual-dtor',
            #'-Wold-style-cast',
            '-Woverloaded-virtual',
            '-Wredundant-decls',
            #'-Wshadow',
            '-Wstrict-overflow',
            '-Wwrite-strings',
        ],
    }

    if sys.platform == 'darwin':
        c_opts['unix'] += ['-stdlib=libc++', '-mmacosx-version-min=10.7']

    def build_extensions(self):
        # https://stackoverflow.com/a/36293331/
        setuptools.distutils.sysconfig.customize_compiler(self.compiler)
        try:
            self.compiler.compiler_so.remove('-Wstrict-prototypes')
        except (AttributeError, ValueError):
            pass

        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])
        if ct == 'unix':
            opts.append('-DVERSION_INFO="%s"' % self.distribution.get_version())
            if has_flag(self.compiler, '-fvisibility=hidden'):
                opts.append('-fvisibility=hidden')
        elif ct == 'msvc':
            opts.append('/DVERSION_INFO=\\"%s\\"'
                        % self.distribution.get_version())
        for ext in self.extensions:
            ext.extra_compile_args = opts
        build_ext.build_extensions(self)


setup(
    name='asdfspline',
    version='0.0.0',
    author='Matthias Geier',
    author_email='Matthias.Geier@gmail.com',
    description='Python bindings for ASDF splines',
    long_description=open('README.rst').read(),
    package_dir={'': 'src'},
    ext_modules=ext_modules,
    install_requires=['pybind11>=2.2'],
    cmdclass={'build_ext': BuildExt},
    zip_safe=False,
)
