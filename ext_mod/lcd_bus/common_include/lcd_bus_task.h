

#ifndef __LCD_BUS_TASK_H__
    #define __LCD_BUS_TASK_H__

    struct _lcd_bus_lock_t {
        void *handle;
        void *buffer;
    };

    struct _lcd_bus_event_t {
        void *handle;
        void *buffer;
    };

    #define lcd_bus_event_wait(event)
    #define lcd_bus_event_set(event)
    #define lcd_bus_event_clear(event)
    #define lcd_bus_event_clear_from_isr(event)
    #define lcd_bus_event_set_from_isr(event)

    #define lcd_bus_lock_acquire(lock)
    #define lcd_bus_lock_release(lock)
    #define lcd_bus_lock_release_from_isr(lock)
    #define lcd_bus_lock_delete(lock)

#endif