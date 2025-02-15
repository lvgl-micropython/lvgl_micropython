import os

try:
    import toml
except ImportError:
    raise RuntimeError(
        'The toml library is needed to use this feature.\n'
        'Please run "pip3 install toml" and then restart your build'
    )

used_imports = []
global_variable_names = []


class TOMLMeta(type):

    def __call__(cls, name, parent=None, **kwargs):
        if name and name == 'conditional':
            return TOMLConditional(name, None, **kwargs)

        children = {}
        for key, value in list(kwargs.items()):
            if isinstance(value, dict):
                children[key] = value
                del kwargs[key]

        instance = super().__call__(name, parent=parent, **kwargs)

        for child_name, child_data in children.items():
            child = TOMLObject(child_name, parent=instance, **child_data)
            instance.add_child(child)

        return instance


class TOMLObject(metaclass=TOMLMeta):

    def __init__(self, name, parent=None, **kwargs):
        if parent is not None and parent.name == 'MCU':
            self.build_args = kwargs
            self.mcu = name

            paren = parent.parent
            while paren.parent is not None:
                paren = parent.parent

            paren.mcu_obj = TOMLmcu(self)
        else:
            self.mcu = None

        self.mcu_obj = None
        self.name = name
        self.parent = parent
        self.__kwargs = kwargs
        self.__children = []
        self.imports = []

    def add_child(self, child):
        if child.name != 'MCU':
            self.__children.append(child)

    def __getattr__(self, item):
        if item in self.__dict__:
            return self.__dict__[item]

        if item in self.__kwargs:
            return self.__kwargs[item]

        raise AttributeError(item)

    @property
    def fqn(self):
        if self.parent is not None and self.parent.name == 'Pin':
            return self.name + ' = ' + self.parent.fqn

        if self.__kwargs:
            if 'params' in self.__kwargs or 'value' in self.__kwargs:
                return self.parent.fqn + '.' + self.name

            return self.name + ' = ' + self.parent.fqn

        if self.name == 'RGBDisplay':
            return 'rgb_display.RGBDisplay'

        if self.name == 'SDLDisplay':
            return 'sdl_display.SDLDisplay'

        if self.name == 'SDLPointer':
            return 'sdl_pointer.SDLPointer'

        if self.name == 'I2C':
            return 'i2c.I2C'

        if self.name == 'Spi3Wire':
            return 'spi3wire.Spi3Wire'

        if self.name == 'SPI':
            return 'machine.SPI'

        if self.name == 'SDCard':
            return 'machine.SDCard'

        if self.name.lower() in display_drivers:
            return self.name.lower() + '.' + self.name

        if self.name.lower() in indev_drivers:
            return self.name.lower() + '.' + self.name

        if self.name in io_expanders:
            return self.name

        if self.name in ('I80Bus', 'SPIBus', 'I2CBus', 'RGBBus'):
            return 'lcd_bus.' + self.name

        if self.parent is None:
            return None

        if self.parent.name:
            return self.parent.fqn + '.' + self.name

        return self.name

    @property
    def var_names(self):
        if self.__kwargs:
            fqn = self.fqn

            if '=' in fqn:
                return [fqn.split('=')[0].strip()]

            return []

        var_names = []

        for child in self.__children:
            if isinstance(child, TOMLConditional):
                continue

            var_names.extend(child.var_names)

        return var_names

    @property
    def constants(self):
        res = []

        if not self.__children:
            for key, value in list(self.__kwargs.items()):
                if not isinstance(value, int) or key == 'value':
                    continue

                name = self.name.upper()

                key_upper = key.upper()
                if name not in key_upper:
                    key_upper = name + '_' + key_upper

                res.append(f'_{key_upper} = const({value})')
                self.__kwargs[key] = f'_{key_upper}'
        else:
            for child in self.__children:
                if isinstance(child, TOMLConditional):
                    continue

                res.extend(child.constants)

        return res

    def __str__(self):
        if self.parent is None:
            global_variable_names.extend(self.var_names)

            output = []
            output.extend(self.constants)

            for child in self.__children:
                if isinstance(child, TOMLConditional):
                    output.append('')

                elif child.name not in global_variable_names:
                    module = child.fqn.split('.')[0]
                    if module not in self.imports and module not in used_imports:
                        self.imports.append(module)
                        used_imports.append(module)
                        output.extend(['', f'import {module}', ''])

                output.append(str(child))
                if isinstance(child, TOMLConditional):
                    output.append('')

            if output:
                output = [
                    'from micropython import const',
                    'import lvgl as lv',
                    '',
                    ''
                ] + output

            return '\n'.join(output)

        if self.__children and not self.__kwargs:
            output = [str(child) for child in self.__children]
            return '\n'.join(output)

        fqn = self.fqn

        if len(self.__kwargs) == 1:
            key = list(self.__kwargs.keys())[0]
            if key == 'params':
                output = ''
                for param in self.__kwargs[key]:
                    if isinstance(param, str) and '.' in param:
                        mod = param.split('.', 1)[0]
                        if (
                            mod not in used_imports and
                            mod not in global_variable_names and (
                                mod in display_drivers or
                                mod in indev_drivers or
                                mod in io_expanders
                            )
                        ):
                            output += f'import {mod}\n\n'
                            used_imports.append(mod)

                params = ', '.join(str(itm) for itm in self.__kwargs[key])
                output += f'{fqn}({params})'
                return output
            else:
                output = ''
                if (
                    isinstance(self.__kwargs[key], str) and
                    '.' in self.__kwargs[key]
                ):
                    mod = self.__kwargs[key].split('.', 1)[0]
                    if (
                        mod not in used_imports and
                        mod not in global_variable_names and (
                            mod in display_drivers or
                            mod in indev_drivers or
                            mod in io_expanders
                        )
                    ):
                        output += f'import {mod}\n\n'
                        used_imports.append(mod)

                if key == 'value':
                    output += f'{fqn} = ' + str(self.__kwargs[key])
                else:
                    output += f'{fqn}({key}={str(self.__kwargs[key])})'

                return output
        else:
            output = []

            for v in self.__kwargs.values():
                if not (isinstance(v, str) and '.' in v):
                    continue

                mod = v.split('.', 1)[0]
                if (
                    mod not in used_imports and
                    mod not in global_variable_names and (
                        mod in display_drivers or
                        mod in indev_drivers or
                        mod in io_expanders
                    )
                ):
                    output.append(f'import {mod}')
                    used_imports.append(mod)
            if output:
                output.append('')

            params = ',\n'.join(f'    {k}={str(v)}' for k, v in self.__kwargs.items() if not isinstance(v, dict))
            if params:
                output.append(f'{fqn}(\n{params}\n)\n')
            else:
                raise RuntimeError

            for child in self.__children:
                output.append(self.name + '.' + str(child).split('.', 2)[-1])

            if len(output) > 2:
                output.append('')

            return '\n'.join(output)


class TOMLConditionalObject:

    def __init__(self, name, parent=None, **kwargs):
        self.name = name
        self.parent = parent
        self.__children = []

        for key, value in list(kwargs.items())[:]:
            if isinstance(value, dict):
                self.__children.append(TOMLConditionalObject(key, self, **value))
                del kwargs[key]

        self.__kwargs = kwargs

    def get_conditional(self):
        if 'not_equal' in self.__kwargs:
            return f'{self.name} != {self.__kwargs["not_equal"]}'
        elif 'equal' in self.__kwargs:
            return f'{self.name} == {self.__kwargs["equal"]}'
        elif 'greater_than' in self.__kwargs:
            return f'{self.name} > {self.__kwargs["greater_than"]}'
        elif 'less_than' in self.__kwargs:
            return f'{self.name} < {self.__kwargs["less_than"]}'
        elif 'greater_than_or_equal' in self.__kwargs:
            return f'{self.name} >= {self.__kwargs["greater_than_or_equal"]}'
        elif 'less_than_or_equal' in self.__kwargs:
            return f'{self.name} <= {self.__kwargs["less_than_or_equal"]}'
        elif 'is' in self.__kwargs:
            return f'{self.name} is {self.__kwargs["is"]}'
        elif 'is_not' in self.__kwargs:
            return f'{self.name} is not {self.__kwargs["is_not"]}'
        elif 'in' in self.__kwargs:
            return f'{self.name} in {self.__kwargs["in"]}'
        elif 'not_in' in self.__kwargs:
            return f'{self.name} not in {self.__kwargs["not_in"]}'
        else:
            for child in self.__children:
                res = child.get_conditional()
                if res is not None:
                    return f'{self.name}.{res}'

    def is_conditional(self):
        cond = self.get_conditional()
        return cond is not None and not cond.startswith(f'{self.name}.')

    def get_objects(self):
        res = []

        base_conditional = self.parent
        while not isinstance(base_conditional, TOMLConditional):
            base_conditional = base_conditional.parent

        if self.is_conditional():
            for child in self.__children:
                res.extend(child.get_objects())
        else:
            if 'value' in self.__kwargs:
                names = []
                parent = self.parent
                while parent.name and not isinstance(parent, TOMLConditional):
                    names.insert(0, parent.name)
                    parent = parent.parent

                while names and not base_conditional.has_variable_name('.'.join(names)):
                    names.pop(0)

                names.append(self.name)
                var_name = '.'.join(names)
                base_conditional.add_variable_name(var_name)
                res.append(f'{var_name} = {self.__kwargs["value"]}')

            elif 'params' in self.__kwargs:
                params = ', '.join(str(item) for item in self.__kwargs['params'])
                if self.name == 'del':
                    res.append(f'{self.name} {params}')
                else:
                    res.append(f'{self.name}({params})')

                return res

            for child in self.__children:
                lines = child.get_objects()

                for line in lines:
                    if ' = ' in line:
                        var_name = line.split(' = ', 1)[0]
                        if base_conditional.has_variable_name(var_name):
                            res.append(line)
                            continue
                    else:
                        try:
                            func_name = line.split('(', 1)[0]
                            if base_conditional.has_variable_name(func_name):
                                res.append(line)
                                continue

                            func_name = func_name.split('.', 1)[0]
                            if base_conditional.has_variable_name(func_name):
                                res.append(line)
                                continue
                        except IndexError:
                            pass

                    if line.startswith('del '):
                        res.append(line)
                    else:
                        res.append(f'{self.name}.{line}')
        return res

    def is_variable(self):
        return 'value' in self.__kwargs

    def is_function(self):
        return 'params' in self.__kwargs


class TOMLConditional:

    def __init__(self, name, parent=None, **kwargs):
        self.name = name
        self.parent = parent
        self.__children = []
        self.__variable_names = []

        for key, value in list(kwargs.items())[:]:
            if isinstance(value, dict):
                self.__children.append(TOMLConditionalObject(key, self, **value))
                del kwargs[key]

        self.__kwargs = kwargs

        code = []
        for child in self.__children:
            cond = child.get_conditional()
            if cond is not None:
                code.append(f'if {cond}:')
                break
        else:
            raise RuntimeError('Unable to locate conditional argument')

        for child in self.__children:
            code.extend([f'    {line}' for line in child.get_objects()])

        self.__code = '\n'.join(code)

    def add_variable_name(self, name):
        if name not in self.__variable_names:
            self.__variable_names.append(name)

    def has_variable_name(self, name):
        return name in self.__variable_names

    def __str__(self):
        return self.__code


class TOMLmcu:

    def __init__(self, mcu):
        name = mcu.mcu
        build_args = mcu.build_args

        command = [name]
        for arg, value in build_args.items():
            if arg.islower():
                build_arg = '--' + arg.replace('_', '-')
                if isinstance(value, bool) and not value:
                    raise SyntaxError(
                        'optionless build commands must be set to "true"\n'
                        f'if they are used in the toml file. ({arg} = {repr(value)})'
                    )
            else:
                build_arg = arg

            if not isinstance(value, bool):
                build_arg = f'{build_arg}={value}'

            command.append(build_arg)

        self.build_command = command


base_path = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))

display_driver_path = os.path.join(base_path, 'api_drivers/common_api_drivers/display')
indev_driver_path = os.path.join(base_path, 'api_drivers/common_api_drivers/indev')
io_expander_path = os.path.join(base_path, 'api_drivers/common_api_drivers/io_expander')


display_drivers = [file for file in os.listdir(display_driver_path) if not file.endswith('.wip') and not file.endswith('.py')]
indev_drivers = [file[:-3] for file in os.listdir(indev_driver_path) if file.endswith('.py')]
io_expanders = [file[:-3] for file in os.listdir(io_expander_path) if file.endswith('.py')]


def run(toml_path, output_file):

    if not os.path.exists(toml_path):
        raise RuntimeError(f'inable to locate .toml ({toml_path})')

    try:
        with open(toml_path, 'r') as f:
            toml_data = toml.load(f)

        toml_obj = TOMLObject('', **toml_data)

        t_data = str(toml_obj)

        if t_data:
            with open(output_file, 'w') as f:
                f.write(t_data)

        displays = [f'DISPLAY={item}' for item in toml_obj.imports if item in display_drivers]
        indevs = [f'INDEV={item}' for item in toml_obj.imports if item in indev_drivers]
        expanders = [f'EXPANDER={item}' for item in toml_obj.imports if item in io_expanders]

        if toml_obj.mcu_obj is None:
            build_command = []
        else:
            build_command = toml_obj.mcu_obj.build_command

        for display in displays[:]:
            if display not in build_command:
                build_command.append(display)

        for indev in indevs[:]:
            if indev not in build_command:
                build_command.append(indev)

        for expander in expanders[:]:
            if expander not in build_command:
                build_command.append(expander)

        return build_command

    except OSError as err:
        raise RuntimeError(f'Unable to write data to {output_file}') from err
    except Exception as err:  # NOQA
        raise SyntaxError(f'Unable to parse .toml file ({toml_path})') from err


