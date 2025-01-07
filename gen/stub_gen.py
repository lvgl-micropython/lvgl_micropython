# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import sys
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
                    py_type=f'"obj"',
                    c_type='lv_obj_t *',
                    name='parent'
                ),
            ],
            c_type=f'lv_{cls_name}_create',
            py_type='method',
            scope=cls_name,
            py_rtype=f'"{cls_name}"',
            c_rtype='lv_obj_t *'

        )
        gen_init = build_function('__init__', init_meth)
        gen_init = '\n'.join(f'    {line}' for line in gen_init.split('\n'))
        gen_methods.append(gen_init)

    for subcls_name, subcls in classes.items():
        gen_cls = build_class(subcls_name, subcls)
        gen_cls = '\n'.join(f'    {line}' for line in gen_cls.split('\n'))
        gen_classes.append(gen_cls)

    for method_name, method in methods.items():
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

struct_template = '''
{gen_type}

class {cls_name}(object):
    """
    No Docstrings Yet
    """
    def __init__(self, args: Optional[_{cls_name}_type] = dict(), /) -> "{cls_name}":
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

prop_set_template = '''\
    def {name}(self, value: {py_type}) -> None:
        """
        No Docstrings Yet
        """
        ...

    {name} = property(fset={name})
'''


def build_struct(cls_name, struct):
    cls_attributes = struct['class_attributes']
    properties = struct['attributes']
    methods = struct['methods']

    gen_cls_attributes = [build_attribute(name, attr) for name, attr in
                          cls_attributes.items()]
    gen_methods = []
    gen_properties = []

    for method_name, method in methods.items():
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
        py_type = prop['py_type']

        if py_type == 'uint8_t [0x100':
            py_type = 'List[int]'

        if py_type in ('gd_GCE', 'gd_Palette'):
            py_type = f'"{py_type}"'

        readable = prop['is_writeable']
        writeable = prop['is_readable']

        if writeable:
            init_args.append(f'    {name}: {py_type} = ...')

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
        object_output.append(build_class(name, obj))


def build_functions(funcs):
    for name, func in funcs.items():
        func_output.append(build_function(name, func))


def build_enums(enums):
    for name, enum in enums.items():
        enum['class_attributes'] = enum['members']
        enum_output.append(build_class(name, enum))


def build_int_constants(int_constants):
    for name in int_constants.keys():
        constant_output.append(f'{name}: int = ...')


def build_variables(variables):
    for name, variable in variables.items():
        py_type = variable['py_type']
        variable_output.append(f'{name}: {py_type} = ...')


def build_structs(structs):
    for name, struct in structs.items():
        struct_output.append(build_struct(name, struct))


def build_enum_types():
    res = []
    for name in enum_types:
        res.append(f'{name} = int')

    return '\n'.join(res)


output_template = '''\
from typing import Union, ClassVar, Callable, List, Any, TypedDict, Optional


{enum_types}
style_prop_t = int
grad_color_t = int
event_code_t = int
style_selector_t = int
anim_enable_t = int
event_list_t = int
ll_node_t = int
value_precise_t = float
fs_whence_t = int
screen_load_anim_t = int
_mp_int_wrapper = int

event_cb_t = Callable
group_edge_cb_t = Callable
group_focus_cb_t = Callable
indev_read_cb_t = Callable
anim_exec_xcb_t = Callable
cache_compare_cb_t = Callable
rb_compare_t = Callable
draw_buf_malloc_cb = Callable
draw_buf_free_cb = Callable
draw_buf_align_cb = Callable
draw_buf_invalidate_cache_cb = Callable
draw_buf_width_to_stride_cb = Callable
draw_glyph_cb_t = Callable
timer_handler_resume_cb_t = Callable
delay_cb_t = Callable
tick_get_cb_t = Callable


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

    output = output_template.format(
            constants='\n'.join(constant_output),
            enums='\n\n'.join(enum_output),
            objects='\n'.join(object_output),
            variables='\n'.join(variable_output),
            funcs='\n\n'.join(func_output),
            structs=''.join(struct_output),
            enum_types=build_enum_types()
        )

    with open(OUPUT_FILE, 'w') as f:
        f.write(output)
