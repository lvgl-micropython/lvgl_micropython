

#ifndef __LCD_BUS_TASK_H__
    #define __LCD_BUS_TASK_H__

    typedef struct _lcd_bus_lock_t {
        void *handle;
        void *buffer;
    } lcd_bus_lock_t;

    typedef struct _lcd_bus_event_t {
        void *handle;
        void *buffer;
    } lcd_bus_event_t;

    void lcd_bus_event_init(lcd_bus_event_t *event);


    #define lcd_bus_event_wait(event)
    #define lcd_bus_event_set(event)
    #define lcd_bus_event_clear(event)
    #define lcd_bus_event_clear_from_isr(event)
    #define lcd_bus_event_set_from_isr(event)

    void lcd_bus_event_delete(lcd_bus_event_t *event);
    bool lcd_bus_event_isset(lcd_bus_event_t *event);
    bool lcd_bus_event_isset_from_isr(lcd_bus_event_t *event);


    #define lcd_bus_lock_acquire(lock)
    #define lcd_bus_lock_release(lock)
    #define lcd_bus_lock_release_from_isr(lock)
    #define lcd_bus_lock_delete(lock)

    void lcd_bus_lock_init(lcd_bus_lock_t *lock);

    void lcd_bus_task(void *self_in);

#endif