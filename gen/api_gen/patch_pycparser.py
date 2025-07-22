
from pycparser import c_ast


# ---------------- monkey patch code for pycparser -------------------------
# this code cleans up the output when logging the objects created by pycparser
# It makes it easier to read

def _repr(obj):
    if isinstance(obj, list):
        if len(obj) >= 1:
            if isinstance(obj[0], str):
                res = ', '.join(repr(itm) for itm in obj)
            else:
                res = ['\n'.join(f'  {line}'
                       for line in _repr(e).split('\n')) for e in obj]

                res = ',\n'.join(res)
                res = f'\n{res}\n'
        else:
            res = ''

        return f'[{res}]'
    else:
        return repr(obj)


def Node__repr__(self):
    result = f'{self.__class__.__name__}('
    res = []

    for n in self.__slots__[:-2]:
        dta = f'{n}={_repr(getattr(self, n))}'
        res.append(dta)

    res = ',\n'.join(res)

    if (
        isinstance(self, c_ast.FileAST) or
        len(self.__slots__[:-2]) > 1 or
        res.count('\n')
    ):
        res = '\n'.join(f'  {line}' for line in res.split('\n'))
        result += f'\n{res}\n)'
    else:
        result += f'{res})'

    return result


c_ast._repr = _repr
setattr(c_ast.Node, '__repr__', Node__repr__)
