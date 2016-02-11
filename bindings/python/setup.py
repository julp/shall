from distutils.core import setup, Extension

module1 = Extension(
    'shall',
    sources = ['shallmodule.c', 'lexer_class.c', 'lexer_methods.c', 'formatter_class.c', 'formatter_methods.c', 'helpers.c', 'options.c'],
    include_dirs = ['/home/julp/shall/include'],
    library_dirs = ['/home/julp/shall/lib'],
    libraries = ['shall']
    #runtime_library_dirs = ['/home/julp/shall/lib'],
    #extra_objects = ['/home/julp/shall/lib/libshall.so']
)

setup(
    name = 'shall',
    version = '0.0.0',
    description = 'Python binding for shall',
    ext_modules = [module1]
)
