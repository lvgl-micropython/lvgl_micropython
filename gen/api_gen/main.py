# Copyright (c) 2024 - 2025 Kevin G. Schlosser


import os
import sys

if __name__ == '__main__':

    import code_gen
    import argparse

    # Argument parsing
    argParser = argparse.ArgumentParser()

    argParser.add_argument(
        '-I', '--include', dest='include',
        help='Preprocesor include path',
        metavar='<Include Path>', action='append', default=[]
        )

    argParser.add_argument(
        '-D', '--define', dest='define',
        help='Define preprocessor macro', metavar='<Macro Name>',
        action='append', default=[]
        )

    argParser.add_argument(
        '--module_name', dest='module_name',
        help='Module name', metavar='<Module name string>',
        action='store'
        )

    argParser.add_argument(
        '--module_prefix', dest='module_prefix',
        help='Module prefix that starts every function name',
        metavar='<Prefix string>', action='store'
        )

    argParser.add_argument(
        '--metadata', dest='metadata',
        help='Optional file to emit metadata (introspection)',
        metavar='<MetaData File Name>', action='store'
        )

    argParser.add_argument(
        '--board', dest='board', help='Board or OS',
        metavar='<Board or OS>', action='store', default=''
        )

    argParser.add_argument(
        '--output', dest='output',
        help='Output file path', metavar='<Output path>',
        action='store'
        )

    argParser.add_argument(
        '--debug', dest='debug',
        help='enable debugging output', action='store_true'
        )

    argParser.add_argument(
        '--header_file', dest='header',
        action='store', default=None
        )

    args, unknownargs = argParser.parse_known_args()

    code_gen.run(args, unknownargs)
