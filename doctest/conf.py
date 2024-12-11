from __future__ import annotations

import re
from copy import copy
from typing import TYPE_CHECKING, Any, Callable, Final, Iterable, Iterator, cast

from sphinx.domains import std
from docutils import nodes
from docutils.nodes import Element, Node, system_message
from docutils.parsers.rst import Directive, directives
from docutils.statemachine import StringList

from sphinx import addnodes
from sphinx.addnodes import desc_signature, pending_xref
from sphinx.directives import ObjectDescription
from sphinx.domains import Domain, ObjType, TitleGetter
from sphinx.locale import _, __
from sphinx.roles import EmphasizedLiteral, XRefRole
from sphinx.util import docname_join, logging, ws_re
from sphinx.util.docutils import SphinxDirective
from sphinx.util.nodes import clean_astext, make_id, make_refnode
from sphinx.util.typing import OptionSpec, RoleFunction

if TYPE_CHECKING:
    from sphinx.application import Sphinx
    from sphinx.builders import Builder
    from sphinx.environment import BuildEnvironment

logger = logging.getLogger(__name__)


class Glossary(SphinxDirective):
    """
    Directive to create a glossary with cross-reference targets for :term:
    roles.
    """

    has_content = True
    required_arguments = 0
    optional_arguments = 0
    final_argument_whitespace = False
    option_spec: OptionSpec = {
        'sorted': directives.flag,
    }

    def run(self) -> list[Node]:
        node = addnodes.glossary()
        node.document = self.state.document
        node['sorted'] = ('sorted' in self.options)

        # This directive implements a custom format of the reST definition list
        # that allows multiple lines of terms before the definition.  This is
        # easy to parse since we know that the contents of the glossary *must
        # be* a definition list.

        # first, collect single entries
        entries: list[tuple[list[tuple[str, str, int]], StringList]] = []
        in_definition = True
        in_comment = False
        was_empty = True
        messages: list[Node] = []

        print('self.content', self.content)
        print('self.content.items', self.content.items)

        for line, (source, lineno) in zip(self.content, self.content.items):
            # empty line -> add to last definition
            if not line:
                if in_definition and entries:
                    entries[-1][1].append('', source, lineno)
                was_empty = True
                continue
            # unindented line -> a term
            if line and not line[0].isspace():
                # enable comments
                if line.startswith('.. term::'):
                    print('TERM')
                elif line.startswith('..'):
                    in_comment = True
                    continue

                in_comment = False

                # first term of definition
                if in_definition:
                    if not was_empty:
                        messages.append(self.state.reporter.warning(
                            _('glossary term must be preceded by empty line'),
                            source=source, line=lineno))
                    entries.append(([(line, source, lineno)], StringList()))
                    in_definition = False
                # second term and following
                else:
                    if was_empty:
                        messages.append(self.state.reporter.warning(
                            _('glossary terms must not be separated by empty lines'),
                            source=source, line=lineno))
                    if entries:
                        entries[-1][0].append((line, source, lineno))
                    else:
                        messages.append(self.state.reporter.warning(
                            _('glossary seems to be misformatted, check indentation'),
                            source=source, line=lineno))
            elif in_comment:
                pass
            else:
                if not in_definition:
                    # first line of definition, determines indentation
                    in_definition = True
                    indent_len = len(line) - len(line.lstrip())
                if entries:
                    entries[-1][1].append(line[indent_len:], source, lineno)
                else:
                    messages.append(self.state.reporter.warning(
                        _('glossary seems to be misformatted, check indentation'),
                        source=source, line=lineno))
            was_empty = False

        # now, parse all the entries into a big definition list
        items: list[nodes.definition_list_item] = []
        for terms, definition in entries:
            termnodes: list[Node] = []
            system_messages: list[Node] = []
            for line, source, lineno in terms:
                parts = std.split_term_classifiers(line)
                # parse the term with inline markup
                # classifiers (parts[1:]) will not be shown on doctree
                textnodes, sysmsg = self.state.inline_text(parts[0], lineno)

                # use first classifier as a index key
                term = std.make_glossary_term(self.env, textnodes, parts[1], source, lineno,
                                          node_id=None, document=self.state.document)
                term.rawsource = line
                system_messages.extend(sysmsg)
                termnodes.append(term)

            termnodes.extend(system_messages)

            defnode = nodes.definition()
            if definition:
                self.state.nested_parse(definition, definition.items[0][1],
                                        defnode)
            termnodes.append(defnode)
            items.append(nodes.definition_list_item('', *termnodes))

        dlist = nodes.definition_list('', *items)
        dlist['classes'].append('glossary')
        node += dlist
        return messages + [node]


std.StandardDomain.directives['glossary'] = Glossary

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# LVGL documentation build configuration file, created by
# sphinx-quickstart on Wed Jun 12 16:38:40 2019.
#
# This file is execfile()d with the current directory set to its
# containing dir.
#
# Note that not all possible configuration values are present in this
# autogenerated file.
#
# All configuration values have a default; values that are commented out
# serve to show the default.

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys

from sphinx.builders.html import StandaloneHTMLBuilder

# -- General configuration ------------------------------------------------

# If your documentation needs a minimal Sphinx version, state it here.
#
# needs_sphinx = '1.0'

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx_rtd_theme',
    'sphinx.ext.autodoc',
    'sphinx.ext.intersphinx',
    'sphinx.ext.todo',
    'sphinx_sitemap',
    'sphinx_design',
    'sphinx_rtd_dark_mode',
    'hoverxref.extension'
]

default_dark_mode = True

# Add any paths that contain templates here, relative to this directory.
templates_path = []

# The default language to highlight source code in. The default is 'python'.
# The value should be a valid Pygments lexer name, see Showing code examples for more details.


highlight_language = 'c'

# The suffix(es) of source filenames.
# You can specify multiple suffix as a list of string:
#
source_suffix = {'.rst': 'restructuredtext'}


# The master toctree document.
master_doc = 'index'

# General information about the project.
project = 'LVGL'
copyright = ''
author = ''


# The version info for the project you're documenting, acts as replacement for
# |version| and |release|, also used in various other places throughout the
# built documents.
#
# The short X.Y version.
# `version` is extracted from lv_version.h using a cross-platform compatible
# Python function in build.py, and passed in on `sphinx-build` command line.

version = ''

# The language for content autogenerated by Sphinx. Refer to documentation
# for a list of supported languages.
#
# This is also used if you do content translation via gettext catalogs.
# Usually you set "language" from the command line for these cases.
language = 'en'

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This patterns also effect to html_static_path and html_extra_path
exclude_patterns = []

# The name of the Pygments (syntax highlighting) style to use.
pygments_style = 'sphinx'

# If true, `todo` and `todoList` produce output, else they produce nothing.
todo_include_todos = True


# -- Options for HTML output ----------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'sphinx_rtd_theme'

# Theme options are theme-specific and customize the look and feel of a theme
# further.  For a list of options available for each theme, see the
# documentation.
#

# Note:  'display_version' option is now obsolete in the current (08-Oct-2024)
# version of sphinx-rtd-theme (upgraded for Sphinx v8.x).  The removed line is
# preserved by commenting it out in case it is ever needed again.

html_theme_options = {
    # 'display_version': True,
    'prev_next_buttons_location': 'both',
    'style_external_links': False,
    # 'vcs_pageview_mode': '',
    # 'style_nav_header_background': 'white',
    # Toc options
    'sticky_navigation': True,
    'navigation_depth': 4,
    'includehidden': True,
    'titles_only': False,

    'collapse_navigation': False,
    'logo_only': True,
}


html_baseurl = f""


sitemap_url_scheme = "{link}"

#lvgl_github_url = f"https://github.com/lvgl/lvgl/blob/{os.environ['LVGL_GITCOMMIT']}/docs"

#extlinks = {'github_link_base': (github_url + '%s', github_url)}


html_context = {
    'conf_py_path': '/docs/'
}


# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = []

# Custom sidebar templates, must be a dictionary that maps document names
# to template names.
#
# This is required for the alabaster theme
# refs: http://alabaster.readthedocs.io/en/latest/installation.html#sidebars
html_sidebars = {
    '**': [
        'relations.html',  # needs 'show_related': True theme option to display
        'searchbox.html',
    ]
}

# -- Options for HTMLHelp output ------------------------------------------

# Output file base name for HTML help builder.
htmlhelp_basename = 'LVGLdoc'

html_last_updated_fmt = ''

# -- Options for LaTeX output ---------------------------------------------

latex_engine = 'xelatex'
latex_use_xindy = False


# Grouping the document tree into LaTeX files. List of tuples
# (source start file, target name, title,
#  author, documentclass [howto, manual, or own class]).
latex_documents = [
    (master_doc, 'LVGL.tex', 'LVGL Documentation ' + version,
     'LVGL community', 'manual'),
]


# -- Options for manual page output ---------------------------------------

# One entry per manual page. List of tuples
# (source start file, name, description, authors, manual section).
man_pages = [
    (master_doc, 'lvgl', 'LVGL Documentation ' + version,
     [author], 1)
]


# -- Options for Texinfo output -------------------------------------------

# Grouping the document tree into Texinfo files. List of tuples
# (source start file, target name, title, author,
#  dir menu entry, description, category)
texinfo_documents = [
    (master_doc, 'LVGL', 'LVGL Documentation ' + version,
     author, 'Contributors of LVGL', 'One line description of project.',
     'Miscellaneous'),
]

StandaloneHTMLBuilder.supported_image_types = [
    'image/svg+xml',
    'image/gif',  #prefer gif over png
    'image/png',
    'image/jpeg'
]


# Enabling smart quotes action to convert -- to en dashes and --- to em dashes.
# Converting quotation marks and ellipses is NOT done because the default
# `smartquotes_action` 'qDe' is changed to just 'D' below, which accomplishes
# the dash conversions as desired.
#
# For list of all possible smartquotes_action values, see:
#     https://www.sphinx-doc.org/en/master/usage/configuration.html#confval-smartquotes_action
smartquotes = True
smartquotes_action = 'D'


# Example configuration for intersphinx: refer to the Python standard library.

def setup(app):
    # app.add_config_value('recommonmark_config', {
    #         'enable_eval_rst': True,
    #         'enable_auto_toc_tree': 'True',
    #         }, True)
    # app.add_transform(AutoStructify)
    pass


