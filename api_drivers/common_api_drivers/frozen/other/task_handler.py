import lvgl as lv  # NOQA
import micropython  # NOQA
import sys

try:
    from machine import Timer  # NOQA
except:
    try:
        from lv_timer import Timer  # NOQA
    except:
        raise RuntimeError("Missing machine.Timer implementation!")


_default_timer_id = 0

if sys.platform in ('pyboard', 'rp2'):
    _default_timer_id = -1


def _default_exception_hook(e):
    sys.print_exception(e)
    TaskHandler._current_instance.deinit()  # NOQA


class TaskHandler(object):
    _current_instance = None

    def __init__(
        self,
        duration=33,
        timer_id=_default_timer_id,
        max_scheduled=2,
        refresh_cb=None,
        exception_hook=_default_exception_hook
    ):
        if TaskHandler._current_instance is not None:
            self.__dict__.update(TaskHandler._current_instance.__dict__)
        else:
            if not lv.is_initialized():
                lv.init()

            TaskHandler._current_instance = self

            self.duration = duration
            self.refresh_cb = refresh_cb
            self.exception_hook = exception_hook

            self._timer = Timer(timer_id)

            # Allocation occurs here
            self._task_handler_ref = self._task_handler
            self.max_scheduled = max_scheduled

            self._timer.init(
                mode=Timer.PERIODIC,
                period=self.duration,
                callback=self._timer_cb
            )
            self._scheduled = 0

    def deinit(self):
        self._timer.deinit()
        TaskHandler._current_instance = None

    def disable(self):
        self._scheduled += self.max_scheduled

    def enable(self):
        self._scheduled -= self.max_scheduled

    @classmethod
    def is_running(cls):
        return cls._current_instance is not None

    def _task_handler(self, _):
        try:
            if lv._nesting.value == 0:
                lv.task_handler()

                if self.refresh_cb:
                    self.refresh_cb()

            self._scheduled -= 1
        except Exception as e:
            if self.exception_hook:
                self.exception_hook(e)

    def _timer_cb(self, _):
        lv.tick_inc(self.duration)
        if self._scheduled < self.max_scheduled:
            try:
                micropython.schedule(self._task_handler_ref, 0)
                self._scheduled += 1
            except:  # NOQA
                pass
