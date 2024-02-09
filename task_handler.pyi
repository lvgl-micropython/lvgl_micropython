##############################################################################
# Event Loop module: advancing tick count and scheduling lvgl task handler.
# Import after lvgl module.
# This should be imported and used by display driver.
# Display driver should first check if an event loop is already running.
#
# Usage example with SDL:
#
#        SDL.init(auto_refresh=False)
#        # Register SDL display driver.
#        # Register SDL mouse driver
#        event_loop = lv_utils.event_loop()
#
#
# uasyncio example with SDL:
#
#        SDL.init(auto_refresh=False)
#        # Register SDL display driver.
#        # Register SDL mouse driver
#        event_loop = lv_utils.event_loop(asynchronous=True)
#        uasyncio.Loop.run_forever()
#
# uasyncio example with ili9341:
#
#        event_loop = lv_utils.event_loop(asynchronous=True) # Optional!
#        self.disp = ili9341(asynchronous=True)
#        uasyncio.Loop.run_forever()
#
# MIT license; Copyright (c) 2021 Amir Gonnen
#
##############################################################################
from typing import Callable, ClassVar, Optional
from machine import Timer

_default_timer_id: int = ...


def _default_exception_hook(e: Exception):
    ...

##############################################################################

class TaskHandler(object):
    _current_instance: Optional[ClassVar["TaskHandler"]] = ...

    duration: int = ...
    refresh_cb: Optional[Callable] = ...
    _timer: Timer = ...
    _task_handler_ref: Callable = ...

    exception_hook: Callable[[Exception], None] = ...
    max_scheduled: int = ...
    _scheduled: int = ...

    def __init__(
        self,
        duration: int = 33,
        timer_id: int = _default_timer_id,
        max_scheduled: int = 2,
        refresh_cb: Optional[Callable] = None,
        exception_hook: Callable[[Exception], None] = _default_exception_hook
    ):
        ...

    def deinit(self) -> None:
        ...

    def disable(self) -> None:
        ...

    def enable(self) -> None:
        ...

    @classmethod
    def is_running(cls) -> bool:
        ...

    def _task_handler(self, _) -> None:
        ...

    def _timer_cb(self, _) -> None:
        ...
