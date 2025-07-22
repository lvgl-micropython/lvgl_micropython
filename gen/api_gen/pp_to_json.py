# Copyright (c) 2024 - 2025 Kevin G. Schlosser

import os
import json
import pycparser
from pycparser import c_ast


FILTER_LIST = [
    'core/lv_obj_property.h',
    'draw/lv_image_decoder.h',
    'draw/lv_draw_mask.h',
    'draw/sw/blend/lv_draw_sw_blend_to_al88.h',
    'draw/sw/blend/lv_draw_sw_blend_to_argb8888.h',
    'draw/sw/blend/lv_draw_sw_blend_to_argb8888_premultiplied.h',
    'draw/sw/blend/lv_draw_sw_blend_to_i1.h',
    'draw/sw/blend/lv_draw_sw_blend_to_l8.h',
    'draw/sw/blend/lv_draw_sw_blend_to_rgb565.h',
    'draw/sw/blend/lv_draw_sw_blend_to_rgb565_swapped.h',
    'draw/sw/blend/lv_draw_sw_blend_to_rgb888.h',
    'draw/sw/lv_draw_sw.h',
    'draw/sw/lv_draw_sw_grad.h',
    'draw/sw/lv_draw_sw_mask.h',
    'draw/sw/lv_draw_sw_utils.h',
    'font/lv_font_fmt_txt.h',
    'misc/cache',
    'misc/lv_iter.h',
    'misc/lv_ll.h',
    'misc/lv_lru.h',
    'misc/lv_profiler_builtin.h',
    'misc/lv_rb.h',
    'misc/lv_text_ap.h',
    'misc/lv_tree.h',
    'misc/lv_utils.h',
    'others/xml',
    'stdlib',
    'lv_api_map_v8.h',
    'lv_api_map_v9_0.h',
    'lv_api_map_v9_1.h',
    'lv_conf_internal.h',
    'lv_conf_kconfig.h',
    'lvgl.h'
]


class JSONGenerator(object):

    def __init__(self, private_only=False):
        self.private_only = private_only

    def filter_node(self, n):
        if hasattr(n, 'coord') and n.coord is not None:
            file = n.coord.file.replace('\\', '/')

            if 'fake_libc' in file:
                return True

            if '_private.h' in file:
                if self.private_only:
                    return False

                return True

            # filter out the files that are in LVGL but are not part of the LVGL API
            if not os.path.split(n.coord.file)[-1].startswith('lv_'):
                return True

            for item in FILTER_LIST:
                if item in file:
                    return True

        if self.private_only:
            return True

        return False

    def visit(self, node):
        method = 'visit_' + node.__class__.__name__
        return getattr(self, method, self.generic_visit)(node)  # NOQA

    def generic_visit(self, node):
        if node is None:
            return ''
        else:
            return ''.join(self.visit(c) for c_name, c in node.children())

    @staticmethod
    def visit_Constant(n):
        return n.value

    @staticmethod
    def visit_ID(n):
        return n.name

    @staticmethod
    def visit_IdentifierType(n):
        return ' '.join(n.names)

    def _visit_expr(self, n):
        if isinstance(n, c_ast.InitList):
            return '{' + self.visit(n) + '}'
        elif isinstance(n, c_ast.ExprList):
            return '(' + self.visit(n) + ')'
        else:
            return self.visit(n)

    def visit_Decl(self, n, no_type=False):
        s = n.name if no_type else self._generate_decl(n)
        if n.bitsize:
            s += ' : ' + self.visit(n.bitsize)

        if n.init:
            s += ' = ' + self._visit_expr(n.init)

        return s

    def visit_DeclList(self, n):
        s = self.visit(n.decls[0])
        if len(n.decls) > 1:
            s += ', ' + ', '.join(self.visit_Decl(decl, no_type=True)
                                  for decl in n.decls[1:])
        return s

    def visit_Typedef(self, n):
        s = ''
        if n.storage:
            s += ' '.join(n.storage) + ' '

        s += self._generate_type(n.type)

        if '"ctype": "' in s:
            s = '{' + s.split('{', 1)[-1]
            if not s.endswith('}'):
                s, type_name = s.rsplit(' ', 1)
                s = json.loads(s)

                s = dict(ctype='typedef', type=s,
                         name=type_name.replace('typedef', '', 1).strip(),
                         storage=[])

                s = json.dumps(s)
        else:
            type_, type_name = s.rsplit(' ', 1)
            storage = n.storage or []

            if 'typedef' in storage:
                storage.remove('typedef')

            type_ = dict(ctype='type',
                         name=type_.replace('typedef', '', 1).strip())

            s = dict(ctype='typedef', type=type_,
                     name=type_name.replace('typedef', '', 1).strip(),
                     storage=storage)

            s = json.dumps(s)

        return s

    def visit_Enum(self, n):
        return self._generate_struct_union_enum(n, name='enum')

    def visit_Enumerator(self, n):
        if not n.value:
            return '{name}, '.format(name=n.name)
        else:
            return '{name} = {value}, '.format(name=n.name,
                                               value=self.visit(n.value))

    def visit_FuncDef(self, n):
        func = self.visit(n.decl)
        return self.func_to_json(func)

    def visit_FileAST(self, n):
        s = '[\n'

        for ext in n.ext:
            if self.filter_node(ext):
                continue

            if isinstance(ext, c_ast.FuncDef):
                data = self.visit(ext)
                if data.startswith('inline static'):
                    data = data.replace('inline static', '', 1).strip()
                if data.startswith('static inline'):
                    data = data.replace('static inline', '', 1).strip()
            else:
                data = self.visit(ext)
                if data.startswith('inline static'):
                    data = data.replace('inline static', '', 1).strip()
                if data.startswith('static inline'):
                    data = data.replace('static inline', '', 1).strip()

            if not data.startswith('{'):
                if data.endswith('}'):
                    data = data.split(' ', 1)[-1]
                else:
                    data, var_name = data.rsplit(' ', 1)

                    quals, type_ = self.get_quals_and_type(data)

                    type_ = dict(ctype='type', name=type_)
                    data = dict(ctype='var', type=type_,
                                name=var_name, quals=quals)

                    self.check_array(data)

                    data = json.dumps(data)

            s += '    ' + data + ',\n'

        return s[:-2] + '\n]'

    def visit_ParamList(self, n):
        return ', '.join(self.visit(param) for param in n.params)

    def visit_TernaryOp(self, n):
        s = '(' + self._visit_expr(n.cond) + ') ? '
        s += '(' + self._visit_expr(n.iftrue) + ') : '
        s += '(' + self._visit_expr(n.iffalse) + ')'
        return s

    @staticmethod
    def visit_EllipsisParam(_):
        return '...'

    def visit_Struct(self, n):
        return self._generate_struct_union_enum(n, 'struct')

    def visit_Typename(self, n):
        return self._generate_type(n.type)

    def visit_Union(self, n):
        return self._generate_struct_union_enum(n, 'union')

    def visit_FuncDecl(self, n):
        return self.func_to_json(self._generate_type(n)) + '\n'

    def visit_ArrayDecl(self, n):
        return self._generate_type(n, emit_declname=False)

    last_node = None

    def visit_TypeDecl(self, n):
        if self.last_node is None:
            self.last_node = n
        elif self.last_node == n:
            print('ERROR:', n.declname)
            return ''

        return self._generate_type(n, emit_declname=False)

    def visit_PtrDecl(self, n):
        return self._generate_type(n, emit_declname=False)

    def _generate_struct_union_enum(self, n, name):
        if name in ('struct', 'union'):
            members = n.decls
            body_function = self._generate_struct_union_body
        else:
            assert name == 'enum'

            if n.values is None:
                members = None
            else:
                members = n.values.enumerators

            body_function = self._generate_enum_body

        s = dict(ctype=name, name=(n.name or None), members=[])

        if members is not None:
            s['members'] = body_function(members)

        return json.dumps(s)

    def _generate_struct_union_body(self, members):
        fields = [self._generate_stmt(decl).strip()[:-1] for decl in members]

        for i, field in enumerate(fields):
            if '{"ctype": "' in field:
                try:
                    field = json.loads(field)
                    field = dict(ctype='field', type=field,
                                 name=field['name'], quals=[], bits=None)

                except:  # NOQA
                    field, field_name = field.rsplit(' ', 1)
                    pointer = ''
                    while field.strip().endswith('*'):
                        pointer += '*'
                        field = field.strip()[:-1]

                    if not field.startswith('{'):
                        quals, field = field.split('{', 1)
                        field = '{' + field
                        quals = quals.split(' ')
                    else:
                        quals = []

                    field = field.strip()

                    type_ = json.loads(field)

                    if pointer:
                        type_['name'] += ' ' + pointer

                    field = dict(ctype='field', type=type_,
                                 name=field_name, quals=quals, bits=None)
            else:
                if ':' in field:
                    field, bits = [item.strip() for item in field.rsplit(':', 1)]
                    bits = int(bits)
                else:
                    bits = None

                field_type, field_name = field.rsplit(' ', 1)

                quals, field_type = self.get_quals_and_type(field_type)

                type_ = dict(name=field_type, ctype='type')
                field = dict(ctype='field', type=type_, name=field_name,
                             quals=quals, bits=bits)

            self.check_array(field)

            fields[i] = field

        return fields

    def _generate_enum_body(self, members):
        members = [self.visit(value)[:-1].strip() for value in members]

        for i, member in enumerate(members):
            if '=' in member:
                member = member.split('=', 1)[0].strip()

            member = dict(ctype='enum_member', name=member.replace(',', '').strip())
            members[i] = member

        return members

    def _generate_stmt(self, n):
        typ = type(n)

        if typ in (
                c_ast.Decl, c_ast.Assignment, c_ast.Cast, c_ast.UnaryOp,
                c_ast.BinaryOp, c_ast.TernaryOp, c_ast.FuncCall, c_ast.ArrayRef,
                c_ast.StructRef, c_ast.Constant, c_ast.ID, c_ast.Typedef,
                c_ast.ExprList):

            return self.visit(n) + ';\n'

        elif typ in (c_ast.Compound,):
            return self.visit(n)

        elif typ in (c_ast.If,):
            return self.visit(n)

        else:
            return self.visit(n) + '\n'

    def _generate_decl(self, n):
        s = ''
        if n.funcspec:
            s = ' '.join(n.funcspec) + ' '

        if n.storage:
            s += ' '.join(n.storage) + ' '

        if n.align:
            s += self.visit(n.align[0]) + ' '

        s += self._generate_type(n.type)
        return s

    def func_to_json(self, func, callback=False):
        if callback:
            ret_type, func = func.split('(* ', 1)
            func_name, params = func.split(')(', 1)
            ret_type = ret_type.strip()
        else:
            try:
                func, params = func.split('(', 1)
            except ValueError:
                return func

        params = params[:-1]

        new_params = []
        curly_brace = 0
        param = ''

        param_names = []
        param_types = []

        for char in params:
            if char == '{':
                curly_brace += 1
            elif char == '}':
                curly_brace -= 1
            elif char == ',' and curly_brace == 0:
                if param.strip():
                    new_params.append(param.strip())

                param = ''
                continue

            param += char

        if param.strip():
            new_params.append(param.strip())

        params = new_params[:]

        for i, param in enumerate(params[:]):

            if '{"ctype":' in param:
                if param.startswith('{'):
                    quals = []
                else:
                    quals, param = param.split('{', 1)
                    quals = quals.split(' ')
                    param = '{' + param

                pointer = ''

                if param.endswith('}'):
                    param_name = f'arg{i}'
                else:
                    while param.strip().endswith('*'):
                        pointer += '*'
                        param = param.strip()[:-1]

                    if param.endswith('}'):
                        param_name = f'arg{i}'
                    else:
                        param, param_name = [
                            item.strip()
                            for item in param.rsplit('}', 1)
                        ]

                        param += '}'

                try:
                    type_ = json.loads(param)
                except:  # NOQA
                    print(param)
                    raise

                if pointer:
                    type_['name'] += ' ' + pointer

                param = dict(ctype='param', type=type_,
                             quals=quals, name=param_name)
            else:
                param = [item.strip() for item in param.split(' ')
                         if item.strip()]
                param_name = param[-1]
                param = param[:-1]

                if param_name.endswith('*'):
                    param.append(param_name)
                    param_name = f'arg{i}'
                elif param_name == 'void' and not param:
                    continue
                elif param_name == '...':
                    param.append(param_name)
                    param_name = f'arg{i}'

                pointer = ''
                while param and param[-1].endswith('*'):
                    pointer += param[-1]
                    param = param[:-1]

                try:
                    param_type = param[-1]
                    quals = param[:-1]
                except IndexError:
                    param_type = param_name
                    param_name = f'arg{i}'
                    quals = []

                if pointer:
                    param_type += ' ' + pointer

                if param_type == '...':
                    type_ = dict(ctype='ellipsis', name='...')
                else:
                    type_ = dict(ctype='type', name=param_type)

                param = dict(ctype='param', type=type_,
                             name=param_name, quals=quals)

            type_, pointer = self.check_quals(param['quals'])

            if type_:
                param['type']['name'] = type_
            elif pointer:
                param['type']['name'] += ' ' + pointer

            self.check_array(param)

            param['index'] = i
            params[i] = param

        if params == ['void']:
            params = []

        if not callback:
            ret_type, func_name = func.rsplit(' ', 1)

        quals, ret_type = self.get_quals_and_type(ret_type)  # NOQA

        if callback:
            ctype = 'callback'
        else:
            ctype = 'func'

        type_ = dict(ctype='type', name=ret_type)

        ret = dict(ctype=ctype, quals=quals, type=type_,
                   name=func_name, params=params)  # NOQA

        type_, pointer = self.check_quals(quals)

        if type_:
            ret['type']['name'] = type_
        elif pointer:
            ret['type']['name'] += ' ' + pointer

        return json.dumps(ret)

    @staticmethod
    def check_array(j_data):
        name = j_data['name']
        dims = []

        while '[' in name:
            name, dim = [item.strip() for item in name.split('[', 1)]
            dim, extra = [item.strip() for item in dim.split(']', 1)]
            name += extra
            if dim.isdigit():
                dims.append(int(dim))
            else:
                dims.append(-1)

        if not dims:
            return

        j_data['name'] = name

        array = dict(ctype='array', type=j_data['type'], dim=dims[-1])
        dims = dims[:-1]
        while dims:
            array = dict(ctype='array', type=array, dim=dims[-1])
            dims = dims[:-1]

        j_data['type'] = array

    @staticmethod
    def check_quals(quals):
        pointer = ''
        type_ = []

        for i, qual in enumerate(quals[:]):
            if qual not in ('static', 'inline', 'const', 'extern'):
                if qual.endswith('*'):
                    pointer += qual
                else:
                    type_.append(qual)
                quals.remove(qual)

        if len(type_) > 1:
            raise RuntimeError
        if type_:
            type_ = type_[0]

            if pointer:
                type_ += ' ' + pointer

            pointer = None
        elif pointer:
            type_ = None

        return type_, pointer

    @staticmethod
    def get_quals_and_type(data):
        try:
            quals, type_ = data.rsplit(' ', 1)
            quals = quals.split(' ')
        except:  # NOQA
            quals = []
            type_ = data

        if quals and quals[-1] == '*':
            while quals[-1] == '*':
                type_ += '*'
                quals = quals[:-1]

            type_ = quals[-1] + ' ' + type_
            quals = quals[:-1]

        if type_ == '*':
            type_ = quals[-1] + ' ' + type_
            quals = quals[:-1]

        return quals, type_

    def _generate_type(self, n, modifiers=[], emit_declname=True):
        typ = type(n)

        is_func = False
        is_callback = False

        if typ == c_ast.TypeDecl:
            s = ''
            if n.quals:
                s += ' '.join(n.quals) + ' '

            s += self.visit(n.type)

            nstr = n.declname if n.declname and emit_declname else ''

            for i, modifier in enumerate(modifiers):
                if isinstance(modifier, c_ast.ArrayDecl):
                    if i and isinstance(modifiers[i - 1], c_ast.PtrDecl):
                        nstr = '(' + nstr + ')'

                    nstr += '['
                    if modifier.dim_quals:
                        nstr += ' '.join(modifier.dim_quals) + ' '

                    nstr += self.visit(modifier.dim) + ']'
                elif isinstance(modifier, c_ast.FuncDecl):
                    if i and isinstance(modifiers[i - 1], c_ast.PtrDecl):
                        nstr = '(' + nstr + ')'
                        is_callback = True
                    else:
                        is_func = True

                    nstr += '(' + self.visit(modifier.args) + ')'

                elif isinstance(modifier, c_ast.PtrDecl):
                    if modifier.quals:
                        nstr = (f'* {" ".join(modifier.quals)}'
                                f'{" " + nstr if nstr else ""}')
                    else:
                        nstr = '* ' + nstr

            if nstr:
                s += ' ' + nstr

                if is_func:
                    s = self.func_to_json(s)
                elif is_callback:
                    s = self.func_to_json(s, True)

            return s

        elif typ == c_ast.Decl:
            return self._generate_decl(n.type)

        elif typ == c_ast.Typename:
            return self._generate_type(n.type, emit_declname=emit_declname)

        elif typ == c_ast.IdentifierType:
            return ' '.join(n.names) + ' '

        elif typ in (c_ast.ArrayDecl, c_ast.PtrDecl, c_ast.FuncDecl):
            return self._generate_type(n.type, modifiers + [n],
                                       emit_declname=emit_declname)
        else:
            return self.visit(n)

    @staticmethod
    def parse(input_header: str, pp: str, private_filter: bool) -> dict:
        cparser = pycparser.CParser()
        ast = cparser.parse(pp, input_header)

        generator = JSONGenerator(private_filter)
        res = json.loads(generator.visit(ast))
        return res


if __name__ == '__main__':

    with open(r'lvgl.pp', 'r') as f:
        _data = f.read()

    json_api = JSONGenerator.parse(r'../../lib/lvgl/lvgl.h', _data, private_filter=False)

    # This is a check to see if something went
    # amiss with the encoding of the JSON data
    def iter_j(dct):
        for k, v in dct.items():
            if isinstance(v, dict):
                try:
                    iter_j(v)
                except RuntimeError:
                    print(dct)
                    raise
            elif isinstance(v, list):
                for i in v:
                    if isinstance(i, dict):
                        try:
                            iter_j(i)
                        except RuntimeError:
                            print(dct)
                            raise

                    elif isinstance(i, str):
                        if '"' in i or i == '*':
                            print(dct)
                            raise RuntimeError()
            elif isinstance(v, str):
                if '"' in v or v == '*':
                    print(dct)
                    raise RuntimeError()


    for items in json_api.values():
        for itm in items:
            iter_j(itm)

    print(json.dumps(json_api, indent=4))
