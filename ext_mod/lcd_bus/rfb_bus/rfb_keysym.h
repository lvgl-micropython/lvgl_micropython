/* Copyright (C) 2024  Kevin G Schlosser
 * Code that is written by thE above named is done under the GPL license
 * and that license is able to be viewed in the LICENSE file in the root
 * of this project.
 */

#ifndef __RFB_KEYSYM_H__
    #define __RFB_KEYSYM_H__

    typedef struct {
        uint8_t ascii;
        uint32_t utf;

        uint8_t state : 1;
        uint8_t is_ascii : 1;
        uint8_t is_utf : 1;
    } rfb_key_t;

    uint8_t rfb_keysym_to_ascii(uint32_t keysym, rfb_key_t *data);
    uint8_t rfb_keysym_to_utf(uint32_t keysym, rfb_key_t *data);

#endif /* __RFB_KEYSYM_H__ */