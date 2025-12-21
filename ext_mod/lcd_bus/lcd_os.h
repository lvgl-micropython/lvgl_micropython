// Copyright (c) 2024 - 2025 Kevin G. Schlosser

#ifndef __LCD_OS_H__
    #define __LCD_OS_H__

    typedef struct lcd_event_t lcd_event_t;
    typedef struct lcd_lock_t lcd_lock_t;

    void lcd_event_init(lcd_event_t *event);
    void lcd_event_delete(lcd_event_t *event);
    bool lcd_event_isset(lcd_event_t *event);
    void lcd_event_set(lcd_event_t *event);
    void lcd_event_clear(lcd_event_t *event);
    void lcd_event_clear_from_isr(lcd_event_t *event);
    bool lcd_event_isset_from_isr(lcd_event_t *event);
    void lcd_event_set_from_isr(lcd_event_t *event);
    void lcd_event_wait(lcd_event_t *event);

    void lcd_lock_acquire(lcd_lock_t *lock);
    void lcd_lock_release(lcd_lock_t *lock);
    void lcd_lock_init(lcd_lock_t *lock);
    void lcd_lock_delete(lcd_lock_t *lock);
    void lcd_lock_release_from_isr(lcd_lock_t *lock);

    void lcd_rotate_task(void * self_in);

#endif
