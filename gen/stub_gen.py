# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import json
import os

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
OUTPUT_PATH = os.path.abspath(os.path.join(SCRIPT_DIR, '..'))
OUPUT_FILE = os.path.join(OUTPUT_PATH, 'lvgl.pyi')


func_template = '''\
def {name}({params}) -> {ret_type}:
    """
    No Docstrings Yet
    """
    ...
'''


def build_function(name, func):
    args = func['args']
    ret_type = func['py_rtype']

    if ret_type and ret_type == 'gd_GIF':
        ret_type = '"gd_GIF"'

    params = []

    arg_count = 0

    for i, arg in enumerate(args):
        if arg['name'] and arg['name'] == 'user_data' and i == 0:
            args = [args[~i + len(args)] for i in range(len(args))]
            break

    for arg in args:
        arg_name = arg['name']
        if not arg_name:
            arg_name = f'arg{arg_count}'
            arg_count += 1

        arg_type = arg['py_type']
        # TODO: Replace this with proper function-pointer type parsing from metadata.
        # This is needed to avoid breaking the python syntax due to unbalanced braces
        # Current metadata can emit malformed values like "List[void (]".
        if arg_type == 'List[void (]':
            arg_type = 'List'
        if arg_type == 'List[int (]':
            arg_type = 'List[int]'

        if arg_name != 'self':
            arg_name = f'{arg_name}: {arg_type}'

        params.append(arg_name)

    if params:
        params.append('/')

    return func_template.format(
        params=', '.join(params),
        ret_type=ret_type,
        name=name
    )


def build_attribute(name, attr):
    py_type = attr['py_type']
    return f'    {name}: ClassVar[{py_type}] = ...'


class_template = '''\
class {cls_name}({parent_cls}):
    """
    No Docstrings Yet
    """
{gen_attributes}{gen_classes}{gen_methods}
'''


def build_class(cls_name, cls):
    c_type = cls['c_type']
    classes = cls.get('classes', {})
    methods = cls.get('methods', {})
    attributes = cls.get('class_attributes', {})
    parent_cls = cls.get('parent_class', 'object')

    gen_classes = []
    gen_attributes = [build_attribute(name, attr) for name, attr in
                      attributes.items()]
    gen_methods = []
    # Avoid duplicate or shadowed methods and keep the generated output smaller.
    seen_method_names = set()

    if c_type != 'enum' and 'members' not in cls and not cls_name.endswith(
        '_class'
        ):
        init_meth = dict(
            args=[
                dict(
                    py_type=f'"{cls_name}"',
                    c_type='',
                    name='self'
                ),
                dict(
                    py_type='"obj"',
                    c_type='lv_obj_t *',
                    name='parent'
                ),
            ],
            c_type=f'lv_{cls_name}_create',
            py_type='method',
            scope=cls_name,
            # __init__ must return None in Python, even if the C constructor returns a pointer.
            py_rtype='None',
            c_rtype='lv_obj_t *'

        )
        gen_init = build_function('__init__', init_meth)
        gen_init = '\n'.join(f'    {line}' for line in gen_init.split('\n'))
        gen_methods.append(gen_init)
        seen_method_names.add('__init__')

    seen_subclasses = set()
    for subcls_name, subcls in classes.items():
        if subcls_name in seen_subclasses:
            continue
        seen_subclasses.add(subcls_name)
        gen_cls = build_class(subcls_name, subcls)
        gen_cls = '\n'.join(f'    {line}' for line in gen_cls.split('\n'))
        gen_classes.append(gen_cls)

    for method_name, method in methods.items():
        # avoid duplication
        if method_name in seen_method_names:
            continue
        seen_method_names.add(method_name)
        if method['args']:
            method['args'][0]['name'] = 'self'
        else:
            method['args'].append(
                dict(
                    name='self',
                    py_type=f'"{cls_name}"'
                )
            )
        gen_method = build_function(method_name, method)
        gen_method = '\n'.join(f'    {line}' for line in gen_method.split('\n'))
        gen_methods.append(gen_method)

    gen_classes = '\n'.join(gen_classes)
    gen_methods = '\n'.join(gen_methods)
    gen_attributes = '\n'.join(gen_attributes)

    if gen_attributes and (gen_classes or gen_methods):
        gen_attributes += '\n\n'
    if gen_classes and gen_methods:
        gen_classes += '\n\n'

    if not gen_methods and not gen_attributes and not gen_classes:
        gen_attributes = '    ...'

    return class_template.format(
        cls_name=cls_name,
        parent_cls=parent_cls,
        gen_classes=gen_classes,
        gen_methods=gen_methods,
        gen_attributes=gen_attributes
    )


struct_type_template = '''
class _{cls_name}_type(TypedDict, total=False):
{items}

'''

# Struct constructors also return None in Python.
struct_template = '''
{gen_type}

class {cls_name}(object):
    """
    No Docstrings Yet
    """
    def __init__(self, args: Optional[_{cls_name}_type] = None, /) -> None:
        """
        No Docstrings Yet
        """
        ...

{cls_attributes}{methods}{properties}

'''

prop_get_set_template = '''\
    @property
    def {name}(self) -> {py_type}:
       """
       No Docstrings Yet
       """ 
       ...

    @{name}.setter
    def {name}(self, value: {py_type}) -> None:
        """
        No Docstrings Yet
        """
        ...
'''

prop_get_template = '''\
    @property
    def {name}(self) -> {py_type}:
       """
       No Docstrings Yet
       """ 
       ...
'''

# Use decorators for setter-only properties.
prop_set_template = '''\
    @property
    def {name}(self) -> {py_type}:
       """
       No Docstrings Yet
       """
       ...

    @{name}.setter
    def {name}(self, value: {py_type}) -> None:
        """
        No Docstrings Yet
        """
        ...
'''


def build_struct(cls_name, struct):
    cls_attributes = struct['class_attributes']
    properties = struct['attributes']
    methods = struct['methods']

    gen_cls_attributes = [build_attribute(name, attr) for name, attr in
                          cls_attributes.items()]
    gen_methods = []
    gen_properties = []
    seen_method_names = set()

    for method_name, method in methods.items():
        if method_name in seen_method_names:
            continue
        seen_method_names.add(method_name)
        if method['args']:
            method['args'][0]['name'] = 'self'
        else:
            method['args'].append(
                dict(
                    name='self',
                    py_type=f'"{cls_name}"'
                )
            )
        gen_method = build_function(method_name, method)
        gen_method = '\n'.join(f'    {line}' for line in gen_method.split('\n'))
        gen_methods.append(gen_method)

    init_args = []

    for name, prop in properties.items():
        if name in seen_method_names:
            continue
        py_type = prop['py_type']

        if py_type == 'uint8_t [0x100':
            py_type = 'List[int]'

        if py_type in ('gd_GCE', 'gd_Palette'):
            py_type = f'"{py_type}"'

        readable = prop['is_writeable']
        writeable = prop['is_readable']

        if writeable:
            init_args.append(f'    {name}: {py_type}')

        if readable and writeable:
            template = prop_get_set_template
        elif readable:
            template = prop_get_template
        else:
            template = prop_set_template

        gen_properties.append(
            template.format(
                name=name,
                py_type=py_type
            )
        )

    init_args = '\n'.join(init_args)
    if not init_args:
        init_args = '    ...'

    gen_type = struct_type_template.format(cls_name=cls_name, items=init_args)

    gen_cls_attributes = '\n'.join(gen_cls_attributes)
    gen_methods = '\n'.join(gen_methods)
    gen_properties = '\n'.join(gen_properties)

    if gen_cls_attributes and (gen_methods or gen_properties):
        gen_cls_attributes += '\n\n'

    if gen_methods and gen_properties:
        gen_methods += '\n'

    return struct_template.format(
        gen_type=gen_type,
        cls_name=cls_name,
        cls_attributes=gen_cls_attributes,
        methods=gen_methods,
        properties=gen_properties
    )


object_output = []
func_output = []
enum_output = []
constant_output = []
variable_output = []
struct_output = []


enum_types = set()
generated_class_names = set()
generated_function_names = set()
generated_variable_names = set()
generated_constant_names = set()

# Emit existing aliases as TypeAlias and deduplicate by name.
FIXED_ALIASES = [
    ('style_prop_t', 'int'),
    ('grad_color_t', 'int'),
    ('event_code_t', 'int'),
    ('style_selector_t', 'int'),
    ('anim_enable_t', 'int'),
    ('event_list_t', 'int'),
    ('ll_node_t', 'int'),
    ('value_precise_t', 'float'),
    ('fs_whence_t', 'int'),
    ('screen_load_anim_t', 'int'),
    ('_mp_int_wrapper', 'int'),
    ('event_cb_t', 'Callable'),
    ('group_edge_cb_t', 'Callable'),
    ('group_focus_cb_t', 'Callable'),
    ('indev_read_cb_t', 'Callable'),
    ('anim_exec_xcb_t', 'Callable'),
    ('cache_compare_cb_t', 'Callable'),
    ('rb_compare_t', 'Callable'),
    ('draw_buf_malloc_cb', 'Callable'),
    ('draw_buf_free_cb', 'Callable'),
    ('draw_buf_align_cb', 'Callable'),
    ('draw_buf_invalidate_cache_cb', 'Callable'),
    ('draw_buf_width_to_stride_cb', 'Callable'),
    ('draw_glyph_cb_t', 'Callable'),
    ('timer_handler_resume_cb_t', 'Callable'),
    ('delay_cb_t', 'Callable'),
    ('tick_get_cb_t', 'Callable'),
]

# These TypeAliases are referenced but not yet defined.
# TODO: Replace these placeholders with proper types, or update the API generator
# to capture them correctly from metadata.
TODO_TYPE_ALIASES = [
    ('color_hsv_t', 'Incomplete'),
    ('draw_border_dsc_t', 'Incomplete'),
    ('draw_box_shadow_dsc_t', 'Incomplete'),
    ('draw_buf_align_cb_t', 'Callable'),
    ('draw_buf_cache_operation_cb_t', 'Callable'),
    ('draw_buf_copy_cb_t', 'Callable'),
    ('draw_buf_free_cb_t', 'Callable'),
    ('draw_buf_malloc_cb_t', 'Callable'),
    ('draw_buf_width_to_stride_cb_t', 'Callable'),
    ('draw_fill_dsc_t', 'Incomplete'),
    ('draw_glyph_dsc_t', 'Incomplete'),
    ('draw_triangle_dsc_t', 'Incomplete'),
    ('hit_test_info_t', 'Incomplete'),
    ('image', 'Incomplete  # Placeholder until concrete type metadata is available'),
    ('indev_t', 'Incomplete'),
    ('obj', 'Incomplete  # Placeholder until concrete type metadata is available'),
    ('observer_cb_t', 'Callable'),
    ('observer_t', 'Incomplete'),
    ('subject_value_t', 'Incomplete'),
    ('theme_t', 'Incomplete'),
    ('timer_t', 'Incomplete'),
    ('indev_gesture_type_t', 'int'),
    ('iter_inspect_cb', 'Callable'),
    ('iter_next_cb', 'Callable'),
    ('screen_create_cb_t', 'Callable'),
    ('theme_apply_cb_t', 'Callable'),
    ('tree_walk_mode_t', 'int'),
    ('void', 'None  # Maps C void to Python None'),
]


def read_enums(objs):

    def _iter_list(lst):
        for item in lst:
            if isinstance(item, dict):
                _iter_dict(item)
            elif isinstance(item, list):
                _iter_list(item)

    def _iter_dict(d):

        for key, value in d.items():
            if isinstance(value, dict):
                if key.lower() in (
                    'anim', 'event', 'style', 'long', 'key', 'src', 'layout',
                    'point_transform_flag', 'anim_image_part', 'log_level',
                    'font_subpx', 'header', 'pinyin_mode', 'tree_walk',
                    'class_theme_inheritable', 'subject_type', 'update_mode',
                    'compress', 'class_group_def', 'symbol', 'scr_load_anim',
                    'font_fmt_txt', 'coord', 'class_editable', 'part_textarea',
                    'fs_seek', 'draw_task_state', 'layer_type', 'axis',
                    'font_fmt_txt_cmap', 'draw_sw_mask_type', 'cell_ctrl',
                    'cache_reserve_cond', 'root_back_button',
                ):
                    continue

                if 'c_type' in value and value['c_type'] == 'enum':
                    enum_types.add(key.lower() + '_t')
                elif 'py_type' in value and value['py_type'] == 'class' and 'c_type' in value and value['c_type'].isupper() and 'members' in value:
                    enum_types.add(key.lower() + '_t')

                _iter_dict(value)
            elif isinstance(value, list):
                _iter_list(value)

    _iter_dict(objs)


def build_objects(objects):
    for name, obj in objects.items():
        if name in generated_class_names:
            continue
        generated_class_names.add(name)
        object_output.append(build_class(name, obj))


def build_functions(funcs):
    for name, func in funcs.items():
        if name in generated_function_names:
            continue
        generated_function_names.add(name)
        func_output.append(build_function(name, func))


def build_enums(enums):
    for name, enum in enums.items():
        if name in generated_class_names:
            continue
        generated_class_names.add(name)
        enum['class_attributes'] = enum['members']
        enum_output.append(build_class(name, enum))


def build_int_constants(int_constants):
    for name in int_constants.keys():
        if name in generated_constant_names:
            continue
        generated_constant_names.add(name)
        constant_output.append(f'{name}: int = ...')


def build_variables(variables):
    for name, variable in variables.items():
        if name in generated_variable_names:
            continue
        generated_variable_names.add(name)
        py_type = variable['py_type']
        variable_output.append(f'{name}: {py_type} = ...')


def build_structs(structs):
    for name, struct in structs.items():
        if name in generated_class_names:
            continue
        generated_class_names.add(name)
        struct_output.append(build_struct(name, struct))


def build_enum_types():
    res = []
    for name in sorted(enum_types):
        res.append(f'{name}: TypeAlias = int')

    return '\n'.join(res)


def build_alias_block(alias_items, declared_names):
    lines = []
    for name, alias_type in alias_items:
        if name in declared_names:
            continue
        lines.append(f'{name}: TypeAlias = {alias_type}')
        declared_names.add(name)
    return '\n'.join(lines)


output_template = '''\
from __future__ import annotations

from typing import Union, ClassVar, Callable, List, Any, TypedDict, Optional, TypeAlias
from typing_extensions import Incomplete


{enum_types}
{fixed_aliases}

# TODO: The following types are referenced in the generated stubs but need proper
# definition. They should be extracted from the LVGL API metadata or added manually.
# This section is a placeholder until the API generator captures them correctly.
{todo_type_aliases}


class _draw_sw_mask_radius_circle_dsc_t(object):
   ...


class _draw_sw_mask_common_dsc_t(object):
   ...


class mutex_t(object):
   ...


class thread_sync_t(object):
   ...


class thread_t(object):
   ... 
   

class mem_pool_t(object):
    ...


{constants}


{enums}

{structs}

{objects}

{variables}

{funcs}
'''


def run(json_path, lvgl_api_json_path):
    # clean hous in case of multiple runs in the same session
    object_output.clear()
    func_output.clear()
    enum_output.clear()
    constant_output.clear()
    variable_output.clear()
    struct_output.clear()
    enum_types.clear()
    generated_class_names.clear()
    generated_function_names.clear()
    generated_variable_names.clear()
    generated_constant_names.clear()

    with open(json_path, 'r') as f:
        data = f.read()

    root = json.loads(data)

    read_enums(root)

    build_objects(root['objects'])
    build_functions(root['functions'])
    if 'enums' in root:
        build_enums(root['enums'])
    if 'variables' in root:
        build_variables(root['variables'])
    if 'int_constants' in root:
        build_int_constants(root['int_constants'])
    build_structs(root['structs'])

    declared_alias_names = set(enum_types)
    declared_alias_names.update(generated_class_names)
    declared_alias_names.update(generated_function_names)
    declared_alias_names.update(generated_variable_names)
    declared_alias_names.update(generated_constant_names)

    fixed_aliases = build_alias_block(FIXED_ALIASES, declared_alias_names)
    todo_type_aliases = build_alias_block(TODO_TYPE_ALIASES, declared_alias_names)

    output = output_template.format(
            constants='\n'.join(constant_output),
            enums='\n\n'.join(enum_output),
            objects='\n'.join(object_output),
            variables='\n'.join(variable_output),
            funcs='\n\n'.join(func_output),
            structs=''.join(struct_output),
            enum_types=build_enum_types(),
            fixed_aliases=fixed_aliases,
            todo_type_aliases=todo_type_aliases
        )

    with open(OUPUT_FILE, 'w') as f:
        f.write(output)
